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
- `rootfs/gestionnaire de mise à jour/` : centre de mise à jour
- `rootfs/gestionnaire de sécurité/` : console de sécurité / pare-feu
- `rootfs/usr/src/` : sources de ChatIA et des outils USB et mémoire
- `rootfs/usr/assistance de migration/` : détection en lecture seule des systèmes installés sur les partitions
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
  pkg-config
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

## Utilisation

### Démarrage

- gravez l’image ISO sur un support USB,
- ou exécutez-la depuis un environnement de virtualisation compatible (QEMU, VirtualBox, VMware, etc.),
- choisissez le système bootable et lancez le live session.

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

Le module C++ `rootfs/usr/assistance de migration/detector.hpp` expose
`detect_installed_os()`. Il inspecte les partitions Linux, LVM et chiffrées
déjà déverrouillées, monte celles qui ne sont pas déjà montées en lecture seule,
et reconnaît les installations Linux (`os-release`), Windows et macOS. Le
résultat fournit les systèmes trouvés ainsi que les erreurs de partitions qui
n’ont pas pu être inspectées. L’inventaire et le montage nécessitent les outils
`lsblk` et les privilèges root ; les volumes chiffrés doivent être déverrouillés
au préalable.

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
