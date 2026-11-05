#include "extensions.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

void demander_extensions()
{
    std::cout << "Entrez les extensions a rechercher, separees par des espaces "
                 "(exemple : .txt .md .cpp)."
              << '\n'
              << "Laissez vide pour rechercher tous les fichiers : ";

    std::string ligne;
    std::getline(std::cin, ligne);

    std::istringstream entree(ligne);
    std::vector<std::string> extensions;
    std::string extension;

    while (entree >> extension) {
        extensions.push_back(extension);
    }

    set_extensions(extensions);
}