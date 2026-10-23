#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "RappelService.h"
#include "TextProcessing.h"

namespace chatia {
namespace {

struct Rappel {
    std::size_t id;
    std::string text;
};

bool getStoragePath(std::filesystem::path& path, std::string& error) {
    const char* home = std::getenv("HOME");
    if (home == nullptr || *home == '\0') {
        error = "Impossible de trouver le dossier personnel (HOME) pour enregistrer les rappels.";
        return false;
    }

    path = std::filesystem::path(home) / ".config" / "chatia" / "rappels.txt";
    return true;
}

bool loadRappels(const std::filesystem::path& path, std::vector<Rappel>& rappels,
                 std::string& error) {
    std::ifstream file(path);
    if (!file) {
        std::error_code statusError;
        const bool exists = std::filesystem::exists(path, statusError);
        if (!statusError && !exists) {
            return true;
        }
        error = "Impossible de lire le fichier des rappels.";
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream record(line);
        Rappel rappel{};
        if (!(record >> rappel.id >> std::quoted(rappel.text)) || rappel.text.empty()) {
            error = "Le fichier des rappels contient une entrée invalide.";
            return false;
        }
        rappels.push_back(std::move(rappel));
    }
    if (file.bad()) {
        error = "Une erreur est survenue pendant la lecture des rappels.";
        return false;
    }
    return true;
}

bool saveRappels(const std::filesystem::path& path, const std::vector<Rappel>& rappels,
                 std::string& error) {
    std::error_code directoryError;
    std::filesystem::create_directories(path.parent_path(), directoryError);
    if (directoryError) {
        error = "Impossible de créer le dossier de sauvegarde des rappels : " +
                directoryError.message();
        return false;
    }

    std::ofstream file(path, std::ios::trunc);
    if (!file) {
        error = "Impossible d'ouvrir le fichier des rappels en écriture.";
        return false;
    }

    for (const Rappel& rappel : rappels) {
        file << rappel.id << ' ' << std::quoted(rappel.text) << '\n';
    }
    file.flush();
    if (!file) {
        error = "Impossible d'enregistrer les rappels.";
        return false;
    }
    return true;
}

std::string trim(std::string text) {
    const auto first = std::find_if_not(text.begin(), text.end(), [](unsigned char ch) {
        return std::isspace(ch);
    });
    const auto last = std::find_if_not(text.rbegin(), text.rend(), [](unsigned char ch) {
        return std::isspace(ch);
    }).base();
    if (first >= last) {
        return {};
    }
    return std::string(first, last);
}

std::string reminderText(const std::string& input) {
    const std::string normalized = normalizeText(input);
    const std::vector<std::string> prefixes = {
        "rappelle moi de", "rappele moi de", "rapelle moi de",
        "rappelle moi d", "rappele moi d", "rapelle moi d",
        "souviens toi de", "souvient toi de",
        "rappelle moi", "rappele moi", "rapelle moi",
        "souviens toi", "souvient toi"
    };

    for (const std::string& prefix : prefixes) {
        if (normalized.compare(0, prefix.size(), prefix) != 0 ||
            (normalized.size() > prefix.size() && normalized[prefix.size()] != ' ')) {
            continue;
        }

        for (std::size_t end = 1; end <= input.size(); ++end) {
            if (normalizeText(input.substr(0, end)) != prefix) {
                continue;
            }

            std::string text = input.substr(end);
            const auto content = std::find_if_not(text.begin(), text.end(), [](unsigned char ch) {
                return std::isspace(ch) || ch == ':' || ch == ',' || ch == '-' || ch == '\'';
            });
            text.erase(text.begin(), content);
            return trim(std::move(text));
        }
    }
    return {};
}

std::string formatRappels(const std::vector<Rappel>& rappels) {
    if (rappels.empty()) {
        return "Tu n'as aucun rappel enregistré.";
    }

    std::ostringstream output;
    output << "Voici tes rappels :";
    for (const Rappel& rappel : rappels) {
        output << "\n" << rappel.id << ". " << rappel.text;
    }
    return output.str();
}

bool startsWithPhrase(const std::string& text, const std::string& phrase) {
    return text.compare(0, phrase.size(), phrase) == 0 &&
           (text.size() == phrase.size() || text[phrase.size()] == ' ');
}

} // namespace

std::string RappelService::handleRequest(const std::string& input) const {
    const std::string normalized = normalizeText(input);
    const bool listRequest =
        startsWithPhrase(normalized, "liste") ||
        startsWithPhrase(normalized, "affiche") ||
        startsWithPhrase(normalized, "mes rappels") ||
        startsWithPhrase(normalized, "quels sont mes rappels");
    const bool deleteRequest =
        startsWithPhrase(normalized, "supprime") ||
        startsWithPhrase(normalized, "supprimer") ||
        startsWithPhrase(normalized, "efface") ||
        startsWithPhrase(normalized, "retire");

    std::filesystem::path path;
    std::string error;
    if (!getStoragePath(path, error)) {
        return error;
    }

    std::vector<Rappel> rappels;
    if (!loadRappels(path, rappels, error)) {
        return error;
    }

    if (listRequest) {
        return formatRappels(rappels);
    }

    if (deleteRequest) {
        const auto number = std::find_if(normalized.begin(), normalized.end(), [](unsigned char ch) {
            return std::isdigit(ch);
        });
        if (number == normalized.end()) {
            return "Indique le numéro du rappel à supprimer, par exemple : supprime le rappel 2.";
        }

        std::size_t id = 0;
        const char* first = &*number;
        const char* last = normalized.data() + normalized.size();
        const auto parsed = std::from_chars(first, last, id);
        if (parsed.ec != std::errc{} || parsed.ptr == first) {
            return "Le numéro du rappel est invalide.";
        }

        const auto rappel = std::find_if(rappels.begin(), rappels.end(), [id](const Rappel& item) {
            return item.id == id;
        });
        if (rappel == rappels.end()) {
            return "Aucun rappel ne porte le numéro " + std::to_string(id) + ".";
        }

        rappels.erase(rappel);
        if (!saveRappels(path, rappels, error)) {
            return error;
        }
        return "Le rappel " + std::to_string(id) + " a été supprimé.";
    }

    const std::string text = reminderText(input);
    if (text.empty()) {
        return "Que dois-je te rappeler ? Essaie : rappelle-moi d'acheter du pain.";
    }

    std::size_t nextId = 1;
    for (const Rappel& rappel : rappels) {
        if (rappel.id >= nextId) {
            if (rappel.id == static_cast<std::size_t>(-1)) {
                return "Impossible de créer un nouveau rappel : tous les numéros sont utilisés.";
            }
            nextId = rappel.id + 1;
        }
    }
    rappels.push_back({nextId, text});
    if (!saveRappels(path, rappels, error)) {
        return error;
    }

    return "D'accord, je te rappellerai : " + text + " (rappel " +
           std::to_string(nextId) + ").";
}

} // namespace chatia
