#pragma once

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace iso
{
    inline bool est_extension_iso(const std::filesystem::path& chemin)
    {
        if (chemin.extension().empty()) {
            return false;
        }

        std::string ext = chemin.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(),
            [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

        return ext == ".ISO";
    }

    inline bool est_iso9660(const std::filesystem::path& chemin)
    {
        std::ifstream fichier(chemin, std::ios::binary);
        if (!fichier) {
            return false;
        }

        std::array<char, 32768> tampon{};
        fichier.read(tampon.data(), static_cast<std::streamsize>(tampon.size()));

        if (fichier.gcount() < 0x8000) {
            return false;
        }

        std::string donnees(tampon.begin(), tampon.end());

        // Signature ISO 9660 typique : "CD001"
        // Elle apparaît souvent à l'offset 0x8001 dans le volume descriptor.
        const std::string signature = "CD001";
        return donnees.find(signature, 0x8001) != std::string::npos;
    }

    inline bool est_fichier_iso(const std::filesystem::path& chemin)
    {
        return std::filesystem::is_regular_file(chemin) &&
               est_extension_iso(chemin) &&
               est_iso9660(chemin);
    }

    inline std::vector<std::filesystem::path> scanner_isos(const std::filesystem::path& repertoire)
    {
        std::vector<std::filesystem::path> resultats;

        if (!std::filesystem::exists(repertoire) ||
            !std::filesystem::is_directory(repertoire)) {
            return resultats;
        }

        for (const auto& entree : std::filesystem::directory_iterator(repertoire)) {
            if (est_fichier_iso(entree.path())) {
                resultats.push_back(entree.path());
            }
        }

        std::sort(resultats.begin(), resultats.end());
        return resultats;
    }
}
