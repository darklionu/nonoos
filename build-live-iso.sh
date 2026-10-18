#!/usr/bin/env bash
set -Eeuo pipefail

PROJECT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="$PROJECT_DIR/dist"
OUTPUT_ISO="$OUTPUT_DIR/nonoos-live-amd64.iso"
KERNEL_SOURCE="$PROJECT_DIR/linux-7.2.6"

if [[ "$EUID" -ne 0 ]]; then
    echo "Lancez ce script avec sudo : sudo bash $0" >&2
    exit 1
fi

if [[ ! -f "$KERNEL_SOURCE/Makefile" ]]; then
    echo "Sources du noyau introuvables : $KERNEL_SOURCE" >&2
    exit 1
fi

missing=()
for tool in lb debootstrap xorriso mksquashfs gcc make bc bison flex openssl depmod perl python3 nproc; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        missing+=("$tool")
    fi
done

if (("${#missing[@]}" > 0)); then
    printf 'Outils manquants : %s\n' "${missing[*]}" >&2
    echo "Sur Debian/Ubuntu, installez-les avec :" >&2
    echo "  sudo apt update && sudo apt install live-build debootstrap xorriso squashfs-tools grub-pc-bin grub-efi-amd64-bin mtools dosfstools build-essential bc bison flex libelf-dev libssl-dev openssl perl python3 kmod" >&2
    exit 1
fi

BUILD_DIR="$(mktemp -d "${TMPDIR:-/var/tmp}/nonoos-live.XXXXXX")"
trap 'rm -rf -- "$BUILD_DIR"' EXIT

KERNEL_BUILD_DIR="$BUILD_DIR/kernel-build"
KERNEL_STAGE="$BUILD_DIR/kernel-stage"
KERNEL_RELEASE=""

mkdir -p "$KERNEL_BUILD_DIR"
make -C "$KERNEL_SOURCE" O="$KERNEL_BUILD_DIR" x86_64_defconfig
"$KERNEL_SOURCE/scripts/config" --file "$KERNEL_BUILD_DIR/.config" \
    --set-str LOCALVERSION "-nonoos" \
    --disable LOCALVERSION_AUTO \
    --enable BLK_DEV_INITRD \
    --module BLK_DEV_LOOP \
    --module EXT4_FS \
    --module ISO9660_FS \
    --module OVERLAY_FS \
    --module SQUASHFS \
    --module VIRTIO \
    --module VIRTIO_BLK \
    --module VIRTIO_NET \
    --module VIRTIO_PCI
make -C "$KERNEL_SOURCE" O="$KERNEL_BUILD_DIR" olddefconfig
make -C "$KERNEL_SOURCE" O="$KERNEL_BUILD_DIR" -j"$(nproc)" bzImage modules
KERNEL_RELEASE="$(make -s -C "$KERNEL_SOURCE" O="$KERNEL_BUILD_DIR" kernelrelease)"
make -C "$KERNEL_SOURCE" O="$KERNEL_BUILD_DIR" \
    INSTALL_MOD_PATH="$KERNEL_STAGE" modules_install

cd "$BUILD_DIR"
lb config \
    --distribution trixie \
    --architectures amd64 \
    --binary-images iso-hybrid \
    --bootloaders "grub-pc grub-efi" \
    --linux-packages none \
    --archive-areas "main contrib non-free non-free-firmware" \
    --debian-installer live \
    --debian-installer-gui true \
    --iso-application "nonoos" \
    --iso-publisher "nonoos" \
    --iso-volume "NONOOS" \
    --bootappend-live "boot=live components locales=fr_FR.UTF-8 keyboard-layouts=fr username=nonoos hostname=nonoos"

mkdir -p config/package-lists config/includes.chroot
cat > config/package-lists/newdistrib.list.chroot <<'EOF'
live-boot
initramfs-tools
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
mkdir -p config/includes.chroot/boot config/includes.chroot/lib/modules
install -m 0644 "$KERNEL_BUILD_DIR/arch/x86/boot/bzImage" \
    "config/includes.chroot/boot/vmlinuz-$KERNEL_RELEASE"
install -m 0644 "$KERNEL_BUILD_DIR/System.map" \
    "config/includes.chroot/boot/System.map-$KERNEL_RELEASE"
install -m 0644 "$KERNEL_BUILD_DIR/.config" \
    "config/includes.chroot/boot/config-$KERNEL_RELEASE"
cp -a "$KERNEL_STAGE/lib/modules/$KERNEL_RELEASE" config/includes.chroot/lib/modules/
mkdir -p config/hooks/live
cat > "config/hooks/live/9999-nonoos-initramfs.hook.chroot" <<EOF
#!/bin/sh
set -eu
depmod -a "$KERNEL_RELEASE"
update-initramfs -c -k "$KERNEL_RELEASE"
test -s "/boot/initrd.img-$KERNEL_RELEASE"
EOF
chmod 0755 config/hooks/live/9999-nonoos-initramfs.hook.chroot

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
printf 'Noyau intégré : %s\n' "$KERNEL_RELEASE"
