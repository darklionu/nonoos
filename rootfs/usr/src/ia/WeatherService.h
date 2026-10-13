// WeatherService.h
#pragma once
#include <string>

namespace chatia {

class WeatherService {
public:
    std::string getWeatherForCity(const std::string& city) const;
    std::string extractCityFromPrompt(const std::string& input) const;
};

}