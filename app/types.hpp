#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

struct Coordinates {
    double lat = 0.0;
    double lon = 0.0;
    std::string city;
};

struct Weather {
    double temp_c = 0.0;
    std::string condition;
    std::string description;
    bool is_rain = false;
    bool is_snow = false;
    double wind_kph = 0.0;
};

struct Place {
    std::string name;
    std::string category;
    double lat = 0.0;
    double lon = 0.0;
    double distance_m = 0.0;
};

using Places = std::vector<Place>;

inline void from_json(const nlohmann::json& j, Coordinates& c) {
    c.lat  = std::stod(j.at("latitude").get<std::string>());
    c.lon  = std::stod(j.at("longitude").get<std::string>());
    c.city = j.at("city_rus").get<std::string>();
}

inline void from_json(const nlohmann::json& j, Weather& w) {
    const auto& cur = j.at("current");
    w.temp_c    = cur.at("temp_c").get<double>();
    w.condition = cur.at("condition").at("text").get<std::string>();
    w.wind_kph  = cur.at("wind_kph").get<double>();

    int code = cur.at("condition").at("code").get<int>();
    w.is_rain = (code == 1063 || code == 1072 ||
                 (code >= 1150 && code <= 1201) ||
                 (code >= 1240 && code <= 1246));
    w.is_snow = (code == 1066 || code == 1114 || code == 1117 ||
                 (code >= 1210 && code <= 1225) ||
                 (code >= 1255 && code <= 1258));
}