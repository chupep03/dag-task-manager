#pragma once

#include <cstddef>
#include <string>

namespace config {

inline const std::string kWeatherApiKey = "ecb36afd53c54438820130919261809";

inline constexpr std::size_t kMaxPlaces = 20;
inline constexpr std::size_t kTopN      = 5;

}  // namespace config