# Dossier internet

Ce dossier contient le module de gestion Wi‑Fi de NonoOS. Il permet de détecter une interface sans fil, de lancer un scan et de conserver les réseaux découverts dans une structure globale dédiée.

## Objet

Le module remplit deux rôles :

- détecter l’interface Wi‑Fi active du système,
- récupérer les réseaux visibles et les stocker pour que l’application cliente puisse les lire facilement.

Le code s’appuie sur le système Linux `/sys/class/net` pour identifier les cartes Wi‑Fi et, selon le cas, sur des APIs de scan réseau (Netlink / nl80211) pour obtenir les SSID détectés.

## Fichiers

- `gestion.h` : API publique du module
- `gestion.c` : implémentation de la détection de carte Wi‑Fi
- `README.md` : documentation du module

## API publique

### Interface détectée

```c
extern char interface_wifi[WIFI_INTERFACE_NAME_SIZE];
int detecter_carte_wifi(void);
```

- `interface_wifi` contient le nom de l’interface Wi‑Fi trouvée.
- `WIFI_INTERFACE_NAME_SIZE` vaut 16.
- `detecter_carte_wifi()` retourne :
  - `1` si une interface Wi‑Fi a été trouvée,
  - `0` si aucune interface n’a été détectée,
  - `-1` si une erreur système est survenue.

### Réseaux détectés

Le module peut stocker les réseaux trouvés dans une variable globale unique, afin de garder l’état complet pour l’application.

```c
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

extern WifiScanResult g_wifi_scan_result;
```

Cette structure permet de conserver :

- tous les réseaux découverts dans `g_wifi_scan_result.networks[]`,
- le nombre total de réseaux dans `g_wifi_scan_result.count`.

## Fonctionnement

Le module parcourt d’abord le dossier :

```bash
/sys/class/net
```

puis vérifie, pour chaque interface réseau, si elle possède les marqueurs Wi‑Fi attendus :

- `wireless`
- `phy80211`

Une fois l’interface trouvée, l’application peut effectuer un scan et remplir `g_wifi_scan_result`.

## Exemple d’utilisation

```c
#include "gestion.h"
#include <stdio.h>

int main(void)
{
    int resultat = detecter_carte_wifi();

    if (resultat == 1) {
        printf("Interface Wi-Fi detectee : %s\n", interface_wifi);
    } else if (resultat == 0) {
        printf("Aucune interface Wi-Fi trouvee.\n");
        return 0;
    } else {
        printf("Erreur lors de la detection.\n");
        return 1;
    }

    // Exemple d'utilisation des resultats du scan
    printf("Nombre de reseaux trouves : %zu\n", g_wifi_scan_result.count);

    for (size_t i = 0; i < g_wifi_scan_result.count; ++i) {
        printf("%zu - %s | %s | signal=%d dBm | canal=%d\n",
               i,
               g_wifi_scan_result.networks[i].ssid,
               g_wifi_scan_result.networks[i].bssid,
               g_wifi_scan_result.networks[i].signal_dbm,
               g_wifi_scan_result.networks[i].channel);
    }

    return 0;
}
```

## Points techniques

- utilisation de `opendir()` / `readdir()` pour parcourir les interfaces réseau,
- accès sur `/sys/class/net` pour identifier les cartes Wi‑Fi,
- stockage des résultats dans une structure globale pour éviter de les perdre entre fonctions,
- validation des erreurs avec `errno`, diagnostics sur `stderr` et contrôles de taille des chaînes.

## Cas d’usage

- vérification automatique d’une carte Wi‑Fi au démarrage,
- affichage d’une liste de réseaux disponibles dans une interface graphique,
- préparation de la configuration du réseau sans fil,
- intégration dans un gestionnaire de connexion ou un outil de diagnostic réseau.

## Remarque

Le module est conçu pour les systèmes Linux modernes et repose sur la structure sysfs fournie par le noyau. Il est compatible avec l’usage d’interfaces sans fil standard exposées dans `/sys/class/net`, ce qui le rend simple à intégrer dans un environnement NonoOS ou un environnement Linux classique.
