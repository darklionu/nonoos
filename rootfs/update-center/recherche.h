/*
 * Le serveur doit exposer un fichier texte contenant une version X.Y.Z.
 */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#ifndef VERSION_ACTUELLE
#define VERSION_ACTUELLE "1.2.0"
#endif

#ifndef URL_VERSION
#define URL_VERSION      "https://exemple.com/version.txt"
#endif

struct Buffer {
    char *data;
    size_t taille;
};

static size_t ecrire_reponse(void *contenu, size_t size, size_t nmemb, void *userp)
{
    size_t total = size * nmemb;
    struct Buffer *buf = (struct Buffer *)userp;

    if (total > 1024 - buf->taille) return 0;

    char *tmp = (char *)realloc(buf->data, buf->taille + total + 1);
    if (!tmp) return 0; /* erreur mémoire : curl s'arrête */

    buf->data = tmp;
    memcpy(buf->data + buf->taille, contenu, total);
    buf->taille += total;
    buf->data[buf->taille] = '\0';
    return total;
}

static int analyser_version(const char *version, unsigned long valeurs[3])
{
    const char *curseur = version;

    for (int i = 0; i < 3; ++i) {
        if (!isdigit((unsigned char)*curseur)) return 0;

        errno = 0;
        char *fin = NULL;
        valeurs[i] = strtoul(curseur, &fin, 10);
        if (errno == ERANGE || fin == curseur) return 0;

        if (i < 2) {
            if (*fin != '.') return 0;
            curseur = fin + 1;
        } else if (*fin != '\0') {
            return 0;
        }
    }

    return 1;
}

static int comparer_versions(const char *a, const char *b)
{
    unsigned long va[3] = {0, 0, 0};
    unsigned long vb[3] = {0, 0, 0};

    if (!analyser_version(a, va) || !analyser_version(b, vb)) return 0;

    for (int i = 0; i < 3; ++i) {
        if (va[i] > vb[i]) return 1;
        if (va[i] < vb[i]) return -1;
    }

    return 0;
}

/* Retourne 1 si une mise à jour existe, 0 si à jour, -1 en cas d'erreur. */
static int verifier_mise_a_jour(const char *url_version,
                                char *version_distante,
                                size_t taille_max)
{
    if (!url_version || !*url_version || !version_distante || taille_max == 0) {
        return -1;
    }

    CURL *curl = curl_easy_init();
    if (!curl) return -1;

    struct Buffer buf = { NULL, 0 };

    curl_easy_setopt(curl, CURLOPT_URL, url_version);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ecrire_reponse);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "MonApp/" VERSION_ACTUELLE);

    CURLcode res = curl_easy_perform(curl);
    long code_http = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code_http);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || code_http != 200 || !buf.data) {
        free(buf.data);
        return -1;
    }

    char *debut = buf.data;
    while (isspace((unsigned char)*debut)) ++debut;
    char *fin = debut + strlen(debut);
    while (fin > debut && isspace((unsigned char)fin[-1])) --fin;
    *fin = '\0';

    unsigned long version_lue[3] = {0, 0, 0};
    unsigned long version_actuelle[3] = {0, 0, 0};
    size_t longueur = strlen(debut);
    if (longueur == 0 || longueur >= taille_max ||
        !analyser_version(debut, version_lue) ||
        !analyser_version(VERSION_ACTUELLE, version_actuelle)) {
        free(buf.data);
        return -1;
    }

    memcpy(version_distante, debut, longueur + 1);
    free(buf.data);

    return comparer_versions(version_distante, VERSION_ACTUELLE) > 0 ? 1 : 0;
}