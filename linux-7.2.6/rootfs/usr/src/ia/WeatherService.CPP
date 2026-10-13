// WeatherService.cpp
#include "WeatherService.h"
#include "TextProcessing.h"
#include <algorithm>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <sstream>

using json = nlohmann::json;

namespace chatia {

static size_t writeCallback(void* ptr, size_t size, size_t nmemb, std::string* out) {
    size_t total = size * nmemb;
    out->append(static_cast<char*>(ptr), total);
    return total;
}

static std::string urlEncode(const std::string& value) {
    CURL* curl = curl_easy_init();
    if (!curl) return value;

    char* encoded = curl_easy_escape(curl, value.c_str(), static_cast<int>(value.length()));
    std::string result = encoded ? encoded : value;
    curl_free(encoded);
    curl_easy_cleanup(curl);
    return result;
}

static std::string fetchHttp(const std::string& url) {
    CURL* curl = curl_easy_init();
    if (!curl) return "";

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    return (res == CURLE_OK) ? response : "";
}

static std::string weatherDescription(int code) {
    switch (code) {
        case 0: return "ciel clair";
        case 1: return "principalement clair";
        case 2: return "partiellement nuageux";
        case 3: return "couvert";
        case 45:
        case 48: return "brouillard";
        case 51:
        case 53:
        case 55: return "bruine";
        case 56:
        case 57: return "bruine verglaçante";
        case 61:
        case 63:
        case 65: return "pluie";
        case 66:
        case 67: return "pluie verglaçante";
        case 71:
        case 73:
        case 75:
        case 77:
        case 85:
        case 86: return "neige";
        case 80:
        case 81:
        case 82: return "averse";
        case 95: return "orage";
        case 96:
        case 99: return "orage avec grêle";
        default: return "conditions météo";
    }
}

std::string WeatherService::extractCityFromPrompt(const std::string& input) const {
    std::string normalized = normalizeText(input);

    std::vector<std::string> stopWords = {
        "meteo", "weather", "temperature", "quelle", "quel", "est",
        "la", "le", "de", "a", "au", "sur", "pour", "il", "fait",
        "quelleest", "quelest"
    };

    std::vector<std::string> tokens = splitWords(normalized);
    std::string city;

    for (const std::string& token : tokens) {
        if (std::find(stopWords.begin(), stopWords.end(), token) == stopWords.end()) {
            if (!city.empty()) city += " ";
            city += token;
        }
    }

    return city.empty() ? "Paris" : city;
}

std::string WeatherService::getWeatherForCity(const std::string& city) const {
    std::string cityEncoded = urlEncode(city);

    std::string geoUrl = "https://geocoding-api.open-meteo.com/v1/search?name="
        + cityEncoded + "&count=1&language=fr&format=json";

    std::string geoJson = fetchHttp(geoUrl);
    if (geoJson.empty()) {
        return "Je n’ai pas pu récupérer la météo pour " + city + ".";
    }

    try {
        json geo = json::parse(geoJson);
        if (!geo.contains("results") || geo["results"].empty()) {
            return "Je n’ai pas trouvé cette ville : " + city + ".";
        }

        std::string cityName = geo["results"][0]["name"];
        double lat = geo["results"][0]["latitude"];
        double lon = geo["results"][0]["longitude"];

        std::string forecastUrl =
            "https://api.open-meteo.com/v1/forecast?latitude=" +
            std::to_string(lat) + "&longitude=" + std::to_string(lon) +
            "&current=temperature_2m,weather_code&timezone=auto&language=fr";

        std::string forecastJson = fetchHttp(forecastUrl);
        if (forecastJson.empty()) {
            return "Je n’ai pas pu récupérer la météo pour " + cityName + ".";
        }

        json forecast = json::parse(forecastJson);
        if (!forecast.contains("current")) {
            return "Erreur lors de la lecture de la météo pour " + city + ".";
        }

        double temp = forecast["current"]["temperature_2m"];
        int code = forecast["current"]["weather_code"];

        std::ostringstream oss;
        oss << "Météo à " << cityName << " : " << temp << "°C, "
            << weatherDescription(code) << ".";

        return oss.str();
    } catch (const std::exception&) {
        return "Erreur lors de la lecture de la météo pour " + city + ".";
    }
}

}