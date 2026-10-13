#!/usr/bin/env bash
set -Eeuo pipefail

PROJECT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="$PROJECT_DIR/dist"
OUTPUT_ISO="$OUTPUT_DIR/nonoos-live-amd64.iso"

if [[ "$EUID" -ne 0 ]]; then
    echo "Lancez ce script avec sudo : sudo bash $0" >&2
    exit 1
fi

missing=()
for tool in lb debootstrap xorriso mksquashfs; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        missing+=("$tool")
    fi
done

if (("${#missing[@]}" > 0)); then
    printf 'Outils manquants : %s\n' "${missing[*]}" >&2
    echo "Sur Debian/Ubuntu, installez-les avec :" >&2
    echo "  sudo apt update && sudo apt install live-build debootstrap xorriso squashfs-tools grub-pc-bin grub-efi-amd64-bin mtools dosfstools" >&2
    exit 1
fi

BUILD_DIR="$(mktemp -d "${TMPDIR:-/var/tmp}/nonoos-live.XXXXXX")"
trap 'rm -rf -- "$BUILD_DIR"' EXIT

cd "$BUILD_DIR"
lb config \
    --distribution trixie \
    --architectures amd64 \
    --binary-images iso-hybrid \
    --bootloaders grub-efi \
    --archive-areas "main contrib non-free non-free-firmware" \
    --debian-installer false \
    --iso-application "nonoos" \
    --iso-publisher "nonoos" \
    --iso-volume "NONOOS" \
    --bootappend-live "boot=live components locales=fr_FR.UTF-8 keyboard-layouts=fr username=nonoos hostname=nonoos"

mkdir -p config/package-lists config/includes.chroot
cat > config/package-lists/newdistrib.list.chroot <<'EOF'
live-boot
live-config
systemd-sysv
network-manager
wpasupplicant
whiptail
sudo
locales
keyboard-configuration
ca-certificates
kde-plasma-desktop
sddm
plasma-nm
dolphin
konsole
EOF

cp -a "$PROJECT_DIR/rootfs/." config/includes.chroot/
mkdir -p config/includes.chroot/etc/systemd/system/multi-user.target.wants
ln -sfn ../newdistrib-first-boot.service \
    config/includes.chroot/etc/systemd/system/multi-user.target.wants/newdistrib-first-boot.service
ln -sfn ../newdistrib-every-boot.service \
    config/includes.chroot/etc/systemd/system/multi-user.target.wants/newdistrib-every-boot.service

lb build

ISO_PATH="$BUILD_DIR/live-image-amd64.hybrid.iso"
if [[ ! -s "$ISO_PATH" ]]; then
    echo "La construction s'est terminée sans produire l'image ISO attendue : $ISO_PATH" >&2
    exit 1
fi

install -D -m 0644 "$ISO_PATH" "$OUTPUT_ISO"
if [[ -n "${SUDO_USER:-}" ]] && id "$SUDO_USER" >/dev/null 2>&1; then
    chown "$SUDO_USER" "$OUTPUT_ISO"
fi

printf 'ISO créée : %s\n' "$OUTPUT_ISO"
