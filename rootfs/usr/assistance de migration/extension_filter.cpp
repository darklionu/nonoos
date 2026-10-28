#include "detector.hpp"
#include "extensions.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

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