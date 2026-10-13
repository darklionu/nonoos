#include "isodetection.h"
#include "usb_comminicate.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

static void afficher_usage(const char* programme)
{
    std::cerr << "Usage:\n"
              << "  " << programme << " --list\n"
              << "  " << programme << " --scan <repertoire>\n"
              << "  " << programme << " VID PID ISO [BUS ADRESSE]\n"
              << "\n"
              << "Exemples:\n"
              << "  " << programme << " --list\n"
              << "  " << programme << " --scan /images\n"
              << "  " << programme << " 0x1234 0x5678 /images/system.iso\n"
              << "  " << programme << " 0x1234 0x5678 /images/system.iso 2 7\n";
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        afficher_usage(argv[0]);
        return 2;
    }

    std::string commande = argv[1];

    if (commande == "--list") {
        libusb_context* contexte = nullptr;
        int resultat = libusb_init(&contexte);
        if (resultat < 0) {
            std::cerr << "Erreur d'initialisation de libusb : "
                      << libusb_error_name(resultat) << '\n';
            return 1;
        }

        int code = lister_peripheriques(contexte);
        libusb_exit(contexte);
        return code;
    }

    if (commande == "--scan") {
        if (argc != 3) {
            afficher_usage(argv[0]);
            return 2;
        }

        std::filesystem::path repertoire = argv[2];
        auto isos = iso::scanner_isos(repertoire);

        if (isos.empty()) {
            std::cout << "Aucune image ISO valide trouvée dans : "
                      << repertoire << '\n';
            return 1;
        }

        for (const auto& iso : isos) {
            std::cout << "ISO valide : " << iso << '\n';
        }
        return 0;
    }

    if (argc != 4 && argc != 6) {
        afficher_usage(argv[0]);
        return 2;
    }

    unsigned long vid = 0;
    unsigned long pid = 0;
    if (!lire_nombre(argv[1], 0xFFFF, vid) || !lire_nombre(argv[2], 0xFFFF, pid)) {
        std::cerr << "VID ou PID invalide\n";
        return 2;
    }

    std::filesystem::path image = argv[3];
    if (!iso::est_fichier_iso(image)) {
        std::cerr << "Le fichier n'est pas une ISO valide : " << image << '\n';
        return 2;
    }

    int bus = -1;
    int adresse = -1;
    if (argc == 6) {
        unsigned long bus_lu = 0;
        unsigned long adresse_lue = 0;
        if (!lire_nombre(argv[4], 255, bus_lu) || !lire_nombre(argv[5], 255, adresse_lue)) {
            std::cerr << "Bus ou adresse invalide\n";
            return 2;
        }
        bus = static_cast<int>(bus_lu);
        adresse = static_cast<int>(adresse_lue);
    }

    return envoyer_fichier(static_cast<uint16_t>(vid),
                           static_cast<uint16_t>(pid),
                           image.string(),
                           bus,
                           adresse);
}
