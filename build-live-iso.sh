#!/usr/bin/env bash
set -Eeuo pipefail

PROJECT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="$PROJECT_DIR/dist"
OUTPUT_ISO="$OUTPUT_DIR/nonoos-live-amd64.iso"
KERNEL_SOURCE="$PROJECT_DIR/linux-7.2.6"
TOTAL_STAGES=8

progress() {
    local stage="$1"
    local message="$2"
    local width=32
    local filled=$((stage * width / TOTAL_STAGES))
    local empty=$((width - filled))
    local percent=$((stage * 100 / TOTAL_STAGES))
    printf '\n[%3d%%] [%s%s] %s\n' \
        "$percent" \
        "$(printf '%*s' "$filled" '' | tr ' ' '#')" \
        "$(printf '%*s' "$empty" '')" \
        "$message" >&2
}

if [[ "$EUID" -ne 0 ]]; then
    echo "Lancez ce script avec sudo : sudo bash $0" >&2
    exit 1
fi

if [[ ! -f "$KERNEL_SOURCE/Makefile" ]]; then
    echo "Sources du noyau introuvables : $KERNEL_SOURCE" >&2
    exit 1
fi

missing=()
for tool in lb debootstrap xorriso mksquashfs gcc g++ pkg-config make bc bison flex openssl depmod perl python3 rustc nproc; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        missing+=("$tool")
    fi
done

if (("${#missing[@]}" > 0)); then
    printf 'Outils manquants : %s\n' "${missing[*]}" >&2
    echo "Sur Debian/Ubuntu, installez-les avec :" >&2
    echo "  sudo apt update && sudo apt install live-build debootstrap xorriso squashfs-tools grub-pc-bin grub-efi-amd64-bin mtools dosfstools build-essential bc bison flex libelf-dev libssl-dev openssl perl python3 rustc kmod qt6-base-dev" >&2
    exit 1
fi

if ! pkg-config --exists Qt6Widgets; then
    echo "Qt 6 Widgets est introuvable via pkg-config." >&2
    echo "Installez le paquet qt6-base-dev sur l’hôte de construction." >&2
    exit 1
fi

progress 1 "Préparation de l'environnement de construction"
BUILD_DIR="$(mktemp -d "${TMPDIR:-/var/tmp}/nonoos-live.XXXXXX")"
trap 'rm -rf -- "$BUILD_DIR"' EXIT

progress 2 "Compilation des outils personnalisés"
g++ -std=c++17 -Wall -Wextra -pedantic \
    "$PROJECT_DIR/rootfs/gestionnaire de mise à jour/interface_graphique.cpp" \
    $(pkg-config --cflags --libs Qt6Widgets) \
    -o "$BUILD_DIR/nonoos-software-center"

g++ -std=c++17 -Wall -Wextra -pedantic \
    "$PROJECT_DIR/rootfs/gestionnaire de sécurité/interface.cpp" \
    $(pkg-config --cflags --libs Qt6Widgets) \
    -o "$BUILD_DIR/nonoos-security-center"

g++ -std=c++17 -Wall -Wextra -pedantic \
    "$PROJECT_DIR/rootfs/apps/gestionnaire-demarrage/main.cpp" \
    $(pkg-config --cflags --libs Qt6Widgets) \
    -o "$BUILD_DIR/nonoos-startup-manager"

rustc --edition=2021 -O "$PROJECT_DIR/src/nonoos-security.rs" \
    -o "$BUILD_DIR/nonoos-security"

KERNEL_BUILD_DIR="$BUILD_DIR/kernel-build"
KERNEL_STAGE="$BUILD_DIR/kernel-stage"
KERNEL_RELEASE=""

progress 3 "Configuration du noyau Linux"
mkdir -p "$KERNEL_BUILD_DIR"
make -C "$KERNEL_SOURCE" O="$KERNEL_BUILD_DIR" x86_64_defconfig
"$KERNEL_SOURCE/scripts/config" --file "$KERNEL_BUILD_DIR/.config" \
    --set-str LOCALVERSION "-nonoos" \
    --disable LOCALVERSION_AUTO \
    --enable BLK_DEV_INITRD \
    --enable BLK_DEV_LOOP \
    --enable EXT4_FS \
    --enable ISO9660_FS \
    --enable OVERLAY_FS \
    --enable SQUASHFS \
    --enable VIRTIO \
    --enable VIRTIO_BLK \
    --enable VIRTIO_NET \
    --enable VIRTIO_PCI
make -C "$KERNEL_SOURCE" O="$KERNEL_BUILD_DIR" olddefconfig
for required_config in \
    CONFIG_BLK_DEV_INITRD \
    CONFIG_BLK_DEV_LOOP \
    CONFIG_EXT4_FS \
    CONFIG_ISO9660_FS \
    CONFIG_OVERLAY_FS \
    CONFIG_SQUASHFS \
    CONFIG_VIRTIO \
    CONFIG_VIRTIO_BLK \
    CONFIG_VIRTIO_NET \
    CONFIG_VIRTIO_PCI
do
    if ! grep -q "^${required_config}=y$" "$KERNEL_BUILD_DIR/.config"; then
        echo "Configuration noyau requise absente ou non intégrée : $required_config" >&2
        exit 1
    fi
done

progress 4 "Compilation du noyau Linux et de ses modules"
make -C "$KERNEL_SOURCE" O="$KERNEL_BUILD_DIR" -j"$(nproc)" bzImage modules
KERNEL_RELEASE="$(make -s -C "$KERNEL_SOURCE" O="$KERNEL_BUILD_DIR" kernelrelease)"
make -C "$KERNEL_SOURCE" O="$KERNEL_BUILD_DIR" \
    INSTALL_MOD_PATH="$KERNEL_STAGE" modules_install

progress 5 "Configuration de l'image live Debian"
cd "$BUILD_DIR"
lb config \
    --distribution trixie \
    --architectures amd64 \
    --binary-images iso-hybrid \
    --bootloader grub \
    --mirror-bootstrap "http://deb.debian.org/debian" \
    --mirror-chroot "http://deb.debian.org/debian" \
    --mirror-chroot-security "http://security.debian.org/debian-security" \
    --mirror-binary "http://deb.debian.org/debian" \
    --mirror-binary-security "http://security.debian.org/debian-security" \
    --keyring-packages "debian-archive-keyring" \
    --linux-packages none \
    --archive-areas "main contrib non-free non-free-firmware" \
    --debian-installer live \
    --debian-installer-gui true \
    --iso-application "nonoos" \
    --iso-publisher "nonoos" \
    --iso-volume "NONOOS" \
    --bootappend-live "boot=live components persistence console=ttyS0,115200n8 locales=fr_FR.UTF-8 keyboard-layouts=fr username=nonoos hostname=nonoos"

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
libsdl2-dev
kde-plasma-desktop
sddm
plasma-nm
dolphin
konsole
pkexec
polkitd
polkit-kde-agent-1
libqt6widgets6
qt6-qpa-plugins
python3
procps
iproute2
flatpak
ufw
bubblewrap
e2fsprogs
util-linux
EOF

progress 6 "Installation des fichiers NonoOS dans l'image"
cp -a "$PROJECT_DIR/rootfs/." config/includes.chroot/
install -D -m 0755 "$BUILD_DIR/nonoos-software-center" \
    config/includes.chroot/usr/bin/nonoos-software-center
install -D -m 0755 "$BUILD_DIR/nonoos-security-center" \
    config/includes.chroot/usr/bin/nonoos-security-center
install -D -m 0755 "$BUILD_DIR/nonoos-startup-manager" \
    config/includes.chroot/usr/bin/nonoos-startup-manager
install -D -m 0755 "$BUILD_DIR/nonoos-security" \
    config/includes.chroot/usr/bin/nonoos-security
install -D -m 0755 "$PROJECT_DIR/rootfs/etc/new commande/commande_tableauxdebord.sh" \
    config/includes.chroot/usr/local/bin/nonoos-dashboard
install -D -m 0755 "$PROJECT_DIR/rootfs/etc/new commande/commande_newuser.sh" \
    config/includes.chroot/usr/local/bin/nonoos-create-test-user
install -D -m 0755 "$PROJECT_DIR/rootfs/etc/new commande/commande_testshell.sh" \
    config/includes.chroot/usr/local/bin/nonoos-test-shell
install -D -m 0755 "$PROJECT_DIR/rootfs/gestionnaire de sécurité/s" \
    config/includes.chroot/usr/local/bin/nonoos-firewall
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

progress 7 "Construction de l'image ISO (live-build)"
lb build

ISO_PATH="$BUILD_DIR/live-image-amd64.hybrid.iso"
if [[ ! -s "$ISO_PATH" ]]; then
    echo "La construction s'est terminée sans produire l'image ISO attendue : $ISO_PATH" >&2
    exit 1
fi

progress 8 "Finalisation de l'image ISO"
install -D -m 0644 "$ISO_PATH" "$OUTPUT_ISO"
if [[ -n "${SUDO_USER:-}" ]] && id "$SUDO_USER" >/dev/null 2>&1; then
    chown "$SUDO_USER" "$OUTPUT_ISO"
fi

printf 'ISO créée : %s\n' "$OUTPUT_ISO"
printf 'Noyau intégré : %s\n' "$KERNEL_RELEASE"
