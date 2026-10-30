#!/usr/bin/env bash
set -Eeuo pipefail

PROJECT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
ISO_PATH="${1:-$PROJECT_DIR/dist/nonoos-live-amd64.iso}"
TIMEOUT_SECONDS="${QEMU_TIMEOUT_SECONDS:-90}"
MEMORY_MB="${QEMU_MEMORY_MB:-2048}"
CPUS="${QEMU_CPUS:-2}"

if [[ ! -s "$ISO_PATH" ]]; then
    echo "ISO introuvable ou vide : $ISO_PATH" >&2
    echo "Construisez d'abord l'image avec sudo ./build-live-iso.sh." >&2
    exit 2
fi

if ! command -v qemu-system-x86_64 >/dev/null 2>&1; then
    echo "qemu-system-x86_64 est requis pour ce test." >&2
    echo "Installez le paquet qemu-system-x86 sur Debian/Ubuntu." >&2
    exit 2
fi

if ! command -v timeout >/dev/null 2>&1; then
    echo "La commande timeout est requise pour arrêter QEMU proprement." >&2
    exit 2
fi

LOG_FILE="$(mktemp "${TMPDIR:-/tmp}/nonoos-qemu-boot.XXXXXX.log")"
cleanup() {
    rm -f -- "$LOG_FILE"
}
trap cleanup EXIT

set +e
timeout --foreground "${TIMEOUT_SECONDS}s" \
    qemu-system-x86_64 \
    -accel tcg,thread=multi \
    -machine q35 \
    -cpu max \
    -m "$MEMORY_MB" \
    -smp "$CPUS" \
    -cdrom "$ISO_PATH" \
    -boot order=d,menu=off \
    -nic user,model=virtio \
    -display none \
    -serial file:"$LOG_FILE" \
    -monitor none \
    -no-reboot \
    -no-shutdown
qemu_status=$?
set -e

cat "$LOG_FILE"

if grep -Eiq 'Kernel panic|run-init:.*failed|Failed to execute /init|unable to mount root' "$LOG_FILE"; then
    echo "Échec : erreur de démarrage détectée dans la sortie QEMU." >&2
    exit 1
fi

if [[ "$qemu_status" -eq 124 ]]; then
    echo "Démarrage QEMU terminé après ${TIMEOUT_SECONDS}s sans erreur critique détectée."
    exit 0
fi

if [[ "$qemu_status" -ne 0 ]]; then
    echo "QEMU s'est arrêté avec le code $qemu_status." >&2
    exit "$qemu_status"
fi

echo "Démarrage QEMU terminé sans erreur critique."
