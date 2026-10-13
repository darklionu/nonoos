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