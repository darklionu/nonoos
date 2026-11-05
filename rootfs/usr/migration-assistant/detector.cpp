#include "detector.hpp"
#include "extension_filter.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace {

std::string decode_lsblk_value(const std::string& encoded)
{
    std::string value;
    for (std::size_t i = 0; i < encoded.size(); ++i) {
        if (encoded[i] != '\\') {
            value += encoded[i];
            continue;
        }

        if (i + 3 < encoded.size() && encoded[i + 1] == 'x') {
            // lsblk encode certains caractères spéciaux sous la forme \xNN.
            unsigned int byte = 0;
            std::istringstream hex(encoded.substr(i + 2, 2));
            if (!(hex >> std::hex >> byte)) {
                throw std::runtime_error("Valeur lsblk mal formée");
            }
            value += static_cast<char>(byte);
            i += 3;
        } else if (i + 1 < encoded.size()) {
            value += encoded[++i];
        } else {
            throw std::runtime_error("Valeur lsblk mal formée");
        }
    }
    return value;
}
bool is_readonly_mount(const std::string& mountpoint)
{
    struct statvfs info;
    if (statvfs(mountpoint.c_str(), &info) != 0) {
        throw std::system_error(errno, std::generic_category(), "Impossible de lire les informations du point de montage");
    }
    return (info.f_flag & ST_RDONLY) != 0;
}

std::vector<std::pair<std::string, std::string>> parse_lsblk_line(const std::string& line)
{
    std::vector<std::pair<std::string, std::string>> fields;
    std::size_t position = 0;

    while (position < line.size()) {
        while (position < line.size() && line[position] == ' ') {
            ++position;
        }
        if (position == line.size()) {
            break;
        }

        const auto key_end = line.find('=', position);
        if (key_end == std::string::npos || key_end == position ||
            key_end + 1 >= line.size() || line[key_end + 1] != '"') {
            throw std::runtime_error("Sortie lsblk mal formée");
        }

        const std::string key = line.substr(position, key_end - position);
        position = key_end + 2;
        std::string encoded;
        bool closed = false;
        while (position < line.size()) {
            if (line[position] == '"' &&
                (position == 0 || line[position - 1] != '\\')) {
                ++position;
                closed = true;
                break;
            }
            encoded += line[position++];
        }
        if (!closed) {
            throw std::runtime_error("Sortie lsblk mal formée");
        }

        fields.emplace_back(key, decode_lsblk_value(encoded));
    }
    return fields;
}

std::string field_value(
    const std::vector<std::pair<std::string, std::string>>& fields,
    const std::string& key)
{
    for (const auto& field : fields) {
        if (field.first == key) {
            return field.second;
        }
    }
    return {};
}

std::string read_os_release_value(const std::filesystem::path& file, const std::string& key)
{
    std::ifstream input(file);
    std::string line;
    const std::string prefix = key + "=";

    while (std::getline(input, line)) {
        if (line.compare(0, prefix.size(), prefix) != 0) {
            continue;
        }

        std::string value = line.substr(prefix.size());
        if (value.size() >= 2 &&
            ((value.front() == '"' && value.back() == '"') ||
             (value.front() == '\'' && value.back() == '\''))) {
            value = value.substr(1, value.size() - 2);
            std::string unescaped;
            for (std::size_t i = 0; i < value.size(); ++i) {
                if (value[i] == '\\' && i + 1 < value.size()) {
                    ++i;
                }
                unescaped += value[i];
            }
            return unescaped;
        }
        return value;
    }
    return {};
}

std::string first_nonempty(const std::string& first, const std::string& second)
{
    return first.empty() ? second : first;
}

std::vector<InstalledOS> inspect_mountpoint(
    const std::filesystem::path& root,
    const std::string& device)
{
    std::vector<InstalledOS> systems;
    // os-release peut se trouver dans l'un ou l'autre emplacement selon la distribution.
    auto os_release = root / "etc/os-release";
    std::error_code file_error;
    if (!std::filesystem::is_regular_file(os_release, file_error)) {
        os_release = root / "usr/lib/os-release";
    }

    if (std::filesystem::is_regular_file(os_release, file_error) && !file_error) {
        const auto pretty_name = read_os_release_value(os_release, "PRETTY_NAME");
        const auto id = read_os_release_value(os_release, "ID");
        const auto version = read_os_release_value(os_release, "VERSION_ID");
        const auto name = first_nonempty(pretty_name, id);
        if (!name.empty()) {
            systems.push_back({name, version, device});
        }
    }

    // Ces fichiers servent de marqueurs sans lire le contenu propre aux OS.
    const auto windows_registry = root / "Windows/System32/config/SOFTWARE";
    file_error.clear();
    if (std::filesystem::is_regular_file(windows_registry, file_error) && !file_error) {
        systems.push_back({"Windows", {}, device});
    }

    const auto macos_plist = root / "System/Library/CoreServices/SystemVersion.plist";
    file_error.clear();
    if (std::filesystem::is_regular_file(macos_plist, file_error) && !file_error) {
        systems.push_back({"macOS", {}, device});
    }

    return systems;
}

} // namespace

std::vector<Partition> detect_partitions()
{
    constexpr char command[] =
        "lsblk --pairs --paths --noheadings --output NAME,TYPE,FSTYPE,LABEL,MOUNTPOINT";
    FILE* output = popen(command, "r");
    if (output == nullptr) {
        throw std::system_error(errno, std::generic_category(), "Impossible de lancer lsblk");
    }

    std::vector<Partition> partitions;
    std::array<char, 4096> buffer{};
    while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), output) != nullptr) {
        std::string line(buffer.data());
        if (!line.empty() && line.back() == '\n') {
            line.pop_back();
        }

        const auto fields = parse_lsblk_line(line);
        const auto device = field_value(fields, "NAME");
        const auto type = field_value(fields, "TYPE");
        const auto fstype = field_value(fields, "FSTYPE");
        if (device.empty() || fstype.empty() ||
            // LVM2_member est un conteneur physique, pas un système de fichiers montable.
            fstype == "LVM2_member" ||
            (type != "part" && type != "lvm" && type != "crypt")) {
            continue;
        }

        partitions.push_back({
            device,
            fstype,
            field_value(fields, "MOUNTPOINT"),
            field_value(fields, "LABEL")});
    }
    const int read_error = std::ferror(output) ? errno : 0;
    const int status = pclose(output);

    if (read_error != 0) {
        throw std::system_error(read_error, std::generic_category(), "Erreur de lecture de lsblk");
    }
    if (status == -1) {
        throw std::system_error(errno, std::generic_category(), "Impossible de terminer lsblk");
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        throw std::runtime_error("lsblk a échoué pendant l'inventaire des partitions");
    }

    return partitions;
}

DetectionReport detect_installed_os()
{
    DetectionReport report;
    for (const auto& partition : detect_partitions()) {
        if (partition.fstype == "crypto_LUKS") {
            report.errors.push_back(
                partition.device + ": volume chiffré verrouillé, déverrouillez-le avant l'analyse");
            continue;
        }

        std::string mountpoint;
        bool mounted_here = false;
        try {
            if (partition.mountpoint.empty()) {
                mountpoint = mount_partition_readonly(partition);
                mounted_here = true;
            } else {
                // Ne pas démonter un point de montage qui appartenait déjà au système.
                mountpoint = partition.mountpoint;
            }
            auto systems = inspect_mountpoint(mountpoint, partition.device);
            if (mounted_here) {
                unmount_partition(mountpoint);
                mountpoint.clear();
                mounted_here = false;
            }
            report.systems.insert(
                report.systems.end(),
                std::make_move_iterator(systems.begin()),
                std::make_move_iterator(systems.end()));
        } catch (const std::exception& error) {
            if (mounted_here && !mountpoint.empty()) {
                try {
                    unmount_partition(mountpoint);
                } catch (const std::exception& cleanup_error) {
                    report.errors.push_back(
                        partition.device + ": " + error.what() +
                        " ; démontage impossible: " + cleanup_error.what());
                    continue;
                }
            }
            report.errors.push_back(partition.device + ": " + error.what());
        }
    }
    return report;
}

FileListReport list_files(
    const std::string& root,
    const std::vector<std::string>& extensions)
{
    FileListReport report;
    std::error_code error;
    // Utiliser une racine absolue permet de rendre chaque résultat relatif à cette racine.
    const auto root_path = std::filesystem::absolute(root, error);
    if (error) {
        report.errors.push_back(root + ": " + error.message());
        return report;
    }

    if (!std::filesystem::is_directory(root_path, error)) {
        report.errors.push_back(
            root_path.string() + ": " +
            (error ? error.message() : "le chemin n'est pas un dossier"));
        return report;
    }

    // Une pile évite la récursion C++ profonde sur les arborescences très imbriquées.
    std::vector<std::filesystem::path> directories{root_path};
    while (!directories.empty()) {
        const auto directory = directories.back();
        directories.pop_back();

        std::filesystem::directory_iterator iterator(directory, error);
        if (error) {
            report.errors.push_back(directory.string() + ": " + error.message());
            error.clear();
            continue;
        }

        const std::filesystem::directory_iterator end;
        while (iterator != end) {
            const auto entry_path = iterator->path();
            // symlink_status inspecte le lien lui-même : aucun lien ne peut faire sortir du volume.
            const auto status = iterator->symlink_status(error);
            if (error) {
                report.errors.push_back(entry_path.string() + ": " + error.message());
                error.clear();
            } else if (std::filesystem::is_directory(status)) {
                directories.push_back(entry_path);
            } else if (
                std::filesystem::is_regular_file(status) &&
                has_allowed_extension(entry_path, extensions)) {
                // Ne conserver que les chemins relatifs pour faciliter leur affichage et leur copie ultérieure.
                report.files.push_back(entry_path.lexically_relative(root_path).generic_string());
            }

            iterator.increment(error);
            if (error) {
                // Une erreur dans un dossier ne doit pas empêcher l'inventaire des autres dossiers.
                report.errors.push_back(directory.string() + ": " + error.message());
                error.clear();
                break;
            }
        }
    }

    std::sort(report.files.begin(), report.files.end());
    std::sort(report.errors.begin(), report.errors.end());
    return report;
}

std::string mount_partition_readonly(const Partition& partition)
{
    if (partition.device.empty() || partition.fstype.empty()) {
        throw std::invalid_argument("Périphérique ou système de fichiers invalide");
    }

    std::string template_path = "/tmp/nonoos-os-detect-XXXXXX";
    std::vector<char> writable_path(template_path.begin(), template_path.end());
    writable_path.push_back('\0');
    char* mountpoint = mkdtemp(writable_path.data());
    if (mountpoint == nullptr) {
        throw std::system_error(errno, std::generic_category(), "Impossible de créer le point de montage");
    }

    constexpr unsigned long flags =
        // Évite toute écriture et limite les effets de bord des fichiers montés.
        MS_RDONLY | MS_NOSUID | MS_NODEV | MS_NOEXEC;
    if (mount(
            partition.device.c_str(),
            mountpoint,
            partition.fstype.c_str(),
            flags,
            nullptr) != 0) {
        const int mount_error = errno;
        rmdir(mountpoint);
        throw std::system_error(
            mount_error,
            std::generic_category(),
            "Montage en lecture seule impossible");
    }
    return mountpoint;
}

void unmount_partition(const std::string& mountpoint)
{
    if (mountpoint.empty()) {
        throw std::invalid_argument("Point de montage vide");
    }
    if (umount2(mountpoint.c_str(), 0) != 0) {
        throw std::system_error(errno, std::generic_category(), "Démontage impossible");
    }
    if (rmdir(mountpoint.c_str()) != 0) {
        throw std::system_error(
            errno,
            std::generic_category(),
            "Impossible de supprimer le point de montage");
    }
}