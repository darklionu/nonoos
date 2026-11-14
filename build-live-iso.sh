#!/usr/bin/env bash
set -Eeuo pipefail
shopt -s nullglob

PROJECT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="$PROJECT_DIR/dist"
TARGET_ARCH="${TARGET_ARCH:-amd64}"
KERNEL_SOURCE="$PROJECT_DIR/linux-6.18"
BUILD_DIR="${BUILD_DIR:-$PROJECT_DIR/.build/nonoos-live}"
DEBIAN_MIRROR="${DEBIAN_MIRROR:-http://deb.debian.org/debian}"
APT_HTTP_PROXY="${APT_HTTP_PROXY:-}"
SECURE_BOOT="${SECURE_BOOT:-0}"
SECURE_BOOT_KEY="${SECURE_BOOT_KEY:-}"
SECURE_BOOT_CERT="${SECURE_BOOT_CERT:-}"
BOOTLOADER=syslinux
TOTAL_STAGES=9

case "$TARGET_ARCH" in
    amd64|x86_64)
        TARGET_ARCH="amd64"
        DEBIAN_ARCH="amd64"
        KERNEL_ARCH="x86_64"
        KERNEL_DEFCONFIG="x86_64_defconfig"
        KERNEL_BOOT_PATH="arch/x86/boot/bzImage"
        KERNEL_BUILD_TARGET="bzImage"
        GRUB_EFI_PACKAGE="grub-efi-amd64-bin"
        GRUB_EFI_SIGNED_PACKAGE="grub-efi-amd64-signed"
        CROSS_COMPILE=""
        ;;
    arm64|aarch64)
        TARGET_ARCH="arm64"
        DEBIAN_ARCH="arm64"
        KERNEL_ARCH="arm64"
        KERNEL_DEFCONFIG="defconfig"
        KERNEL_BOOT_PATH="arch/arm64/boot/Image"
        KERNEL_BUILD_TARGET="Image"
        GRUB_EFI_PACKAGE="grub-efi-arm64-bin"
        GRUB_EFI_SIGNED_PACKAGE="grub-efi-arm64-signed"
        CROSS_COMPILE="aarch64-linux-gnu-"
        BOOTLOADER=grub-efi
        ;;
    *)
        echo "ARCHITECTURE cible invalide : $TARGET_ARCH (valeurs supportées : amd64, x86_64, arm64, aarch64)" >&2
        exit 1
        ;;
esac

OUTPUT_ISO="$OUTPUT_DIR/nonoos-live-${TARGET_ARCH}.iso"

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

if [[ "$TARGET_ARCH" == "arm64" ]] && ! command -v aarch64-linux-gnu-gcc >/dev/null 2>&1; then
    missing+=("aarch64-linux-gnu-gcc")
fi

if (("${#missing[@]}" > 0)); then
    printf 'Outils manquants : %s\n' "${missing[*]}" >&2
    echo "Sur Debian/Ubuntu, installez-les avec :" >&2
    echo "  sudo apt update && sudo apt install live-build debootstrap xorriso squashfs-tools grub-pc-bin grub-efi-amd64-bin grub-efi-arm64-bin mtools dosfstools build-essential bc bison flex libelf-dev libssl-dev openssl perl python3 rustc kmod qt6-base-dev gcc-aarch64-linux-gnu" >&2
    exit 1
fi

for required_file in \
    /usr/include/gelf.h \
    /usr/share/keyrings/debian-archive-keyring.gpg
do
    if [[ ! -r "$required_file" ]]; then
        echo "Fichier requis introuvable : $required_file" >&2
        echo "Installez libelf-dev et debian-archive-keyring sur l'hôte." >&2
        exit 1
    fi
done

if ! pkg-config --exists Qt6Widgets; then
    echo "Qt 6 Widgets est introuvable via pkg-config." >&2
    echo "Installez le paquet qt6-base-dev sur l’hôte de construction." >&2
    exit 1
fi

if [[ "$SECURE_BOOT" == 1 ]]; then
    if [[ -z "$SECURE_BOOT_KEY" || -z "$SECURE_BOOT_CERT" ]]; then
        echo "SECURE_BOOT=1 nécessite SECURE_BOOT_KEY et SECURE_BOOT_CERT." >&2
        exit 1
    fi
    for secure_boot_file in "$SECURE_BOOT_KEY" "$SECURE_BOOT_CERT"; do
        if [[ ! -r "$secure_boot_file" ]]; then
            echo "Fichier Secure Boot introuvable ou illisible : $secure_boot_file" >&2
            exit 1
        fi
    done
    if ! command -v sbsign >/dev/null 2>&1; then
        echo "Outil requis pour Secure Boot introuvable : sbsign" >&2
        echo "Installez le paquet sbsigntool sur l’hôte de construction." >&2
        exit 1
    fi
    BOOTLOADER=grub-efi
elif [[ "$SECURE_BOOT" != 0 ]]; then
    echo "SECURE_BOOT doit valoir 0 ou 1." >&2
    exit 1
fi

if [[ "$TARGET_ARCH" == "arm64" ]]; then
    if [[ "$BOOTLOADER" == "syslinux" ]]; then
        BOOTLOADER=grub-efi
    fi
fi

progress 1 "Préparation de l'environnement de construction"
mkdir -p "$BUILD_DIR"

progress 2 "Compilation des outils personnalisés"
g++ -std=c++17 -Wall -Wextra -pedantic \
    "$PROJECT_DIR/rootfs/update-center/interface_graphique.cpp" \
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

g++ -std=c++17 -Wall -Wextra -pedantic \
    "$PROJECT_DIR/rootfs/gestion-alllimentations/power-gui.cpp" \
    $(pkg-config --cflags --libs Qt6Widgets) \
    -o "$BUILD_DIR/nonoos-power-gui"

rustc --edition=2021 -O "$PROJECT_DIR/src/nonoos-security.rs" \
    -o "$BUILD_DIR/nonoos-security"

KERNEL_BUILD_DIR="$BUILD_DIR/kernel-build"
KERNEL_STAGE="$BUILD_DIR/kernel-stage"
KERNEL_RELEASE=""

progress 3 "Configuration du noyau Linux"
mkdir -p "$KERNEL_BUILD_DIR"
make -C "$KERNEL_SOURCE" O="$KERNEL_BUILD_DIR" ARCH="$KERNEL_ARCH" CROSS_COMPILE="$CROSS_COMPILE" "$KERNEL_DEFCONFIG"
"$KERNEL_SOURCE/scripts/config" --file "$KERNEL_BUILD_DIR/.config" \
    --set-str LOCALVERSION "-nonoos" \
    --disable LOCALVERSION_AUTO \
    --enable BLK_DEV_INITRD \
    --enable BLK_DEV_LOOP \
    --enable EXT4_FS \
    --enable ISO9660_FS \
    --enable OVERLAY_FS \
    --enable SQUASHFS \
    --enable EFI \
    --enable EFI_STUB \
    --enable EFI_PARTITION \
    --enable VFAT_FS \
    --enable MSDOS_PARTITION \
    --enable ATA \
    --enable SATA_AHCI \
    --enable BLK_DEV_NVME \
    --enable VIRTIO \
    --enable VIRTIO_BLK \
    --enable VIRTIO_NET \
    --enable VIRTIO_PCI
if [[ "$TARGET_ARCH" == "arm64" ]]; then
    "$KERNEL_SOURCE/scripts/config" --file "$KERNEL_BUILD_DIR/.config" \
        --enable ARM64_VA_BITS_48 \
        --enable ARM64_64K_PAGES \
        --enable PCI \
        --enable PCI_HOST_GENERIC
fi
if [[ "$SECURE_BOOT" == 1 ]]; then
    "$KERNEL_SOURCE/scripts/config" --file "$KERNEL_BUILD_DIR/.config" \
        --enable MODULE_SIG \
        --enable MODULE_SIG_ALL \
        --enable MODULE_SIG_SHA256 \
        --set-str MODULE_SIG_KEY "$SECURE_BOOT_KEY" \
        --set-str SYSTEM_TRUSTED_KEYS "$SECURE_BOOT_CERT"
fi
make -C "$KERNEL_SOURCE" O="$KERNEL_BUILD_DIR" olddefconfig
for required_config in \
    CONFIG_BLK_DEV_INITRD \
    CONFIG_BLK_DEV_LOOP \
    CONFIG_EXT4_FS \
    CONFIG_ISO9660_FS \
    CONFIG_OVERLAY_FS \
    CONFIG_SQUASHFS \
    CONFIG_EFI \
    CONFIG_EFI_STUB \
    CONFIG_EFI_PARTITION \
    CONFIG_VFAT_FS \
    CONFIG_MSDOS_PARTITION \
    CONFIG_ATA \
    CONFIG_SATA_AHCI \
    CONFIG_BLK_DEV_NVME \
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
if [[ "$SECURE_BOOT" == 1 ]]; then
    for required_config in CONFIG_MODULE_SIG CONFIG_MODULE_SIG_ALL CONFIG_MODULE_SIG_SHA256; do
        if ! grep -q "^${required_config}=y$" "$KERNEL_BUILD_DIR/.config"; then
            echo "Configuration Secure Boot requise absente : $required_config" >&2
            exit 1
        fi
    done
fi

progress 4 "Compilation du noyau Linux et de ses modules"
make -C "$KERNEL_SOURCE" O="$KERNEL_BUILD_DIR" ARCH="$KERNEL_ARCH" CROSS_COMPILE="$CROSS_COMPILE" -j"$(nproc)" "$KERNEL_BUILD_TARGET" modules
KERNEL_RELEASE="$(make -s -C "$KERNEL_SOURCE" O="$KERNEL_BUILD_DIR" ARCH="$KERNEL_ARCH" CROSS_COMPILE="$CROSS_COMPILE" kernelrelease)"
make -C "$KERNEL_SOURCE" O="$KERNEL_BUILD_DIR" ARCH="$KERNEL_ARCH" CROSS_COMPILE="$CROSS_COMPILE" \
    INSTALL_MOD_PATH="$KERNEL_STAGE" modules_install

KERNEL_IMAGE="$KERNEL_BUILD_DIR/$KERNEL_BOOT_PATH"
if [[ "$SECURE_BOOT" == 1 ]]; then
    local_signed_image="${KERNEL_IMAGE%.gz}.signed"
    if [[ "$TARGET_ARCH" == "arm64" ]]; then
        local_signed_image="${KERNEL_IMAGE%.*}.signed"
    fi
    sbsign --key "$SECURE_BOOT_KEY" --cert "$SECURE_BOOT_CERT" \
        --output "$local_signed_image" \
        "$KERNEL_IMAGE"
    KERNEL_IMAGE="$local_signed_image"
fi

progress 5 "Configuration de l'image live Debian"
cd "$BUILD_DIR"
lb_config_args=(
    --distribution trixie
    --architectures "$DEBIAN_ARCH"
    --mode debian
    --binary-images iso-hybrid
    --bootloader "$BOOTLOADER"
    --syslinux-theme live-build
    --initramfs live-boot
    --initsystem systemd
    --mirror-bootstrap "$DEBIAN_MIRROR"
    --mirror-chroot "$DEBIAN_MIRROR"
    --mirror-binary "$DEBIAN_MIRROR"
    --keyring-packages "debian-archive-keyring"
    --security false
    --linux-packages none
    --archive-areas "main contrib non-free non-free-firmware"
    --debian-installer false
    --iso-application "nonoos"
    --iso-publisher "nonoos"
    --iso-volume "NONOOS"
    --bootappend-live "boot=live components persistence console=ttyS0,115200n8 locales=fr_FR.UTF-8 keyboard-layouts=fr username=nonoos hostname=nonoos"
)
if [[ -n "$APT_HTTP_PROXY" ]]; then
    lb_config_args+=(--apt-http-proxy "$APT_HTTP_PROXY")
fi
lb config "${lb_config_args[@]}"

mkdir -p config/package-lists config/includes.chroot
cat > config/package-lists/newdistrib.list.chroot <<EOF
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
calamares
calamares-settings-debian
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
power-profiles-daemon
ufw
bubblewrap
e2fsprogs
util-linux
parted
gparted
efibootmgr
dosfstools
$GRUB_EFI_PACKAGE
grub2-common
cryptsetup
lvm2
btrfs-progs
xfsprogs
EOF

if [[ "$TARGET_ARCH" == "amd64" ]]; then
    cat >> config/package-lists/newdistrib.list.chroot <<'EOF'
isolinux
EOF
fi

if [[ "$SECURE_BOOT" == 1 ]]; then
    cat >> config/package-lists/newdistrib.list.chroot <<EOF
$GRUB_EFI_PACKAGE
${GRUB_EFI_SIGNED_PACKAGE}
shim-signed
EOF
fi

if [[ "$TARGET_ARCH" == "amd64" ]]; then
    mkdir -p config/includes.chroot/usr/lib/syslinux
    ln -sfn /usr/lib/ISOLINUX/isolinux.bin \
        config/includes.chroot/usr/lib/syslinux/isolinux.bin
    ln -sfn /usr/lib/syslinux/modules/bios/vesamenu.c32 \
        config/includes.chroot/usr/lib/syslinux/vesamenu.c32
fi

progress 6 "Installation des fichiers NonoOS dans l'image"
cp -a "$PROJECT_DIR/rootfs/." config/includes.chroot/
install -D -m 0755 "$BUILD_DIR/nonoos-software-center" \
    config/includes.chroot/usr/bin/nonoos-software-center
install -D -m 0755 "$BUILD_DIR/nonoos-security-center" \
    config/includes.chroot/usr/bin/nonoos-security-center
install -D -m 0755 "$BUILD_DIR/nonoos-startup-manager" \
    config/includes.chroot/usr/bin/nonoos-startup-manager
install -D -m 0755 "$BUILD_DIR/nonoos-power-gui" \
    config/includes.chroot/usr/bin/nonoos-power-gui
install -D -m 0755 "$BUILD_DIR/nonoos-security" \
    config/includes.chroot/usr/bin/nonoos-security
install -D -m 0755 "$PROJECT_DIR/rootfs/etc/new commande/commande_tableauxdebord.sh" \
    config/includes.chroot/usr/local/bin/nonoos-dashboard
install -D -m 0755 "$PROJECT_DIR/rootfs/etc/new commande/commande_newuser.sh" \
    config/includes.chroot/usr/local/bin/nonoos-create-test-user
install -D -m 0755 "$PROJECT_DIR/rootfs/etc/new commande/commande_testshell.sh" \
    config/includes.chroot/usr/local/bin/nonoos-test-shell
install -D -m 0755 "$PROJECT_DIR/rootfs/gestion-alllimentations/nonoos-power" \
    config/includes.chroot/usr/local/bin/nonoos-power
install -D -m 0755 "$PROJECT_DIR/rootfs/gestionnaire de sécurité/s" \
    config/includes.chroot/usr/local/bin/nonoos-firewall
mkdir -p config/includes.chroot/boot config/includes.chroot/lib/modules
install -m 0644 "$KERNEL_IMAGE" \
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

mkdir -p config/hooks
cat > config/hooks/0000-nonoos-network.chroot_early <<'EOF'
#!/bin/sh
set -eu

mkdir -p /etc/apt/apt.conf.d
cat > /etc/apt/apt.conf.d/99nonoos-network <<'APT_CONF'
Acquire::ForceIPv4 "true";
APT_CONF
EOF
chmod 0755 config/hooks/0000-nonoos-network.chroot_early

mkdir -p config/includes.chroot/etc/systemd/system/multi-user.target.wants
ln -sfn ../newdistrib-first-boot.service \
    config/includes.chroot/etc/systemd/system/multi-user.target.wants/newdistrib-first-boot.service
ln -sfn ../newdistrib-every-boot.service \
    config/includes.chroot/etc/systemd/system/multi-user.target.wants/newdistrib-every-boot.service

progress 7 "Préparation du système Debian et de l'image live"
lb bootstrap

host_nameservers=()
while read -r nameserver; do
    host_nameservers+=("$nameserver")
done < <(awk '$1 == "nameserver" { print $2 }' /etc/resolv.conf)
if ((${#host_nameservers[@]} == 0)); then
    echo "Aucun serveur DNS trouvé dans /etc/resolv.conf." >&2
    exit 1
fi

rm -f chroot/etc/resolv.conf
{
    for nameserver in "${host_nameservers[@]}"; do
        printf 'nameserver %s\n' "$nameserver"
    done
    printf 'options timeout:3 attempts:5\n'
} > chroot/etc/resolv.conf

lb chroot_hosts install
mirror_host="${DEBIAN_MIRROR#*://}"
mirror_host="${mirror_host%%/*}"
mirror_addresses="$(getent ahostsv4 "$mirror_host" | awk '!seen[$1]++ { print $1 }')"
if [[ -z "$mirror_addresses" ]]; then
    echo "Impossible de résoudre $mirror_host sur l'hôte." >&2
    exit 1
fi
while IFS= read -r address; do
    printf '%s\t%s\n' "$address" "$mirror_host" >> chroot/etc/hosts
done <<< "$mirror_addresses"
mkdir -p chroot/etc/apt/apt.conf.d
cat > chroot/etc/apt/apt.conf.d/99nonoos-network <<'EOF'
Acquire::ForceIPv4 "true";
EOF

lb chroot

if ! chroot "$BUILD_DIR/chroot" dpkg-query -W -f='${Status}' initramfs-tools 2>/dev/null |
    grep -q '^install ok installed$'
then
    chroot "$BUILD_DIR/chroot" apt-get --yes install initramfs-tools
fi

chroot "$BUILD_DIR/chroot" depmod -a "$KERNEL_RELEASE"
chroot "$BUILD_DIR/chroot" update-initramfs -c -k "$KERNEL_RELEASE"

mkdir -p binary/live
kernel_files=(chroot/boot/vmlinuz-"$KERNEL_RELEASE")
initramfs_files=(chroot/boot/initrd.img-"$KERNEL_RELEASE")
if ((${#kernel_files[@]} != 1 || ${#initramfs_files[@]} != 1)); then
    echo "Noyau ou initramfs personnalisé introuvable dans chroot/boot." >&2
    printf 'Noyau : %s\n' "${kernel_files[*]:-absent}" >&2
    printf 'Initramfs : %s\n' "${initramfs_files[*]:-absent}" >&2
    exit 1
fi
cp -a "${kernel_files[0]}" "${initramfs_files[0]}" binary/live/

progress 8 "Construction de l'image ISO (live-build)"
lb binary

ISO_PATH="$BUILD_DIR/live-image-${DEBIAN_ARCH}.hybrid.iso"
if [[ ! -s "$ISO_PATH" ]]; then
    echo "La construction s'est terminée sans produire l'image ISO attendue : $ISO_PATH" >&2
    exit 1
fi

progress 9 "Finalisation de l'image ISO"
install -D -m 0644 "$ISO_PATH" "$OUTPUT_ISO"
if [[ -n "${SUDO_USER:-}" ]] && id "$SUDO_USER" >/dev/null 2>&1; then
    chown "$SUDO_USER" "$OUTPUT_ISO"
fi

printf 'ISO créée : %s\n' "$OUTPUT_ISO"
printf 'Noyau intégré : %s\n' "$KERNEL_RELEASE"
