#include <usb_communication.h>
#include <iostream>
#include <string>


bool lire_nombre(const char *texte, unsigned long maximum, unsigned long &valeur) {
    try {
        std::size_t position = 0;
        valeur = std::stoul(texte, &position, 0);
        return position == std::string(texte).size() && valeur <= maximum;
    } catch (...) {
        return false;
    }
}

int main(int argc, char **argv) {
    libusb_context *contexte = nullptr;
    int resultat = libusb_init(&contexte);
    if (resultat < 0) {
        std::cerr << "Erreur d'initialisation de libusb : " << libusb_error_name(resultat) << '\n';
        return 1;
    }

    if (argc == 2 && std::string(argv[1]) == "--list") {
        resultat = lister_peripheriques(contexte);
        libusb_exit(contexte);
        return resultat;
    }
    libusb_exit(contexte);

    if (argc != 4 && argc != 6) {
        std::cerr << "Usage : " << argv[0] << " --list\n"
                  << "   ou : " << argv[0] << " VID PID FICHIER [BUS ADRESSE]\n"
                  << "VID/PID acceptent le format hexadécimal (ex. 0x1234).\n";
        return 2;
    }

    unsigned long vid = 0;
    unsigned long pid = 0;
    if (!lire_nombre(argv[1], 0xFFFF, vid) || !lire_nombre(argv[2], 0xFFFF, pid)) {
        std::cerr << "VID ou PID invalide\n";
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

    return envoyer_fichier(static_cast<uint16_t>(vid), static_cast<uint16_t>(pid),
                           argv[3], bus, adresse);
}