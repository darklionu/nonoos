#!/usr/bin/env bash
set -Eeuo pipefail

if [[ "$EUID" -eq 0 ]]; then
    printf '%s\n' "Erreur : le shell de test refuse de s’exécuter en tant que root." >&2
    exit 1
fi

if [[ ! -t 0 || ! -t 1 ]]; then
    printf '%s\n' "Erreur : le bac à sable exige un terminal interactif." >&2
    exit 1
fi

if ! command -v bwrap >/dev/null 2>&1; then
    printf '%s\n' "Erreur : Bubblewrap est absent ; aucun shell non isolé ne sera lancé." >&2
    exit 1
fi

if [[ ! "$HOME" =~ ^/home/[a-z_][a-z0-9_-]{0,31}$ ||
      ! -d "$HOME" || ! -O "$HOME" ]]; then
    printf '%s\n' "Erreur : dossier personnel de test absent ou non possédé par l’utilisateur." >&2
    exit 1
fi

uid=$(id -u)
gid=$(id -g)

# Le système hôte est en lecture seule ; seul le dossier personnel du compte
# est persistant. Les autres espaces inscriptibles sont temporaires et privés.
exec bwrap \
    --die-with-parent \
    --new-session \
    --unshare-all \
    --unshare-user \
    --disable-userns \
    --cap-drop ALL \
    --uid "$uid" \
    --gid "$gid" \
    --hostname nonoos-test \
    --ro-bind / / \
    --proc /proc \
    --dev /dev \
    --tmpfs /tmp \
    --tmpfs /run \
    --tmpfs /var/tmp \
    --tmpfs /home \
    --dir "$HOME" \
    --bind "$HOME" "$HOME" \
    --chdir "$HOME" \
    --clearenv \
    --setenv HOME "$HOME" \
    --setenv USER "$(id -un)" \
    --setenv LOGNAME "$(id -un)" \
    --setenv SHELL /bin/bash \
    --setenv PATH /usr/local/bin:/usr/bin:/bin:/usr/local/sbin:/usr/sbin:/sbin \
    --setenv TERM "${TERM:-dumb}" \
    -- /bin/bash --noprofile --norc -i
