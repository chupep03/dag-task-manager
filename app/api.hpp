#pragma once

#include "types.hpp"
#include "config.hpp"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>

namespace api {

inline size_t WriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* out = static_cast<std::string*>(userdata);
    out->append(ptr, size * nmemb);
    return size * nmemb;
}

inline std::string HttpGet(const std::string& url) {
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl_easy_init failed");

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "TTaskSchedulerApp/1.0");

    CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
        throw std::runtime_error(std::string("curl: ") + curl_easy_strerror(res));
    if (http_code != 200)
        throw std::runtime_error("HTTP " + std::to_string(http_code) + " for " + url);
    return response;
}

inline nlohmann::json ParseJson(const std::string& body, const char* api_name) {
    try {
        return nlohmann::json::parse(body);
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error(std::string(api_name) + " JSON: " + e.what());
    }
}

}  // namespace api


inline Coordinates FetchCoordinates() {
    std::string body = api::HttpGet("https://api.2ip.ua/geo.json");
    return nlohmann::json::parse(body).get<Coordinates>();
}

inline Weather FetchWeather(const Coordinates& c) {
    std::ostringstream url;
    url << "https://api.weatherapi.com/v1/current.json"
        << "?key=" << config::kWeatherApiKey
        << "&q=" << c.lat << "," << c.lon
        << "&lang=ru";

    std::string body = api::HttpGet(url.str());
    auto j = api::ParseJson(body, "WeatherAPI");
    if (j.contains("error"))
        throw std::runtime_error("WeatherAPI: " + j["error"]["message"].get<std::string>());
    return j.get<Weather>();
}

inline Places FetchPlaces(const Coordinates& c) {
    std::ostringstream q;
    q << "[out:json][timeout:25];("
      << "nwr[\"tourism\"=\"museum\"](around:2000,"     << c.lat << "," << c.lon << ");"
      << "nwr[\"tourism\"=\"gallery\"](around:2000,"    << c.lat << "," << c.lon << ");"
      << "nwr[\"tourism\"=\"attraction\"](around:2000," << c.lat << "," << c.lon << ");"
      << "nwr[\"tourism\"=\"viewpoint\"](around:2000,"  << c.lat << "," << c.lon << ");"
      << "nwr[\"leisure\"=\"park\"](around:2000,"       << c.lat << "," << c.lon << ");"
      << "nwr[\"leisure\"=\"garden\"](around:2000,"     << c.lat << "," << c.lon << ");"
      << "nwr[\"amenity\"=\"cafe\"](around:2000,"       << c.lat << "," << c.lon << ");"
      << "nwr[\"amenity\"=\"restaurant\"](around:2000," << c.lat << "," << c.lon << ");"
      << "nwr[\"amenity\"=\"theatre\"](around:2000,"    << c.lat << "," << c.lon << ");"
      << "nwr[\"amenity\"=\"cinema\"](around:2000,"     << c.lat << "," << c.lon << ");"
      << ");out center;";

    // пробелы %20, кавычки %22
    std::string query = q.str();
    std::string encoded;
    for (char ch : query) {
        if (ch == ' ')      encoded += "%20";
        else if (ch == '"') encoded += "%22";
        else                encoded += ch;
    }

    std::string url = "https://overpass-api.de/api/interpreter?data=" + encoded;
    std::string body = api::HttpGet(url);
    auto j = api::ParseJson(body, "Overpass");

    auto detectCategory = [](const nlohmann::json& tags) -> std::string {
        if (tags.contains("tourism")) {
            auto t = tags["tourism"].get<std::string>();
            if (t == "museum")     return "museum";
            if (t == "gallery")    return "gallery";
            if (t == "attraction") return "attraction";
            if (t == "viewpoint")  return "viewpoint";
        }
        if (tags.contains("leisure")) {
            auto l = tags["leisure"].get<std::string>();
            if (l == "park")   return "park";
            if (l == "garden") return "garden";
        }
        if (tags.contains("amenity")) {
            auto a = tags["amenity"].get<std::string>();
            if (a == "cafe")       return "cafe";
            if (a == "restaurant") return "restaurant";
            if (a == "theatre")    return "theatre";
            if (a == "cinema")     return "cinema";
        }
        return "other";
    };

    Places places;
    for (const auto& el : j.at("elements")) {
        if (!el.contains("tags") || !el["tags"].contains("name")) continue;

        Place p;
        p.name = el["tags"]["name"].get<std::string>();

        if (el.contains("lat") && el.contains("lon")) {
            p.lat = el["lat"].get<double>();
            p.lon = el["lon"].get<double>();
        } else if (el.contains("center")) {
            p.lat = el["center"]["lat"].get<double>();
            p.lon = el["center"]["lon"].get<double>();
        } else {
            continue;
        }

        p.category = detectCategory(el["tags"]);
        places.push_back(std::move(p));

        if (places.size() >= config::kMaxPlaces) break;
    }
    return places;
}

inline Places FetchDistances(const Coordinates& c, const Places& places) {
    if (places.empty()) return places;

    std::string coords = std::to_string(c.lon) + "," + std::to_string(c.lat);
    for (const auto& p : places) coords += ";" + std::to_string(p.lon) + "," + std::to_string(p.lat);

    std::string url =
        "https://router.project-osrm.org/table/v1/walking/" + coords +
        "?annotations=distance";

    std::string body = api::HttpGet(url);
    auto j = api::ParseJson(body, "OSRM");

    if (j.contains("code") && j["code"] != "Ok")
        throw std::runtime_error("OSRM: " + j.value("message", "unknown"));

    Places result = places;
    const auto& row0 = j.at("distances").at(0);
    for (size_t i = 0; i < result.size(); ++i)
        result[i].distance_m = row0.at(i + 1).get<double>();

    return result;
}