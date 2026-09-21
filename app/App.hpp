#pragma once

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>

#include "api.hpp"
#include "config.hpp"
#include "types.hpp"
#include "TTaskScheduler.hpp"

class App {
private:
    TTaskScheduler scheduler_;

    static std::string ToLower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return s;
    }

    static bool IsOutdoor(const std::string& category) {
        return category == "park" ||
               category == "garden" ||
               category == "viewpoint";
    }

    static Places FilterByWeather(const Weather& w, const Places& places) {
        Places result;
        const bool bad = w.is_rain || w.is_snow;

        for (const auto& p : places) {
            if (bad && IsOutdoor(p.category)) continue;
            result.push_back(p);
        }

        std::sort(result.begin(), result.end(),
                  [](const Place& a, const Place& b) {
                      return a.distance_m < b.distance_m;
                  });

        return result;
    }

    static void PrintSummary(const Coordinates& c,
                             const Weather& w,
                             const Places& places) {
        std::cout << "City: " << c.city
                  << " (" << c.lat << ", " << c.lon << ")\n";
        std::cout << "Weather: " << w.temp_c << "C, "
                  << w.condition
                  << (w.is_rain ? ", rain" : "")
                  << (w.is_snow ? ", snow" : "")
                  << "\n";

        if (places.empty()) {
            std::cout << "Nothing here :(\n";
            return;
        }

        std::cout << "Recomendations:\n";
        const size_t n = std::min(places.size(), config::kTopN);
        for (size_t i = 0; i < n; ++i) {
            const auto& p = places[i];
            double km = p.distance_m / 1000.0;
            std::cout << "  " << (i + 1) << ". " << p.name
                      << " — " << km << " km\n";
        }
    }

public:
    App() = default;
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void Run() {
        auto locFut = scheduler_.Add([] {
            return FetchCoordinates();
        });

        auto weatherFut = scheduler_.Add(
            [](Coordinates c) { return FetchWeather(c); },
            locFut.GetFutureResult<Coordinates>()
        );

        auto placesFut = scheduler_.Add(
            [](Coordinates c) { return FetchPlaces(c); },
            locFut.GetFutureResult<Coordinates>()
        );

        auto withDistFut = scheduler_.Add(
            [](Coordinates c, Places p) { return FetchDistances(c, p); },
            locFut.GetFutureResult<Coordinates>(),
            placesFut.GetFutureResult<Places>()
        );

        auto filteredFut = scheduler_.Add(
            [](Weather w, Places p) { return FilterByWeather(w, p); },
            weatherFut.GetFutureResult<Weather>(),
            withDistFut.GetFutureResult<Places>()
        );

        auto printFut = scheduler_.Add(
            [](Coordinates c, Weather w, Places p) {
                PrintSummary(c, w, p);
                return 0;
            },
            locFut.GetFutureResult<Coordinates>(),
            weatherFut.GetFutureResult<Weather>(),
            filteredFut.GetFutureResult<Places>()
        );

        scheduler_.ExecuteAll();
        printFut.GetFutureResult<int>().Get();
    }
};