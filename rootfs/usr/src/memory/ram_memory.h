#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <system_error>
#include <vector>

struct RamInfo {
    std::uint64_t totaleKo;
    std::uint64_t disponibleKo;

    std::uint64_t utiliseeKo() const {
        return totaleKo - disponibleKo;
    }
};

struct ProcessusRAM {
    std::string nom;
    int pid;
    std::uint64_t ramKo;
};

inline RamInfo lireRAM() {
    std::ifstream fichier("/proc/meminfo");
    if (!fichier) {
        throw std::runtime_error("Impossible de lire /proc/meminfo");
    }

    std::uint64_t total = 0;
    std::uint64_t disponible = 0;
    std::string ligne;
    while (std::getline(fichier, ligne)) {
        std::istringstream champs(ligne);
        std::string etiquette;
        std::uint64_t valeur = 0;
        if (!(champs >> etiquette >> valeur)) {
            continue;
        }
        if (etiquette == "MemTotal:") {
            total = valeur;
        } else if (etiquette == "MemAvailable:") {
            disponible = valeur;
        }
    }

    if (total == 0 || disponible > total) {
        throw std::runtime_error("Valeurs RAM absentes ou invalides dans /proc/meminfo");
    }
    return {total, disponible};
}

inline void afficherRAM() {
    const RamInfo ram = lireRAM();
    std::cout << "RAM totale     : " << ram.totaleKo / 1024 << " Mo\n";
    std::cout << "RAM disponible : " << ram.disponibleKo / 1024 << " Mo\n";
    std::cout << "RAM utilisee   : " << ram.utiliseeKo() / 1024 << " Mo\n";
}

struct MesureCpu {
    long long inactif;
    long long total;
};

inline MesureCpu lireCpu() {
    std::ifstream f("/proc/stat");
    std::string etiquette;
    long long u = 0, n = 0, s = 0, i = 0, io = 0;
    long long irq = 0, sirq = 0, st = 0;
    if (!(f >> etiquette >> u >> n >> s >> i >> io >> irq >> sirq >> st)) {
        throw std::runtime_error("Impossible de lire les statistiques CPU dans /proc/stat");
    }
    return { i + io, u + n + s + i + io + irq + sirq + st };
}

inline void afficherUsageCpu() {
    MesureCpu a = lireCpu();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    MesureCpu b = lireCpu();

    const long long differenceTotale = b.total - a.total;
    const long long differenceInactive = b.inactif - a.inactif;
    if (differenceTotale <= 0) {
        throw std::runtime_error("Impossible de calculer la variation CPU");
    }

    std::cout << "Usage CPU      : "
              << 100.0 * (1.0 - static_cast<double>(differenceInactive) /
                                    static_cast<double>(differenceTotale))
              << " %\n";
}
inline std::vector<ProcessusRAM> lireProcessusRAM() {
    std::vector<ProcessusRAM> processus;
    std::error_code erreur;
    const std::filesystem::directory_iterator fin;
    std::filesystem::directory_iterator entree("/proc", erreur);
    if (erreur) {
        throw std::runtime_error("Impossible de lire /proc : " + erreur.message());
    }

    while (entree != fin) {
        const std::string pid = entree->path().filename().string();
        if (!pid.empty() && std::all_of(pid.begin(), pid.end(), [](unsigned char c) {
                return c >= '0' && c <= '9';
            })) {
            std::ifstream status(entree->path() / "status");
            std::string nom;
            std::string ligne;
            std::uint64_t ramKo = 0;
            while (std::getline(status, ligne)) {
                if (ligne.rfind("Name:", 0) == 0) {
                    nom = ligne.substr(5);
                    const auto debut = nom.find_first_not_of(" \t");
                    nom = debut == std::string::npos ? "" : nom.substr(debut);
                } else if (ligne.rfind("VmRSS:", 0) == 0) {
                    std::istringstream champs(ligne.substr(6));
                    std::string unite;
                    champs >> ramKo >> unite;
                }
            }

            if (ramKo > 0) {
                std::error_code erreurLien;
                const auto executable = std::filesystem::read_symlink(
                    entree->path() / "exe", erreurLien);
                if (!erreurLien && !executable.filename().empty()) {
                    nom = executable.filename().string();
                }
                if (nom.empty()) {
                    nom = "inconnu";
                }
                processus.push_back({nom, std::stoi(pid), ramKo});
            }
        }

        entree.increment(erreur);
        if (erreur) {
            throw std::runtime_error("Erreur pendant la lecture de /proc : " +
                                     erreur.message());
        }
    }

    std::sort(processus.begin(), processus.end(),
              [](const ProcessusRAM& a, const ProcessusRAM& b) {
                  return a.ramKo > b.ramKo;
              });
    return processus;
}

inline void affichesapplication() {
    const std::vector<ProcessusRAM> processus = lireProcessusRAM();
    std::cout << "\nApplications et processus utilisant la RAM :\n";
    if (processus.empty()) {
        std::cout << "Aucun processus avec une consommation RAM lisible.\n";
        return;
    }

    std::cout << std::left << std::setw(30) << "Application"
              << std::right << std::setw(8) << "PID"
              << std::setw(14) << "RAM (Mo)\n";
    for (const ProcessusRAM& p : processus) {
        std::cout << std::left << std::setw(30) << p.nom
                  << std::right << std::setw(8) << p.pid
                  << std::setw(14) << std::fixed << std::setprecision(1)
                  << p.ramKo / 1024.0 << '\n';
    }
}