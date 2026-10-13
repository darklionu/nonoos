// IntentDetector.cpp
#include "IntentDetector.h"
#include "TextProcessing.h"
#include <algorithm>
#include <cctype>

namespace chatia {

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
            if (levenshteinDistance(token, normalizedKeyword) <= 1) {
                return true;
            }
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

bool looksLikeMathExpression(const std::string& text) {
    std::string normalized = normalizeText(text);
    if (normalized.empty()) {
        return false;
    }

    const std::vector<std::string> mathKeywords = {
        "calcul", "calcule", "calculer", "addition", "plus", "soustraction",
        "moins", "multiplication", "multiplie", "fois", "division", "divise",
        "divise par", "modulo", "puissance", "resultat", "résultat"
    };

    bool hasDigit = std::any_of(normalized.begin(), normalized.end(), [](unsigned char ch) {
        return std::isdigit(ch);
    });

    bool hasMathOperator = normalized.find('+') != std::string::npos ||
                           normalized.find('-') != std::string::npos ||
                           normalized.find('*') != std::string::npos ||
                           normalized.find('/') != std::string::npos ||
                           normalized.find('%') != std::string::npos ||
                           normalized.find('^') != std::string::npos ||
                           normalized.find('(') != std::string::npos ||
                           normalized.find(')') != std::string::npos;

    bool hasMathWord = false;
    for (const std::string& keyword : mathKeywords) {
        if (normalized.find(keyword) != std::string::npos) {
            hasMathWord = true;
            break;
        }
    }

    return hasDigit && (hasMathOperator || hasMathWord);
}

Intent IntentDetector::detect(const std::string& input) const {
    std::string msg = normalizeText(input);

    if (looksLikeMathExpression(msg)) {
        return Intent::Calculator;
    }

    const std::vector<std::pair<Intent, std::vector<std::string>>> rules = {
        {Intent::Greeting, {"bonjour", "salut", "bjr", "bonsoir", "hello"}},
        {Intent::Who, {"qui es tu", "qui est tu", "quel est ton nom", "comment tu t appelles", "nom"}},
        {Intent::Weather, {"meteo", "météo", "weather", "temperature", "pluie", "soleil", "nuage"}},
        {Intent::Time, {"heure", "il est quelle heure", "heure actuelle", "histoire"}},
        {Intent::Date, {"date", "quel jour", "aujourd hui", "date du jour"}},
        {Intent::Thanks, {"merci", "thanks", "thank you"}},
        {Intent::Goodbye, {"au revoir", "bye", "adieu", "a plus"}},
        {Intent::Usb, {"usb", "port usb", "périphérique usb", "dispositif usb", "connecte au usb", "branche usb", "portserie", "ttyusb"}},
        {Intent::Help, {"aide", "help", "assistance", "support"}},
    };

    double best = 0.0;
    Intent bestIntent = Intent::Unknown;

    for (const auto& [intent, keywords] : rules) {
        double s = scoreIntent(msg, keywords);
        if (s > best) {
            best = s;
            bestIntent = intent;
        }
    }

    return bestIntent;
}

}