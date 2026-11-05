#pragma once

#include <filesystem>
#include <string>
#include <vector>

// Retourne true si le fichier possède l'une des extensions autorisées.
// Une liste vide autorise toutes les extensions.
bool has_allowed_extension(
    const std::filesystem::path& file,
    const std::vector<std::string>& extensions);

