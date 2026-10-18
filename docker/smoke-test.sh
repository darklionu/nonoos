#!/bin/sh
set -eu

CHATIA_BIN="${CHATIA_BIN:-/usr/local/bin/chatia}"
MEMORY_BIN="${MEMORY_BIN:-/usr/local/bin/ram-memory}"

chat_output="$(printf 'bonjour\n2 + 2\nusb\n' | "$CHATIA_BIN")"
printf '%s\n' "$chat_output" | grep -Fq "Bonjour !"
printf '%s\n' "$chat_output" | grep -Fq "Le résultat est 4."
if ! printf '%s\n' "$chat_output" | grep -Eq \
    'Aucun périphérique USB connecté|Périphériques USB détectés'; then
    echo "Échec du test de détection USB." >&2
    exit 1
fi
echo "ChatIA : salutations, calcul et détection USB OK."

memory_output="$("$MEMORY_BIN")"
printf '%s\n' "$memory_output" | grep -Eq '^RAM totale[[:space:]]*:[[:space:]]*[0-9]+ Mo$'
printf '%s\n' "$memory_output" | grep -Eq '^RAM disponible[[:space:]]*:[[:space:]]*[0-9]+ Mo$'
printf '%s\n' "$memory_output" | grep -Eq '^RAM utilisee[[:space:]]*:[[:space:]]*[0-9]+ Mo$'
cpu_usage="$(printf '%s\n' "$memory_output" | sed -n \
    's/^Usage CPU[[:space:]]*:[[:space:]]*\([0-9][0-9.]*\) %$/\1/p')"
if [ -z "$cpu_usage" ] || ! awk -v usage="$cpu_usage" \
    'BEGIN { exit !(usage >= 0 && usage <= 100) }'; then
    echo "La mesure CPU est absente ou hors de l’intervalle 0–100 %." >&2
    exit 1
fi
echo "Outil mémoire : mesures RAM et CPU valides."
