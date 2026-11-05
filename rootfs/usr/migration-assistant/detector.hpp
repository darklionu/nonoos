#pragma once
#include <string>
#include <vector>

struct Partition {
    std::string device;
    std::string fstype;
    std::string mountpoint;
    std::string label;
};

struct InstalledOS {
    std::string name;
    std::string version;
    std::string device;
};

struct DetectionReport {
    std::vector<InstalledOS> systems;
    std::vector<std::string> errors;
};

struct FileListReport {
    std::vector<std::string> files;
    std::vector<std::string> errors;
};

// Inventorie les partitions détectables sans modifier leur contenu.
std::vector<Partition> detect_partitions();

// Nécessite les privilèges root pour monter les partitions non montées.
// Les échecs individuels sont conservés dans le rapport.
DetectionReport detect_installed_os();

// Liste les fichiers ordinaires sous root avec des chemins relatifs à root.
// Les liens symboliques ne sont pas suivis et les erreurs sont conservées dans le rapport.
// Si extensions est vide, retourne tous les fichiers ordinaires.
// Les extensions peuvent être fournies avec ou sans point et sont comparées
// sans distinction entre majuscules et minuscules.
FileListReport list_files(
    const std::string& root,
    const std::vector<std::string>& extensions = {});

// Monte une partition dans un dossier temporaire, en lecture seule.
std::string mount_partition_readonly(const Partition& partition);

// Démonte la partition puis supprime le dossier temporaire.
void unmount_partition(const std::string& mountpoint);