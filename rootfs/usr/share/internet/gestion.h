#ifndef GESTION_H
#define GESTION_H

#define WIFI_INTERFACE_NAME_SIZE 16

#ifdef __cplusplus
extern "C" {
#endif

extern char interface_wifi[WIFI_INTERFACE_NAME_SIZE];

/* Retourne 1 si une interface Wi-Fi est trouvee, 0 sinon, -1 en cas d'erreur. */
int detecter_carte_wifi(void);

#ifdef __cplusplus
}
#endif

#endif
