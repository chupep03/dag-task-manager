#pragma once

#include <cstddef>
#include <string>

namespace config {

inline std::string WeatherApiKey() {
    const char* k = std::getenv("WEATHER_API_KEY");
    if (k == nullptr || *k == '\0') {
        throw std::runtime_error(
            "WEATHER_API_KEY is not set. "
            "Export it: export WEATHER_API_KEY=<your_key>");
    }
    return k;
}

inline constexpr std::size_t kMaxPlaces = 20;
inline constexpr std::size_t kTopN      = 5;

}  // namespace config