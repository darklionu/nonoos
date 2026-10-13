#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <libusb-1.0/libusb.h>
#include <string>
#include <vector>

struct CibleUsb {
    libusb_device *peripherique;
    int interface_numero;
    int alternatif;
    unsigned char endpoint_sortie;
};

int lister_peripheriques(libusb_context *contexte) {
    libusb_device **liste = nullptr;
    ssize_t nombre = libusb_get_device_list(contexte, &liste);
    if (nombre < 0) {
        std::cerr << "Impossible de lister les périphériques USB\n";
        return 1;
    }

    for (ssize_t i = 0; i < nombre; ++i) {
        libusb_device_descriptor desc{};
        if (libusb_get_device_descriptor(liste[i], &desc) == 0) {
            std::cout << "VID=0x" << std::hex << desc.idVendor
                      << " PID=0x" << desc.idProduct << std::dec
                      << " bus=" << static_cast<unsigned int>(libusb_get_bus_number(liste[i]))
                      << " adresse=" << static_cast<unsigned int>(libusb_get_device_address(liste[i]))
                      << '\n';
        }
    }

    libusb_free_device_list(liste, 1);
    return 0;
}

int envoyer_fichier(uint16_t vid_cible, uint16_t pid_cible, const std::string &chemin,
                    int bus_cible = -1, int adresse_cible = -1) {
    std::ifstream fichier(chemin, std::ios::binary);
    if (!fichier) {
        std::cerr << "Impossible d'ouvrir le fichier : " << chemin << '\n';
        return 1;
    }

    libusb_context *contexte = nullptr;
    int resultat = libusb_init(&contexte);
    if (resultat < 0) {
        std::cerr << "Erreur d'initialisation de libusb : " << libusb_error_name(resultat) << '\n';
        return 1;
    }

    libusb_device **liste = nullptr;
    ssize_t nombre = libusb_get_device_list(contexte, &liste);
    if (nombre < 0) {
        std::cerr << "Impossible de lister les périphériques USB\n";
        libusb_exit(contexte);
        return 1;
    }

    std::vector<CibleUsb> cibles;
    for (ssize_t i = 0; i < nombre; ++i) {
        libusb_device *peripherique = liste[i];
        libusb_device_descriptor desc{};
        if (libusb_get_device_descriptor(peripherique, &desc) < 0 ||
            desc.idVendor != vid_cible || desc.idProduct != pid_cible ||
            (bus_cible >= 0 && libusb_get_bus_number(peripherique) != bus_cible) ||
            (adresse_cible >= 0 && libusb_get_device_address(peripherique) != adresse_cible)) {
            continue;
        }

        libusb_config_descriptor *config = nullptr;
        if (libusb_get_config_descriptor(peripherique, 0, &config) < 0) {
            continue;
        }

        bool cible_trouvee = false;
        for (uint8_t j = 0; j < config->bNumInterfaces && !cible_trouvee; ++j) {
            const libusb_interface &interface_desc = config->interface[j];
            for (int k = 0; k < interface_desc.num_altsetting && !cible_trouvee; ++k) {
                const libusb_interface_descriptor &altsetting = interface_desc.altsetting[k];
                unsigned char endpoint_sortie = 0;
                bool endpoint_entree = false;

                for (uint8_t endpoint = 0; endpoint < altsetting.bNumEndpoints; ++endpoint) {
                    const libusb_endpoint_descriptor &desc_endpoint = altsetting.endpoint[endpoint];
                    if ((desc_endpoint.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) != LIBUSB_TRANSFER_TYPE_BULK) {
                        continue;
                    }
                    if ((desc_endpoint.bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT &&
                        endpoint_sortie == 0) {
                        endpoint_sortie = desc_endpoint.bEndpointAddress;
                    } else if ((desc_endpoint.bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN) {
                        endpoint_entree = true;
                    }
                }

                if (endpoint_sortie != 0 && endpoint_entree) {
                    cibles.push_back({peripherique, altsetting.bInterfaceNumber,
                                      altsetting.bAlternateSetting, endpoint_sortie});
                    cible_trouvee = true;
                }
            }
        }
        libusb_free_config_descriptor(config);
    }

    if (cibles.size() != 1) {
        if (cibles.empty()) {
            std::cerr << "Aucun appareil correspondant avec des endpoints bulk n'a été trouvé.\n";
        } else {
            std::cerr << "Plusieurs appareils correspondent. Relancez avec le bus et l'adresse affichés par --list.\n";
            for (const CibleUsb &cible : cibles) {
                std::cerr << "bus=" << static_cast<unsigned int>(libusb_get_bus_number(cible.peripherique))
                          << " adresse=" << static_cast<unsigned int>(libusb_get_device_address(cible.peripherique))
                          << '\n';
            }
        }
        libusb_free_device_list(liste, 1);
        libusb_exit(contexte);
        return 1;
    }

    CibleUsb cible = cibles.front();
    libusb_device_handle *poignee = nullptr;
    resultat = libusb_open(cible.peripherique, &poignee);
    libusb_free_device_list(liste, 1);
    if (resultat < 0) {
        std::cerr << "Impossible d'ouvrir l'appareil USB : " << libusb_error_name(resultat) << '\n';
        libusb_exit(contexte);
        return 1;
    }

    resultat = libusb_claim_interface(poignee, cible.interface_numero);
    if (resultat < 0) {
        std::cerr << "Impossible de revendiquer l'interface : " << libusb_error_name(resultat) << '\n';
        libusb_close(poignee);
        libusb_exit(contexte);
        return 1;
    }

    if (cible.alternatif != 0) {
        resultat = libusb_set_interface_alt_setting(poignee, cible.interface_numero, cible.alternatif);
        if (resultat < 0) {
            std::cerr << "Impossible de sélectionner l'alternative d'interface : "
                      << libusb_error_name(resultat) << '\n';
            libusb_release_interface(poignee, cible.interface_numero);
            libusb_close(poignee);
            libusb_exit(contexte);
            return 1;
        }
    }

    std::array<unsigned char, 16 * 1024> tampon{};
    uint64_t total_envoye = 0;
    bool erreur_envoi = false;
    while (fichier) {
        fichier.read(reinterpret_cast<char *>(tampon.data()), tampon.size());
        std::streamsize taille_lue = fichier.gcount();
        std::streamsize position = 0;

        while (position < taille_lue) {
            int transfert = 0;
            resultat = libusb_bulk_transfer(poignee, cible.endpoint_sortie,
                                            tampon.data() + position,
                                            static_cast<int>(taille_lue - position),
                                            &transfert, 5000);
            if (resultat < 0 || transfert <= 0) {
                std::cerr << "Erreur pendant l'envoi : "
                          << (resultat < 0 ? libusb_error_name(resultat) : "transfert vide") << '\n';
                erreur_envoi = true;
                break;
            }
            position += transfert;
            total_envoye += static_cast<uint64_t>(transfert);
        }
        if (erreur_envoi) {
            break;
        }
    }

    if (fichier.bad()) {
        std::cerr << "Erreur de lecture du fichier\n";
        erreur_envoi = true;
    }

    if (!erreur_envoi) {
        std::cout << total_envoye << " octets transmis depuis " << chemin << '\n';
    }
    libusb_release_interface(poignee, cible.interface_numero);
    libusb_close(poignee);
    libusb_exit(contexte);
    return erreur_envoi ? 1 : 0;
}
