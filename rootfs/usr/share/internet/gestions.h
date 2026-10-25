#include "gestion.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

char interface_wifi[WIFI_INTERFACE_NAME_SIZE];

static int has_sysfs_marker(const char *interface_name, const char *marker)
{
    char path[256];
    struct stat info;
    int length = snprintf(path, sizeof(path), "/sys/class/net/%s/%s",
                          interface_name, marker);

    if (length < 0 || (size_t)length >= sizeof(path)) {
        fprintf(stderr, "Chemin sysfs trop long pour %s\n", interface_name);
        return -1;
    }

    if (stat(path, &info) == 0)
        return S_ISDIR(info.st_mode);

    if (errno == ENOENT)
        return 0;

    perror(path);
    return -1;
}

static int is_wifi_interface(const char *interface_name)
{
    int result = has_sysfs_marker(interface_name, "wireless");

    if (result != 0)
        return result;

    return has_sysfs_marker(interface_name, "phy80211");
}

int detecter_carte_wifi(void)
{
    DIR *interfaces = opendir("/sys/class/net");
    struct dirent *entry;

    interface_wifi[0] = '\0';

    if (interfaces == NULL) {
        perror("Impossible de lire /sys/class/net");
        return -1;
    }

    while ((entry = readdir(interfaces)) != NULL) {
        int is_wifi;

        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        is_wifi = is_wifi_interface(entry->d_name);
        if (is_wifi < 0) {
            closedir(interfaces);
            return -1;
        }

        if (is_wifi) {
            size_t name_length = strlen(entry->d_name);

            if (name_length >= sizeof(interface_wifi)) {
                fprintf(stderr, "Nom d'interface Wi-Fi trop long : %s\n",
                        entry->d_name);
                closedir(interfaces);
                return -1;
            }

            memcpy(interface_wifi, entry->d_name, name_length + 1);
            closedir(interfaces);
            return 1;
        }
    }

    closedir(interfaces);
    return 0;
}
