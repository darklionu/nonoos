#include "detector.hpp"
#include "extension_filter.hpp"
#include "extensions.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

std::string normalize_extension(std::string extension)
{
    if (extension.empty()) {
        return {};
    }

    if (extension.front() != '.') {
        extension.insert(extension.begin(), '.');
    }

    std::transform(
        extension.begin(),
        extension.end(),
        extension.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    return extension;
}

} // namespace

bool has_allowed_extension(
    const std::filesystem::path& file,
    const std::vector<std::string>& extensions)
{
    if (extensions.empty()) {
        return true;
    }

    const auto file_extension = normalize_extension(file.extension().string());
    if (file_extension.empty()) {
        return false;
    }

    for (const auto& candidate : extensions) {
        if (normalize_extension(candidate) == file_extension) {
            return true;
        }
    }
    return false;
}

int main()
{
    namespace fs = std::filesystem;

    demander_extensions();

#ifdef _WIN32
    std::vector<fs::path> roots;

    for (char letter = 'A'; letter <= 'Z'; ++letter) {
        fs::path drive = std::string(1, letter) + ":\\";

        std::error_code error;
        if (fs::exists(drive, error)) {
            roots.push_back(drive);
        }
    }
#else
    const std::vector<fs::path> roots = {"/home"};
#endif

    for (const auto& root : roots) {
        const auto report = list_files(root.string(), get_extensions());

        for (const auto& file : report.files) {
            std::cout << file << '\n';
        }

        for (const auto& error : report.errors) {
            std::cerr << "Erreur : " << error << '\n';
        }
    }

    return 0;
}