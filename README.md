# NONOOS

NonoOS est une distribution Linux live basée sur Debian et optimisée pour les environnements de bureau, la gestion système et la sécurité. Le projet combine un noyau Linux 7.2.6, un système de live build Debian/Ubuntu, un environnement graphique KDE Plasma et des outils personnalisés pour administrer le système, gérer les mises à jour et sécuriser l’installation.

Le dépôt sert à construire une image ISO bootable pour x86_64, avec une configuration par défaut en français, des outils de maintenance intégrés et une base système prête à personnaliser.

## Fonctionnalités principales

- ISO live amd64 construite automatiquement
- Noyau Linux 7.2.6 avec modules standards pour le stockage, le réseau et le boot live
- Base Debian trixie avec KDE Plasma, SDDM, NetworkManager, sudo et outils système
- Outils personnalisés :
  - gestionnaire de démarrage
  - centre de contrôle des mises à jour
  - centre de sécurité
  - pare-feu UFW
  - tableau de bord de maintenance
  - assistant ChatIA, scanner Wi-Fi et outils de diagnostic USB et mémoire
  - scripts de création d’utilisateur et de tests
- Configuration automatique au premier démarrage
- Persistance live optionnelle sur une partition dédiée
- Support de l’interface française et du clavier AZERTY

## Vue d’ensemble du projet

### Structure du dépôt

- `build-live-iso.sh` : script principal de génération de l’image ISO
- `linux-7.2.6/` : sources du noyau Linux 7.2.6
- `rootfs/` : fichiers du système live et outils personnalisés
- `src/` : composants logiciels complémentaires (ex. Rust pour la sécurité)
- `docker/` : outils de conteneurisation/tests
- `dist/` : sortie de la construction ISO (généré lors du build)

### Exemple de contenu inclus

- `rootfs/etc/init.d/first-boot` : configuration initiale du système
- `rootfs/etc/init.d/every-boot` : tâches de maintenance à chaque démarrage
- `rootfs/apps/gestionnaire-demarrage/` : gestionnaire de démarrage graphique
- `rootfs/update-center/` : centre de mise à jour
- `rootfs/gestionnaire de sécurité/` : console de sécurité / pare-feu
- `rootfs/usr/src/` : sources de ChatIA et des outils USB et mémoire
- `rootfs/usr/migration-assistant/` : détection en lecture seule des systèmes installés sur les partitions
- `rootfs/usr/local/sbin/nonoos-migration-assistant` : récupération facultative des dossiers personnels proposée au premier démarrage
- `src/nonoos-security.rs` : outil de supervision système en Rust
- `.github/workflows/` et `docker/` : vérifications automatisées des composants

## Prérequis de compilation

Le build utilise `live-build`, un environnement Debian/Ubuntu et des outils de compilation C/C++/Rust.

Sur Debian/Ubuntu, installez les dépendances suivantes :

```bash
sudo apt update
sudo apt install \
  live-build debootstrap xorriso squashfs-tools \
  grub-pc-bin grub-efi-amd64-bin mtools dosfstools \
  build-essential bc bison flex libelf-dev libssl-dev \
  openssl perl python3 rustc kmod qt6-base-dev \
  pkg-config qemu-system-x86
```

Le script vérifie aussi la présence de `lb`, `debootstrap`, `xorriso`, `mksquashfs`, `gcc`, `g++`, `make`, `depmod`, `nproc`, ainsi que le paquet Qt6Widgets.

## Vérifications automatiques

Deux workflows GitHub Actions vérifient les composants du dépôt sur Ubuntu :

- `.github/workflows/ci.yml` vérifie la syntaxe des scripts shell de `rootfs/`, contrôle la syntaxe de tous les fichiers C++ de `rootfs/`, puis construit et exécute les tests de fumée Docker.
- `.github/workflows/di.yml` compile séparément les trois applications graphiques Qt, compile le scanner Wi-Fi, vérifie le module mémoire, compile l’outil USB, vérifie la syntaxe des scripts shell et exécute les mêmes tests de fumée Docker.

Ces workflows se déclenchent sur les push et pull requests vers `main` lorsqu’un fichier correspondant à leurs filtres est modifié. Ils peuvent aussi être lancés manuellement depuis l’onglet **Actions** de GitHub. Les tests Docker s’exécutent avec le réseau désactivé et vérifient notamment les réponses de ChatIA (salutation, calcul et détection USB) ainsi que les mesures RAM et CPU de l’outil mémoire.

## Génération de l’ISO

Depuis la racine du dépôt :

```bash
sudo ./build-live-iso.sh
```

Le script affiche une barre de progression par grandes étapes tout en
conservant les journaux détaillés de compilation. Le répertoire
`.build/nonoos-live/` est conservé entre les exécutions afin de reprendre les
étapes déjà réussies ; utilisez `BUILD_DIR=/chemin/personnalise` pour changer
son emplacement.

Le miroir Debian et le proxy APT peuvent être adaptés à un réseau local ou à
un cache déjà disponible :

```bash
sudo DEBIAN_MIRROR=http://miroir-local/debian \
  APT_HTTP_PROXY=http://proxy-local:3142 \
  ./build-live-iso.sh
```

Le proxy est facultatif. Un accès réseau au miroir reste nécessaire si les
paquets ne sont pas déjà disponibles dans le cache de live-build.

Le script :

1. compile les outils Qt et Rust personnalisés,
2. configure le noyau Linux 7.2.6 pour x86_64,
3. crée un environnement live Debian,
4. injecte le contenu de `rootfs/`,
5. construit l’ISO hybride final.

La sortie est générée dans :

```bash
dist/nonoos-live-amd64.iso
```

### Test de démarrage avec QEMU

Après la construction de l’ISO, vérifiez son démarrage sans machine virtuelle
graphique :

```bash
./tests/qemu-boot.sh
```

Le script démarre l’ISO avec un disque virtuel, une carte réseau VirtIO et une
console série. Il surveille les erreurs critiques du noyau et de l’initramfs,
puis arrête automatiquement QEMU après 90 secondes. Une autre ISO peut être
testée en la donnant comme argument :

```bash
./tests/qemu-boot.sh /chemin/vers/nonoos-live-amd64.iso
```

La mémoire, le nombre de processeurs et le délai peuvent être ajustés avec
`QEMU_MEMORY_MB`, `QEMU_CPUS` et `QEMU_TIMEOUT_SECONDS`.

## Utilisation

### Démarrage

- gravez l’image ISO sur un support USB,
- ou exécutez-la depuis un environnement de virtualisation compatible (QEMU, VirtualBox, VMware, etc.),
- choisissez le système bootable et lancez la session live.

### Persistance de la session live

L’ISO est construite avec le support `live-boot` et demande automatiquement
une couche de persistance grâce à l’option de démarrage `persistence`. Pour
conserver les fichiers et les réglages entre deux démarrages, préparez une
partition dédiée depuis la session live :

```bash
sudo nonoos-create-persistence /dev/sdX3
```

Remplacez `/dev/sdX3` par la partition destinée à la persistance. Cette
commande formate entièrement la partition en ext4 : vérifiez impérativement
le périphérique avant de confirmer. Pour une utilisation non interactive,
l’option destructive doit être explicite :

```bash
sudo nonoos-create-persistence --yes /dev/sdX3
```

La partition reçoit le label `persistence` et le fichier de configuration
`/persistence.conf` avec la règle `/ union`. Elle doit être séparée du support
contenant l’ISO. Le script refuse une partition déjà montée.

Pour désactiver la persistance lors d’un démarrage ponctuel, éditez les
paramètres de démarrage et retirez `persistence`.

### Configuration initiale

Au premier démarrage, le système peut proposer une configuration de langue, clavier, environnements et paquets selon le profil choisi. Les scripts de premier boot sont définis dans :

- `rootfs/etc/init.d/first-boot`
- `rootfs/etc/init.d/every-boot`

### Outils présents dans le live système

- `nonoos-software-center` : gestion des logiciels / mises à jour
- `nonoos-security-center` : interface de sécurité
- `nonoos-startup-manager` : gestion des éléments au démarrage
- `nonoos-security` : surveillance / sécurité système
- `nonoos-firewall` : configuration de UFW
- `nonoos-dashboard` : tableau de bord système
- `nonoos-create-test-user` : création d’un compte de test
- `nonoos-test-shell` : vérification de l’environnement shell

### Détection des systèmes installés

Le module C++ `rootfs/usr/migration-assistant/detector.hpp` expose
`detect_installed_os()`. Il inspecte les partitions Linux, LVM et chiffrées
déjà déverrouillées, monte celles qui ne sont pas déjà montées en lecture seule,
et reconnaît les installations Linux (`os-release`), Windows et macOS. Le
résultat fournit les systèmes trouvés ainsi que les erreurs de partitions qui
n’ont pas pu être inspectées. L’inventaire et le montage nécessitent les outils
`lsblk` et les privilèges root ; les volumes chiffrés doivent être déverrouillés
au préalable.

Après la création du compte principal, le premier démarrage propose aussi de
récupérer `Documents`, `Images`, `Videos`, `Music`, `Desktop` et `Downloads`
depuis une installation contenant un dossier `/home`. La source est montée en
lecture seule lorsqu’elle ne l’est pas déjà, les fichiers existants ne sont pas
écrasés et les erreurs sont consignées dans
`/var/log/newdistrib-migration.log`. Cette étape peut être annulée sans bloquer
la suite de la configuration.

## Personnalisation

Le projet est conçu pour être facilement adapté :

- ajouter des paquets dans `build-live-iso.sh` sous `config/package-lists/`,
- modifier le contenu du système dans `rootfs/`,
- personnaliser le noyau avec les options de `make` et `scripts/config`,
- ajouter des hooks live via `config/hooks/live/`.

## Notes de développement

Le dépôt mélange plusieurs niveaux de conception :

- sources du noyau Linux officiel,
- scripts de génération de live system,
- composants graphiques Qt,
- outils de sécurité écrits en Rust et Bash.

Cette structure permet de construire un système d’exploitation complet et personnalisable, tout en conservant la base Linux standard et ses outils de maintenance.

## Remarques

Ce projet est un environnement expérimental de distribution Linux. Il est destiné à la conception, le test et l’adaptation de systèmes live personnalisés. Les sources du noyau restent celles de Linux et doivent être utilisées dans le respect de leurs licences et de leur documentation officielle.
