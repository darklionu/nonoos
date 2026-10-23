#!/usr/bin/env bash
set -Eeuo pipefail
export LC_ALL=C

if [[ ! -t 0 || ! -t 1 ]]; then
    printf '%s\n' "Erreur : le tableau de bord doit être lancé dans un terminal interactif." >&2
    exit 1
fi

for tool in awk date df free ps; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        printf 'Erreur : commande requise introuvable : %s\n' "$tool" >&2
        exit 1
    fi
done

cpu_snapshot() {
    awk '/^cpu / {
        total = 0
        for (i = 2; i <= NF; i++) {
            total += $i
        }
        printf "%.0f %.0f\n", total, $5 + $6
        exit
    }' /proc/stat
}

clear_screen() {
    clear 2>/dev/null || printf '\033[2J\033[H'
}

cleanup() {
    printf '\033[?25h'
    clear_screen
}

trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

last_total=""
last_idle=""

while :; do
    read -r current_total current_idle < <(cpu_snapshot)

    if [[ -n "$last_total" && $current_total -gt $last_total ]]; then
        cpu_usage=$(awk -v total="$current_total" -v idle="$current_idle" \
            -v previous_total="$last_total" -v previous_idle="$last_idle" \
            'BEGIN {
                printf "%.1f", 100 * (total - previous_total - idle + previous_idle) /
                    (total - previous_total)
            }')
    else
        cpu_usage="calcul en cours"
    fi

    last_total=$current_total
    last_idle=$current_idle
    read -r load_1 load_5 load_15 _ < /proc/loadavg

    clear_screen
    printf 'NonoOS - Tableau de bord système\n'
    printf 'Actualisé : %s    Quitter : q\n\n' "$(date '+%F %T')"
    printf 'CPU utilisé : %s%%\n' "$cpu_usage"
    printf 'Charge moyenne : %s (1 min), %s (5 min), %s (15 min)\n\n' \
        "$load_1" "$load_5" "$load_15"

    printf '%s\n' "Mémoire :"
    free -h
    printf '\n%s\n' "Espace disque de la racine :"
    df -hP /

    printf '\n%s\n' "Processus les plus actifs (CPU) :"
    ps -eo pid,user,%cpu,%mem,rss,comm --sort=-%cpu |
        awk 'NR == 1 || NR <= 6'

    printf '\n%s\n' "Processus utilisant le plus de mémoire :"
    ps -eo pid,user,%cpu,%mem,rss,comm --sort=-%mem |
        awk 'NR == 1 || NR <= 6'

    key=""
    IFS= read -r -s -n 1 -t 2 key || true
    [[ "$key" != q && "$key" != Q ]] || break
done
