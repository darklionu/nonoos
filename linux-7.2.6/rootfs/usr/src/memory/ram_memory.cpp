#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <sys/statvfs.h>
void afficherRAM() {
    std::ifstream f("/proc/meminfo");
    std::string ligne;
    long total = 0, dispo = 0;

    while (std::getline(f, ligne)) {
        if (ligne.rfind("MemTotal", 0) == 0)
            total = std::stol(ligne.substr(10));
        if (ligne.rfind("MemAvailable", 0) == 0)
            dispo = std::stol(ligne.substr(13));
    }

    std::cout << "RAM totale     : " << total / 1024 << " Mo\n";
    std::cout << "RAM disponible : " << dispo / 1024 << " Mo\n";
    std::cout << "RAM utilisee   : " << (total - dispo) / 1024 << " Mo\n";
}

struct MesureCpu {
    long long inactif;
    long long total;
};

MesureCpu lireCpu() {
    std::ifstream f("/proc/stat");
    std::string etiquette;
    long long u, n, s, i, io, irq, sirq, st;
    f >> etiquette >> u >> n >> s >> i >> io >> irq >> sirq >> st;
    return { i + io, u + n + s + i + io + irq + sirq + st };
}

void afficherUsageCpu() {
    MesureCpu a = lireCpu();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    MesureCpu b = lireCpu();

    double dTotal = b.total - a.total;
    double dInactif = b.inactif - a.inactif;

    std::cout << "Usage CPU      : "
              << 100.0 * (1.0 - dInactif / dTotal) << " %\n";
}

int main() {
    afficherRAM();
    afficherUsageCpu();
    return 0;
}