// TextProcessing.cpp
#include "TextProcessing.h"
#include <algorithm>
#include <cctype>
#include <vector>

namespace chatia {

std::string stripAccents(const std::string& input) {
    std::string result = input;
    const std::vector<std::pair<std::string, std::string>> replacements = {
        {"é","e"}, {"è","e"}, {"ê","e"}, {"ë","e"},
        {"à","a"}, {"â","a"}, {"ä","a"},
        {"î","i"}, {"ï","i"},
        {"ô","o"}, {"ö","o"},
        {"ù","u"}, {"û","u"}, {"ü","u"},
        {"ç","c"},
        {"œ","oe"}, {"æ","ae"},
        {"É","E"}, {"È","E"}, {"Ê","E"}, {"Ë","E"},
        {"À","A"}, {"Â","A"}, {"Ä","A"},
        {"Î","I"}, {"Ï","I"},
        {"Ô","O"}, {"Ö","O"},
        {"Ù","U"}, {"Û","U"}, {"Ü","U"},
        {"Ç","C"}
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
                   c == '-' || c == '\'' || c == '.' || c == ',' ||
                   c == '+' || c == '*' || c == '/' || c == '%' ||
                   c == '^' || c == '(' || c == ')' || c == '=') {
            if (c == '+' || c == '*' || c == '/' || c == '%' ||
                c == '^' || c == '(' || c == ')' || c == '=') {
                out.push_back(static_cast<char>(c));
            } else {
                out.push_back(' ');
            }
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

}