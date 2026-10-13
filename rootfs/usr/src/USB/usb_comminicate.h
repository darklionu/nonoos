#ifndef USB_COMMUNICATE_H
#define USB_COMMUNICATE_H

#include "usb_communication.h"
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

#endif // USB_COMMUNICATE_H