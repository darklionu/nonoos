#!/usr/bin/env bash
set -Eeuo pipefail
export LC_ALL=C

# Les deux relevés servent à mesurer les variations sur une courte période.
declare -A cpu_before start_before name_before
declare -A cpu_after start_after name_after
declare -A energy_before max_energy_before energy_after max_energy_after

# Lit les compteurs CPU et l'heure de démarrage de chaque processus dans /proc.
read_processes() {
    local -n cpu_ref="$1"
    local -n start_ref="$2"
    local -n name_ref="$3"
    local stat pid line rest name
    local -a fields

    for stat in /proc/[0-9]*/stat; do
        [[ -r "$stat" ]] || continue
        pid=${stat#/proc/}
        pid=${pid%/stat}

        IFS= read -r line < "$stat" || continue
        [[ "$line" == *"("*") "* ]] || continue

        name=${line#*(}
        name=${name%)*}
        rest=${line##*) }
        read -r -a fields <<< "$rest"

        # Après le nom et l'état, utime/stime sont aux index 11/12 et
        # starttime à l'index 19. starttime permet d'écarter les PID réutilisés.
        ((${#fields[@]} > 19)) || continue
        [[ ${fields[11]} =~ ^[0-9]+$ && ${fields[12]} =~ ^[0-9]+$ &&
            ${fields[19]} =~ ^[0-9]+$ ]] || continue

        cpu_ref["$pid"]=$((10#${fields[11]} + 10#${fields[12]}))
        start_ref["$pid"]=$((10#${fields[19]}))
        name_ref["$pid"]=$name
    done
}

# Lit uniquement les domaines RAPL de premier niveau, sans compter à nouveau
# leurs sous-domaines (cœurs, mémoire, etc.).
read_energy() {
    local -n energy_ref="$1"
    local -n max_ref="$2"
    local domain name energy max_energy
    local found=0

    [[ -d /sys/class/powercap ]] || {
        printf '%s\n' "Erreur : RAPL indisponible (/sys/class/powercap introuvable)." >&2
        return 1
    }

    for domain in /sys/class/powercap/intel-rapl* /sys/class/powercap/amd-rapl*; do
        [[ -d "$domain" ]] || continue
        name=${domain##*/}
        [[ "$name" == intel-rapl* || "$name" == amd-rapl* ]] || continue
        [[ -r "$domain/energy_uj" && -r "$domain/max_energy_range_uj" ]] || continue

        IFS= read -r energy < "$domain/energy_uj" || continue
        IFS= read -r max_energy < "$domain/max_energy_range_uj" || continue
        [[ "$energy" =~ ^[0-9]+$ && "$max_energy" =~ ^[0-9]+$ ]] || continue
        ((10#$max_energy > 0)) || continue

        energy_ref["$domain"]=$((10#$energy))
        max_ref["$domain"]=$((10#$max_energy))
        found=1
    done

    if ((found == 0)); then
        printf '%s\n' "Erreur : aucun compteur RAPL lisible." >&2
        return 1
    fi
}

# Garde une sortie numérique indépendante de la locale de la machine.
format_number() {
    awk -v value="$1" 'BEGIN { printf "%.3f", value }'
}

# Premier relevé, attente d'une seconde, puis second relevé des compteurs.
read_processes cpu_before start_before name_before
read_energy energy_before max_energy_before
IFS= read -r start_uptime _ < /proc/uptime

sleep 1

read_processes cpu_after start_after name_after
read_energy energy_after max_energy_after
IFS= read -r end_uptime _ < /proc/uptime

# Calcule la différence des compteurs RAPL. Si un compteur a atteint sa limite
# entre les deux lectures, il est reparti de zéro : on tient compte du retour.
energy_microjoules=0
for domain in "${!energy_after[@]}"; do
    [[ ${energy_before["$domain"]+present} ]] || continue
    energy=${energy_after["$domain"]}
    previous_energy=${energy_before["$domain"]}

    if ((energy >= previous_energy)); then
        difference=$((energy - previous_energy))
    else
        difference=$((max_energy_before["$domain"] -
            previous_energy + energy))
    fi

    energy_microjoules=$((energy_microjoules + difference))
done

duration=$(awk -v start="$start_uptime" -v end="$end_uptime" \
    'BEGIN { printf "%.3f", end - start }')
energy_joules=$(awk -v energy="$energy_microjoules" \
    'BEGIN { printf "%.9f", energy / 1000000 }')

# Additionne les variations CPU des seuls processus présents aux deux relevés,
# en vérifiant que le PID n'a pas été réattribué à un nouveau processus.
total_cpu=0
for pid in "${!cpu_after[@]}"; do
    [[ ${cpu_before[$pid]+present} ]] || continue
    [[ ${start_before[$pid]} == "${start_after[$pid]}" ]] || continue
    ((cpu_after[$pid] >= cpu_before[$pid])) || continue

    total_cpu=$((total_cpu + cpu_after[$pid] - cpu_before[$pid]))
done

if ((total_cpu == 0)); then
    printf 'Énergie totale du processeur : %s J en %s s.\n' \
        "$(format_number "$energy_joules")" "$duration"
    printf '%s\n' "Aucune activité CPU attribuable à un processus n'a été détectée."
    exit 0
fi

# Répartit l'énergie totale au prorata du temps CPU consommé. Cette valeur est
# une estimation, car RAPL ne mesure pas directement l'énergie par processus.
results=()
for pid in "${!cpu_after[@]}"; do
    [[ ${cpu_before[$pid]+present} ]] || continue
    [[ ${start_before[$pid]} == "${start_after[$pid]}" ]] || continue
    ((cpu_after[$pid] >= cpu_before[$pid])) || continue

    process_cpu=$((cpu_after[$pid] - cpu_before[$pid]))
    ((process_cpu > 0)) || continue
    process_energy=$(awk -v energy="$energy_microjoules" \
        -v cpu="$process_cpu" -v total="$total_cpu" \
        'BEGIN { printf "%.12f", energy * cpu / total / 1000000 }')
    results+=("$process_energy"$'\t'"$pid"$'\t'"${name_after[$pid]}")
done

printf 'Énergie processeur mesurée : %s J en %s s\n' \
    "$(format_number "$energy_joules")" "$duration"
printf '%s\n' "Top 5 des estimations par processus :"

# Trie les estimations décroissantes et n'affiche que les cinq premières.
index=0
while IFS=$'\t' read -r process_energy pid name; do
    ((index += 1))
    printf '%d. %s (PID %s) : %s J\n' \
        "$index" "$name" "$pid" "$(format_number "$process_energy")"
    ((index >= 5)) && break
done < <(printf '%s\n' "${results[@]}" | sort -t $'\t' -k1,1nr)

printf '\n%s\n' \
    "Attention : l'énergie est mesurée pour le processeur, puis répartie approximativement selon l'activité CPU. Ce n'est pas une mesure exacte par application."
