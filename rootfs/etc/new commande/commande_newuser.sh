#!/usr/bin/env bash
set -Eeuo pipefail

readonly TEST_SHELL=/usr/local/bin/nonoos-test-shell
username=${1:-nonoos-test}

if [[ "$EUID" -ne 0 ]]; then
    printf '%s\n' "Erreur : lancez ce script avec sudo ou en tant que root." >&2
    exit 1
fi

if (($# > 1)) || [[ ! "$username" =~ ^[a-z_][a-z0-9_-]{0,31}$ ]]; then
    printf 'Usage : %s [nom-utilisateur]\n' "$0" >&2
    exit 2
fi

if [[ ! -x "$TEST_SHELL" ]]; then
    printf 'Erreur : lanceur de bac à sable introuvable : %s\n' "$TEST_SHELL" >&2
    exit 1
fi

if ! command -v bwrap >/dev/null 2>&1; then
    printf '%s\n' "Erreur : Bubblewrap n'est pas installé ; aucun compte n'a été créé." >&2
    exit 1
fi

if ! command -v useradd >/dev/null 2>&1 || ! command -v passwd >/dev/null 2>&1; then
    printf '%s\n' "Erreur : useradd et passwd sont requis pour créer le compte." >&2
    exit 1
fi

if getent passwd "$username" >/dev/null; then
    printf 'Erreur : le compte %s existe déjà ; il n’a pas été modifié.\n' "$username" >&2
    exit 1
fi

if [[ ! -t 0 ]]; then
    printf '%s\n' "Erreur : exécutez ce script depuis un terminal pour définir le mot de passe." >&2
    exit 1
fi

useradd --create-home --home-dir "/home/$username" --user-group \
    --shell "$TEST_SHELL" "$username"
chmod 0700 "/home/$username"

if ! passwd "$username"; then
    passwd --lock "$username"
    printf 'Erreur : mot de passe non configuré ; le compte %s reste verrouillé.\n' \
        "$username" >&2
    exit 1
fi

printf '\nCompte %s créé sans privilèges administrateur.\n' "$username"
printf '%s\n' \
    "Sa session lance un shell Bubblewrap : système en lecture seule, réseau isolé et fichiers temporaires privés."
printf '%s\n' \
    "Les fichiers conservés dans son dossier personnel restent présents sur le disque."
printf '%s\n' \
    "Cette isolation partage le noyau avec l’hôte ; pour du code réellement hostile, utilisez une machine virtuelle."
