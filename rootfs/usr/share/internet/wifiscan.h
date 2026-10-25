#ifndef WIFI_SCAN_H
#define WIFI_SCAN_H

#include <stddef.h>

#define WIFI_SSID_LEN 33
#define WIFI_BSSID_LEN 18
#define WIFI_MAX_NETWORKS 64

typedef struct {
    char ssid[WIFI_SSID_LEN];
    char bssid[WIFI_BSSID_LEN];
    int signal_dbm;
    int channel;
} WifiNetwork;

typedef struct {
    WifiNetwork networks[WIFI_MAX_NETWORKS];
    size_t count;
} WifiScanResult;

// Variable globale unique contenant tous les réseaux trouvés
// et le nombre de réseaux détectés
extern WifiScanResult g_wifi_scan_result;

// Initialise la structure de résultat
void wifi_scan_reset(void);

// Lance le scan Wi‑Fi et remplit g_wifi_scan_result
int wifi_scan_networks(void);

#endif