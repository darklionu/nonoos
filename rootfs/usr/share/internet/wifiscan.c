#include "wifiscan.h"
#include <string.h>

WifiScanResult g_wifi_scan_result = {0};

void wifi_scan_reset(void)
{
    memset(&g_wifi_scan_result, 0, sizeof(g_wifi_scan_result));
}

int wifi_scan_networks(void)
{
    wifi_scan_reset();

    // Exemple de remplissage fictif
    // Remplace ceci par ton scan réel
    strcpy(g_wifi_scan_result.networks[0].ssid, "Freebox");
    strcpy(g_wifi_scan_result.networks[0].bssid, "AA:BB:CC:DD:EE:FF");
    g_wifi_scan_result.networks[0].signal_dbm = -55;
    g_wifi_scan_result.networks[0].channel = 6;

    strcpy(g_wifi_scan_result.networks[1].ssid, "OrangeWifi");
    strcpy(g_wifi_scan_result.networks[1].bssid, "11:22:33:44:55:66");
    g_wifi_scan_result.networks[1].signal_dbm = -68;
    g_wifi_scan_result.networks[1].channel = 11;

    g_wifi_scan_result.count = 2;
    return 0;
}