#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <ctime>
#include <vector>

using json = nlohmann::json;

static size_t writeCallback(void* ptr, size_t size, size_t nmemb, std::string* out) {
    size_t total = size * nmemb;
    out->append(static_cast<char*>(ptr), total);
    return total;
}

std::string urlEncode(const std::string& value) {
    CURL* curl = curl_easy_init();
    char* encoded = curl_easy_escape(curl, value.c_str(), static_cast<int>(value.length()));
    std::string result = encoded ? encoded : value;
    curl_free(encoded);
    curl_easy_cleanup(curl);
    return result;
}

std::string fetchHttp(const std::string& url) {
    CURL* curl = curl_easy_init();
    std::string response;

    if (!curl) {
        return "";
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    return (res == CURLE_OK) ? response : "";
}

std::string stripAccents(const std::string& input) {
    std::string result = input;

    const std::vector<std::pair<std::string, std::string>> replacements = {
        {"é", "e"}, {"è", "e"}, {"ê", "e"}, {"ë", "e"},
        {"à", "a"}, {"â", "a"}, {"ä", "a"},
        {"î", "i"}, {"ï", "i"},
        {"ô", "o"}, {"ö", "o"},
        {"ù", "u"}, {"û", "u"}, {"ü", "u"},
        {"ç", "c"},
        {"œ", "oe"}, {"æ", "ae"},
        {"É", "E"}, {"È", "E"}, {"Ê", "E"}, {"Ë", "E"},
        {"À", "A"}, {"Â", "A"}, {"Ä", "A"},
        {"Î", "I"}, {"Ï", "I"},
        {"Ô", "O"}, {"Ö", "O"},
        {"Ù", "U"}, {"Û", "U"}, {"Ü", "U"},
        {"Ç", "C"}
    };

    for (const auto& [from, to] : replacements) {
        size_t pos = 0;
        while ((pos = result.find(from, pos)) != std::string::npos) {
            result.replace(pos, from.size(), to);
            pos += to.size();
        }
    }

    return result;
}

std::string normalizeText(const std::string& input) {
    std::string s = stripAccents(input);

    std::string out;
    out.reserve(s.size());

    for (unsigned char c : s) {
        if (std::isalnum(c)) {
            out.push_back(static_cast<char>(std::tolower(c)));
        } else if (c == ' ' || c == '\n' || c == '\r' || c == '\t' ||
                   c == '-' || c == '\'' || c == '.' || c == ',') {
            out.push_back(' ');
        }
    }

    std::string cleaned;
    cleaned.reserve(out.size());
    bool previousSpace = false;

    for (char ch : out) {
        if (ch == ' ') {
            if (!previousSpace) cleaned.push_back(' ');
            previousSpace = true;
        } else {
            cleaned.push_back(ch);
            previousSpace = false;
        }
    }

    while (!cleaned.empty() && cleaned.front() == ' ') cleaned.erase(cleaned.begin());
    while (!cleaned.empty() && cleaned.back() == ' ') cleaned.pop_back();

    return cleaned;
}

std::vector<std::string> splitWords(const std::string& text) {
    std::vector<std::string> tokens;
    std::string current;

    for (char ch : text) {
        if (std::isalnum(static_cast<unsigned char>(ch))) {
            current.push_back(static_cast<char>(std::tolower(ch)));
        } else if (!current.empty()) {
            tokens.push_back(current);
            current.clear();
        }
    }

    if (!current.empty()) {
        tokens.push_back(current);
    }

    return tokens;
}

size_t levenshteinDistance(const std::string& a, const std::string& b) {
    const size_t n = a.size();
    const size_t m = b.size();

    std::vector<std::vector<size_t>> dp(n + 1, std::vector<size_t>(m + 1));

    for (size_t i = 0; i <= n; ++i) dp[i][0] = i;
    for (size_t j = 0; j <= m; ++j) dp[0][j] = j;

    for (size_t i = 1; i <= n; ++i) {
        for (size_t j = 1; j <= m; ++j) {
            if (a[i - 1] == b[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1];
            } else {
                dp[i][j] = 1 + std::min({ dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1] });
            }
        }
    }

    return dp[n][m];
}

bool keywordMatch(const std::string& text, const std::string& keyword) {
    std::string normalizedText = normalizeText(text);
    std::string normalizedKeyword = normalizeText(keyword);

    if (normalizedKeyword.empty()) return false;

    if (normalizedText.find(normalizedKeyword) != std::string::npos) {
        return true;
    }

    auto tokens = splitWords(normalizedText);
    for (const std::string& token : tokens) {
        if (token == normalizedKeyword) return true;

        if (token.size() >= 3) {
            size_t dist = levenshteinDistance(token, normalizedKeyword);
            if (dist <= 1) return true;
        }
    }

    return false;
}

double scoreIntent(const std::string& text, const std::vector<std::string>& keywords) {
    double score = 0.0;
    std::string normalizedText = normalizeText(text);

    for (const std::string& kw : keywords) {
        if (normalizedText.find(normalizeText(kw)) != std::string::npos) {
            score += 2.0;
        }

        if (keywordMatch(normalizedText, kw)) {
            score += 1.5;
        }
    }

    return score;
}

std::string weatherDescription(int code) {
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

std::string getWeatherForCity(const std::string& city) {
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

std::string extractCityFromWeatherPrompt(const std::string& input) {
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

    if (city.empty()) return "Paris";
    return city;
}

std::string generateReply(const std::string& input) {
    std::string msg = normalizeText(input);

    double greetingScore = scoreIntent(msg, { "bonjour", "salut", "bjr", "bonsoir", "hello" });
    double whoAreYouScore = scoreIntent(msg, { "qui es tu", "qui est tu", "quel est ton nom", "comment tu t appelles", "nom" });
    double weatherScore = scoreIntent(msg, { "meteo", "météo", "weather", "temperature", "pluie", "soleil", "nuage" });
    double timeScore = scoreIntent(msg, { "heure", "il est quelle heure", "heure actuelle", "histoire" });
    double dateScore = scoreIntent(msg, { "date", "quel jour", "aujourd hui", "date du jour" });
    double thanksScore = scoreIntent(msg, { "merci", "thanks", "thank you" });
    double goodbyeScore = scoreIntent(msg, { "au revoir", "bye", "adieu", "a plus" });

    double bestScore = 0.0;
    std::string intent = "unknown";

    if (greetingScore > bestScore) { bestScore = greetingScore; intent = "greeting"; }
    if (whoAreYouScore > bestScore) { bestScore = whoAreYouScore; intent = "who"; }
    if (weatherScore > bestScore) { bestScore = weatherScore; intent = "weather"; }
    if (timeScore > bestScore) { bestScore = timeScore; intent = "time"; }
    if (dateScore > bestScore) { bestScore = dateScore; intent = "date"; }
    if (thanksScore > bestScore) { bestScore = thanksScore; intent = "thanks"; }
    if (goodbyeScore > bestScore) { bestScore = goodbyeScore; intent = "goodbye"; }

    if (intent == "greeting") {
        return "Bonjour ! Je suis ton robot, comment puis-je t'aider ?";
    }

    if (intent == "who") {
        return "Je suis ton assistant robot, conçu pour répondre à tes questions.";
    }

    if (intent == "weather") {
        std::string city = extractCityFromWeatherPrompt(input);
        return getWeatherForCity(city);
    }

    if (intent == "time") {
        time_t now = time(0);
        tm* local = localtime(&now);
        if (!local) {
            return "Je ne peux pas lire l’heure actuellement.";
        }
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%H:%M:%S", local);
        return std::string("Il est ") + buffer + ".";
    }

    if (intent == "date") {
        time_t now = time(0);
        tm* local = localtime(&now);
        if (!local) {
            return "Je ne peux pas lire la date actuellement.";
        }
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%d/%m/%Y", local);
        return std::string("Aujourd'hui, nous sommes le ") + buffer + ".";
    }

    if (intent == "thanks") {
        return "Avec plaisir !";
    }

    if (intent == "goodbye") {
        return "Au revoir ! À bientôt.";
    }

    return "Je ne comprends pas encore tout, mais je peux t'aider avec bonjour, météo, heure, date, ou simplement parler.";
}

// TextProcessing.h
#pragma once
#include <string>
#include <vector>

namespace chatia {
    std::string stripAccents(const std::string& input);
    std::string normalizeText(const std::string& input);
    std::vector<std::string> splitWords(const std::string& text);
    size_t levenshteinDistance(const std::string& a, const std::string& b);
}