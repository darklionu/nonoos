# NONOOS

NonoOS est une distribution Linux live basée sur Debian et conçue pour servir de base à un environnement de bureau personnalisé, avec un accent sur la maintenance système, la sécurité, l’administration et les outils de diagnostic.

Le projet construit une image ISO bootable pour architectures amd64/x86_64 et arm64/aarch64, embarque des composants KDE Plasma et propose un ensemble d’outils de gestion, de surveillance et d’installation adaptés à des environnements de test, de démonstration et de personnalisation.

## Ce que contient le projet

NonoOS assemble un système live Debian pour les machines amd64/x86_64 et arm64/aarch64. L’image
comprend notamment :

- un noyau Linux 6.18 configuré et compilé par le script du projet ;
- un environnement KDE Plasma, avec SDDM pour ouvrir une session graphique ;
- NetworkManager et une configuration francophone par défaut ;
- des outils graphiques NonoOS pour le démarrage, les logiciels, la sécurité,
  l’alimentation et les informations système ;
- un choix de profil personnel par compte (standard, développeur ou support),
  modifiable depuis le bureau ou la commande `nonoos-profile` ;
- ChatIA et des outils de diagnostic pour le Wi-Fi, l’USB et la mémoire ;
- Calamares pour guider l’installation sur disque ;
- des services de premier démarrage et d’exécution à chaque démarrage ;
- le mécanisme de persistance live, à utiliser avec une partition dédiée.

Le contenu de `rootfs/` est ajouté au système de fichiers de l’image ; les
binaires personnalisés sont construits séparément pendant la génération de
l’ISO. La documentation de ce contenu se trouve dans
[`rootfs/README.md`](rootfs/README.md).

## Arborescence principale

```text
.
├── build-live-iso.sh          # script principal de génération de l'ISO
├── docker/                    # fichiers Docker et tests de fumée
├── dist/                      # sortie de build (ISO générée)
├── linux-6.18/                # sources du noyau Linux 6.18
├── rootfs/                    # contenu du système live / outils personnalisés
├── src/                       # composants complémentaires (Rust, outils système)
├── tests/                     # scripts de validation et de test du boot
├── .github/workflows/         # CI GitHub Actions
├── README.md                  # documentation du projet
└── ...
```

### Sous-arborescence utile

```text
rootfs/
├── apps/                         # applications NonoOS
├── etc/                          # configuration, services et scripts système
├── gestion-alllimentations/      # interface et commande d’alimentation
├── gestionnaire de sécurité/     # centre de sécurité
├── nonoos-info/                  # tableau de bord système
├── update-center/                # interface de logiciels et mises à jour
├── usr/
│   ├── src/                      # ChatIA et outils USB/mémoire
│   ├── migration-assistant/      # détection et inventaire de systèmes
│   └── share/applications/       # lanceurs du bureau
└── ...
```

## Fonctionnalités principales

### Environnement système

- base Debian trixie / live-build
- KDE Plasma + SDDM + NetworkManager
- support clavier AZERTY et langue française
- configuration automatisée au premier démarrage
- persistance optionnelle sur partition dédiée

### Outils intégrés

- **Gestionnaire de démarrage** : interface d’administration des options liées
  au démarrage.
- **Centre de logiciels et mises à jour** : interface graphique des opérations
  de gestion logicielle.
- **Centre de sécurité** : accès aux outils et réglages de sécurité de NonoOS.
- **Gestion de l’alimentation** : interface et commande dédiées aux actions
  d’alimentation.
- **Tableau de bord système** : informations et état général de la machine.
- **ChatIA** : assistant fourni avec ses sources dans `rootfs/usr/src/ia/`.
- **Outils de diagnostic** : composants liés au Wi-Fi, aux périphériques USB
  et à la mémoire.
- **Assistant de migration** : inventaire et détection de systèmes installés,
  avec validation supplémentaire de la source et de la destination, refus des
  destinations situées dans le système source et copie sans écrasement des
  fichiers existants ; consultez sa documentation avant toute opération sur des
  données.
- **Profils utilisateur** : choix individuel Standard, Développeur ou Support
  au premier démarrage ; chaque compte peut consulter ou changer son choix
  depuis le lanceur « Profils utilisateur NonoOS » ou avec `nonoos-profile`.
  Ce profil ne change pas les privilèges du compte ni les logiciels installés.

## Prérequis de compilation

La construction doit être lancée sur une machine Debian ou Ubuntu amd64/x86_64
ou arm64/aarch64 avec un accès aux dépôts de paquets. Elle requiert les
outils de live-build, les outils de création d’image, les compilateurs C/C++,
Rust et les bibliothèques de développement utilisées par les applications. Pour
un build amd64/x86_64, installez également les paquets EFI amd64 ; pour un
build arm64/aarch64, installez le cross-toolchain GNU ARM 64 et les paquets EFI
arm64 :

```bash
sudo apt update
sudo apt install \
  live-build debootstrap xorriso squashfs-tools \
  grub-pc-bin grub-efi-amd64-bin grub-efi-arm64-bin \
  mtools dosfstools build-essential bc bison flex \
  libelf-dev libssl-dev openssl perl python3 rustc \
  kmod qt6-base-dev pkg-config gcc-aarch64-linux-gnu \
  debian-archive-keyring
```

Le script vérifie la présence de `lb`, `debootstrap`, `xorriso`,
`mksquashfs`, `gcc`, `g++`, `make`, `bc`, `bison`, `flex`, `openssl`,
`depmod`, `perl`, `python3`, `rustc`, `nproc` et de Qt 6 Widgets via
`pkg-config`. Il vérifie également les en-têtes libelf et le trousseau de clés
Debian. Si un outil manque, le script affiche les dépendances à installer.

QEMU n’est pas nécessaire pour générer l’ISO ; il est requis uniquement pour
le test de démarrage décrit plus bas. Pour installer ce paquet séparément :

```bash
sudo apt install qemu-system-x86
```

## Génération de l’ISO

Depuis la racine du dépôt :

```bash
sudo ./build-live-iso.sh
```

Le script doit être exécuté en tant que `root` (par exemple avec `sudo`). Il
vérifie d’abord les sources du noyau et les outils nécessaires, compile les
applications personnalisées, configure puis compile le noyau Linux, prépare
le système live et fabrique l’image ISO. La progression des étapes est
affichée dans le terminal. La sortie dépend de l’architecture cible :

```bash
dist/nonoos-live-amd64.iso
dist/nonoos-live-arm64.iso
```

L’architecture peut être forcée avec `TARGET_ARCH` avant le lancement ; les
aliases `x86_64` et `aarch64` sont également acceptés :

```bash
sudo TARGET_ARCH=amd64 ./build-live-iso.sh
sudo TARGET_ARCH=x86_64 ./build-live-iso.sh
sudo TARGET_ARCH=arm64 ./build-live-iso.sh
sudo TARGET_ARCH=aarch64 ./build-live-iso.sh
```

Le répertoire de travail par défaut est `.build/nonoos-live/`. Il peut être
déplacé en définissant `BUILD_DIR` avant de lancer le script. Les variables
`DEBIAN_MIRROR` et `APT_HTTP_PROXY` permettent respectivement de choisir un
miroir Debian et un proxy HTTP APT lors de la construction. Par exemple :

```bash
sudo TARGET_ARCH=arm64 BUILD_DIR=/var/tmp/nonoos-build \
  DEBIAN_MIRROR=https://deb.debian.org/debian \
  ./build-live-iso.sh
```

### Secure Boot

Secure Boot est désactivé par défaut. Pour le demander, fournissez une clé
privée et son certificat lisibles par le script, ainsi que l’outil `sbsign`
(paquet `sbsigntool`) :

```bash
sudo SECURE_BOOT=1 \
  SECURE_BOOT_KEY=/chemin/vers/MOK.key \
  SECURE_BOOT_CERT=/chemin/vers/MOK.crt \
  ./build-live-iso.sh
```

Avec cette option, le script sélectionne GRUB EFI, active la signature des
modules du noyau et signe l’image du noyau. Le certificat doit être inscrit
dans le firmware UEFI de la machine cible ; sans cela, le démarrage sécurisé
peut refuser les composants signés. Gardez la clé privée protégée et ne la
placez pas dans le dépôt.

## Vérifications automatiques

Les workflows GitHub Actions s’exécutent sur les changements ciblant la
branche `main` et touchant `rootfs/` ou les fichiers de workflow ; ils peuvent
aussi être lancés manuellement. Ils vérifient la syntaxe des scripts shell
sélectionnés dans `rootfs/` avec `bash -n`. Les applications C++ et le noyau
sont compilés pendant la génération de l’ISO, pas par ces workflows CI.

Les workflows sont définis dans [`.github/workflows/ci.yml`](.github/workflows/ci.yml)
et [`.github/workflows/di.yml`](.github/workflows/di.yml).

## Tester le démarrage

Après génération de l’ISO, vous pouvez vérifier le démarrage dans QEMU. Le
paquet `qemu-system-x86` doit être installé :

```bash
./tests/qemu-boot.sh
```

Le test démarre l’ISO sans interface graphique, avec l’accélération TCG, 2 Go
de mémoire et deux processeurs par défaut. Il collecte la sortie série et
signale certaines erreurs critiques de démarrage, notamment un kernel panic
ou un échec du montage de la racine. Après 90 secondes sans erreur détectée,
le délai d’attente est considéré comme une fin de test normale ; cela ne
remplace pas un test fonctionnel complet du bureau.

Les paramètres peuvent être adaptés par variables d’environnement :

```bash
QEMU_TIMEOUT_SECONDS=120 QEMU_MEMORY_MB=3072 QEMU_CPUS=4 \
  ./tests/qemu-boot.sh
```

Il est aussi possible de donner explicitement le chemin de l’ISO en premier
argument.

## Utilisation

### Démarrer en live

1. Écrivez l’ISO sur une clé USB avec un outil adapté, ou démarrez-la dans
   QEMU, VirtualBox ou VMware.
2. Sélectionnez le périphérique de démarrage dans le menu UEFI/BIOS.
3. Lancez la session live depuis le menu proposé.

L’image est configurée avec les paramètres régionaux français, un clavier
français et l’utilisateur live `nonoos`.

### Installation sur disque

Depuis la session live, ouvrez le lanceur d’installation NonoOS ou démarrez
Calamares depuis un terminal :

```bash
calamares
```

Dans l’assistant, vérifiez soigneusement le disque cible et les choix de
partitionnement avant de confirmer l’installation : ces opérations peuvent
modifier les données du disque. Calamares guide ensuite les choix de compte,
de fuseau horaire et de chargeur d’amorçage proposés par l’image.

### Persistance

La persistance live stocke les changements de la session sur une partition
ext4 dédiée. Identifiez avec soin la partition à utiliser avant d’exécuter :

```bash
sudo nonoos-create-persistence /dev/sdX3
```

Remplacez `/dev/sdX3` par le périphérique réel de la partition dédiée. Le
script refuse notamment une partition montée et demande une confirmation
interactive. **Son contenu sera entièrement effacé**, puis la partition sera
formatée en ext4 avec le label `persistence`. N’utilisez pas une partition
contenant des données à conserver. En mode non interactif, l’option `--yes`
confirme explicitement l’effacement. Le démarrage live doit inclure l’option
`persistence` ; elle figure dans la configuration de démarrage de l’image.

## Développement et personnalisation

Les principales zones de personnalisation sont :

- `rootfs/` pour les fichiers intégrés à l’environnement live, les services,
  les scripts et les outils NonoOS ;
- `linux-6.18/` pour la configuration et la compilation du noyau embarqué ;
- `build-live-iso.sh` pour les étapes de construction et les options de
  live-build ;
- `src/` pour les composants complémentaires, dont les outils Rust.

Après une modification de `rootfs/`, reconstruisez l’image pour l’intégrer à
l’ISO. Les composants compilés ne sont pas nécessairement copiés depuis leur
répertoire source directement : le script les construit dans le répertoire de
travail puis les installe dans le système live.

## Notes de conception

Le dépôt mélange plusieurs catégories de composants :

- sources du noyau Linux
- scripts de construction de live system
- outils de bureau Qt
- outils système en Bash et Rust
- composants de sécurité et maintenance

Cela permet de produire un système d’exploitation complet et personnalisable, tout en restant compatible avec la base Debian standard.

## Licence

Ce dépôt ne déclare pas de licence spécifique dans son état actuel. Si vous l’utilisez ou le redistribuez, vérifiez la licence des composants tiers et adaptez la documentation en fonction de votre usage.
