# NonoOS / Newdistrib

NonoOS est une distribution Linux live basée sur Debian, conçue pour servir de base à un environnement de bureau orienté maintenance, sécurité, diagnostics et administration système.

Le dépôt construit une ISO bootable pour les architectures amd64/x86_64 et arm64/aarch64, en embarquant un noyau Linux 6.18, une base Debian live, un environnement KDE Plasma, ainsi qu’un ensemble d’outils spécifiques à la gestion de la machine, des profils utilisateurs, de la sécurité, du Wi‑Fi, de la mémoire, de l’USB et de la migration de système.

La logique du projet est simple :
- `build-live-iso.sh` prépare l’image ISO complète ;
- `rootfs/` contient le contenu de la distribution live (scripts, applications, configuration, lanceurs) ;
- `linux-6.18.tar.xz` contient les sources du noyau Linux utilisées pour la build ;
- `src/` contient les composants complémentaires (notamment la sécurité système en Rust) ;
- `tests/` contient les vérifications de démarrage et les scripts de validation ;
- `dist/` reçoit le livrable final de l’ISO.

Pour la documentation détaillée du contenu intégré dans l’image, voir aussi [`rootfs/README.md`](rootfs/README.md).

## Arborescence du dépôt

```text
.
├── build-live-iso.sh              # script principal de génération de l'ISO
├── docker/                       # fichiers Docker et tests de fumée
├── dist/                         # sortie de build (ISO produite)
├── linux-6.18.tar.xz             # archive des sources du noyau Linux 6.18
├── rootfs/                       # contenu du système live / outils personnalisés
├── src/                          # composants Rust et outils système
├── tests/                        # validation du boot et smoke tests
├── .github/workflows/            # CI GitHub Actions
├── README.md                     # documentation du projet
├── .gitignore
├── .build/                       # travail de compilation (généré)
└── ...
```

### Sous-arborescence utile de `rootfs/`

```text
rootfs/
├── apps/                               # applications du bureau NonoOS
├── etc/                                # configuration, scripts de premier démarrage, services systemd
├── gestion-alimentations/              # gestion de l’alimentation
├── gestionnaire de sécurité/            # centre de sécurité et outils associés
├── nonoos-info/                         # tableau de bord système
├── update-center/                       # centre de logiciels / mises à jour
├── usr/
│   ├── local/bin/                      # commandes utilisateur (nonoos-hardware, nonoos-profile)
│   ├── local/sbin/                     # commandes système (persistance, migration)
│   ├── migration-assistant/            # assistant de migration et détection système
│   ├── share/applications/             # lanceurs KDE des outils NonoOS
│   ├── share/internet/                 # module Wi‑Fi et documentation
│   ├── src/ia/                         # sources de ChatIA
│   ├── src/USB/                        # outils USB / diagnostics
│   └── src/memory/                     # outils de diagnostic mémoire
├── README.md                           # documentation du contenu intégré
└── ...
```

## Ce que contient NonoOS

### Environnement système

- base Debian live avec build de l’image via `live-build` ;
- support des architectures `amd64` et `arm64` ;
- environnement desktop KDE Plasma + SDDM ;
- configuration française par défaut ;
- clavier AZERTY et paramètres régionaux prêts à l’emploi ;
- gestion réseau via NetworkManager ;
- services de premier démarrage et de démarrage à chaque boot ;
- mécanisme de persistance live possible via une partition dédiée.

### Outils intégrés

- `nonoos-startup-manager` : gestionnaire de démarrage, lancé depuis `nonoos-startup-manager.desktop` ;
- `nonoos-software-center` : centre de logiciels et de mises à jour, lancé depuis `nonoos-software-center.desktop` ;
- `nonoos-security-center` : centre de sécurité, lancé depuis `nonoos-security-center.desktop` ;
- `nonoos-power` / `nonoos-power-gui` : gestion de l’alimentation, lancée depuis `nonoos-power.desktop` ;
- `nonoos-dashboard` : tableau de bord système, lancé depuis `nonoos-system-dashboard.desktop` ;
- `nonoos-hardware` : inventaire matériel, lancé depuis `nonoos-hardware.desktop` ;
- `nonoos-profile` : sélection et consultation des profils utilisateur, lancé depuis `nonoos-user-profile.desktop` ;
- `nonoos-create-persistence` : création d’une partition de persistance ;
- `nonoos-migration-assistant` : assistance à la migration des données et détection des systèmes installés ;
- ChatIA et outils de diagnostic Wi‑Fi / USB / mémoire.

### Profils utilisateur

Au premier démarrage, vous pouvez sélectionner un ou plusieurs profils
d’applications : **Bureautique**, **Développeur**, **Cybersécurité**, **Vie
privée**, **Serveur**, **Multimédia**, **Éducation**, **Jeux** et
**Personnalisé**. Chaque profil installe les paquets définis dans
`rootfs/usr/share/newdistrib/packages/`. Les fichiers disponibles sont
`office.list`, `developing.list`, `cybersec.list`, `vieprivee.list`,
`serveur.list`, `multimedia.list`, `education.list`, `gaming.list` et
`custom.list`. Cette dernière peut être modifiée avant de construire l’ISO
pour choisir les paquets du profil Personnalisé.

Ces profils d’applications sont distincts du profil personnel ci-dessous,
qui ne détermine pas les logiciels installés.

Au premier démarrage, NonoOS propose des profils personnalisés :
- Standard
- Développeur
- Support technique

Le choix est enregistré séparément pour chaque compte dans `~/.config/nonoos/profile` et peut être modifié via le lanceur `Profils utilisateur NonoOS` ou la commande :

```bash
nonoos-profile --show
nonoos-profile
```

Ce profil ne modifie pas les privilèges du compte ni les logiciels installés. Il sert surtout à personnaliser l’expérience et les outils proposés à l’utilisateur.

## Prérequis de construction

La génération doit être faite sur une machine Debian ou Ubuntu compatible amd64/x86_64 ou arm64/aarch64, avec accès aux dépôts Debian.

Installer les dépendances de base :

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

Le script vérifie automatiquement la présence de :
- `lb`, `debootstrap`, `xorriso`, `mksquashfs` ;
- `gcc`, `g++`, `make`, `bc`, `bison`, `flex` ;
- `openssl`, `depmod`, `perl`, `python3`, `rustc`, `nproc` ;
- `Qt 6 Widgets` via `pkg-config` ;
- les en-têtes `libelf` et le trousseau de clés Debian.

Si un binaire manque, le script affiche les dépendances à installer.

Pour tester le démarrage dans QEMU, il faut aussi installer :

```bash
sudo apt install qemu-system-x86
```

## Génération de l’image ISO

Depuis la racine du dépôt :

```bash
sudo ./build-live-iso.sh
```

Le script doit être lancé en tant que `root` (par exemple via `sudo`). Il :
1. vérifie les prérequis et les dépendances ;
2. compile les applications Qt personnalisées ;
3. compile la version Rust de la sécurité système ;
4. configure et compile le noyau Linux 6.18 ;
5. assemble le système live Debian ;
6. construit l’ISO finale dans `dist/`.

Les sorties attendues sont :

```bash
dist/nonoos-live-amd64.iso
dist/nonoos-live-arm64.iso
```

### Choisir l’architecture cible

```bash
sudo TARGET_ARCH=amd64 ./build-live-iso.sh
sudo TARGET_ARCH=x86_64 ./build-live-iso.sh
sudo TARGET_ARCH=arm64 ./build-live-iso.sh
sudo TARGET_ARCH=aarch64 ./build-live-iso.sh
```

### Personnaliser le build

Le script prend en charge plusieurs variables d’environnement :

```bash
sudo TARGET_ARCH=arm64 BUILD_DIR=/var/tmp/nonoos-build \
  DEBIAN_MIRROR=https://deb.debian.org/debian \
  ./build-live-iso.sh
```

Variables utiles :
- `TARGET_ARCH` : architecture cible (`amd64`, `arm64`, `x86_64`, `aarch64`)
- `BUILD_DIR` : dossier de travail du build
- `DEBIAN_MIRROR` : miroir Debian utilisé
- `APT_HTTP_PROXY` : proxy HTTP APT si nécessaire
- `SECURE_BOOT` : active la configuration Secure Boot (`0` par défaut)

### Secure Boot

Secure Boot est désactivé par défaut. Pour l’activer, fournis une clé privée et son certificat lisibles par le script, ainsi que l’outil `sbsign` (paquet `sbsigntool`) :

```bash
sudo SECURE_BOOT=1 \
  SECURE_BOOT_KEY=/chemin/vers/MOK.key \
  SECURE_BOOT_CERT=/chemin/vers/MOK.crt \
  ./build-live-iso.sh
```

L’option `SECURE_BOOT=1` force un boot EFI, active la signature de modules noyau et signe les composants nécessaires à l’amorçage sécurisé. Le certificat doit être inscrit dans le firmware UEFI de la cible ; sans cela, le démarrage sécurisé peut être refusé.

## Vérifications automatiques

Le dépôt inclut des workflows GitHub Actions pour valider les changements sur `rootfs/` et les fichiers CI associés :
- [`.github/workflows/ci.yml`](.github/workflows/ci.yml)
- [`.github/workflows/di.yml`](.github/workflows/di.yml)

Les vérifications de CI font notamment des contrôles de syntaxe sur les scripts shell de `rootfs/` avec `bash -n`.

## Tester le démarrage

Après génération de l’ISO, il est possible de vérifier rapidement le démarrage dans QEMU :

```bash
./tests/qemu-boot.sh
```

Le script fonctionne avec l’ISO générée par défaut et collecte la sortie série pour détecter certains évènements critiques : crash du noyau, échec du montage racine, etc.

Il est possible d’adapter la durée ou la mémoire allouée :

```bash
QEMU_TIMEOUT_SECONDS=120 QEMU_MEMORY_MB=3072 QEMU_CPUS=4 \
  ./tests/qemu-boot.sh
```

## Documentation du contenu intégré

Des sous-documentations existent déjà dans le dépôt :
- [`rootfs/README.md`](rootfs/README.md)
- [`rootfs/usr/src/ia/README.md`](rootfs/usr/src/ia/README.md)
- [`rootfs/usr/share/internet/README.md`](rootfs/usr/share/internet/README.md)
- [`rootfs/usr/migration-assistant/README.md`](rootfs/usr/migration-assistant/README.md)

## Utilisation

### Démarrer en live

1. Écrivez l’ISO sur une clé USB avec un outil adapté, ou démarrez-la dans QEMU, VirtualBox ou VMware.
2. Sélectionnez le périphérique de démarrage dans le menu UEFI/BIOS.
3. Lancez la session live depuis le menu proposé.

L’image est configurée avec les paramètres régionaux français, un clavier français et l’utilisateur live `nonoos`.

### Récupération de fichiers au premier démarrage

Lors de la configuration initiale, après la création ou la détection du compte
utilisateur, NonoOS propose de récupérer des fichiers depuis une ancienne
installation. L’assistant recherche les partitions contenant un dossier
`/home`, puis permet de choisir les catégories personnelles à copier. La source
est montée en lecture seule et les fichiers déjà présents dans le dossier
personnel de destination ne sont pas écrasés.

Cette étape est facultative et ne se lance qu’au premier démarrage : le service
`newdistrib-first-boot.service` exécute `rootfs/etc/init.d/first-boot`, qui
appelle `nonoos-migration-assistant`. Pour intégrer une modification de
l’assistant à l’image, reconstruisez l’ISO après avoir modifié les fichiers
correspondants dans `rootfs/`.

Après la configuration initiale, l’assistant peut aussi être ouvert au
démarrage en appuyant sur **Ctrl+Alt+7** pendant les cinq secondes affichées
sur le terminal de démarrage, avant l’écran de connexion. Le raccourci lance
le même assistant, qui demande le compte de destination avant de proposer la
partition source et les catégories à récupérer. Sans raccourci, le démarrage
continue normalement après ce bref délai.

### Installation sur disque

Depuis la session live, ouvrez le lanceur d’installation NonoOS ou démarrez Calamares depuis un terminal :

```bash
calamares
```

Dans l’assistant, vérifiez soigneusement le disque cible et les choix de partitionnement avant de confirmer l’installation : ces opérations peuvent modifier les données du disque. Calamares guide ensuite les choix de compte, de fuseau horaire et de chargeur d’amorçage proposés par l’image.

### Persistance

La persistance live stocke les changements de la session sur une partition ext4 dédiée. Identifiez avec soin la partition à utiliser avant d’exécuter :

```bash
sudo nonoos-create-persistence /dev/sdX3
```

Remplacez `/dev/sdX3` par le périphérique réel de la partition dédiée. Le script refuse notamment une partition montée et demande une confirmation interactive. **Son contenu sera entièrement effacé**, puis la partition sera formatée en ext4 avec le label `persistence`. N’utilisez pas une partition contenant des données à conserver. En mode non interactif, l’option `--yes` confirme explicitement l’effacement. Le démarrage live doit inclure l’option `persistence` ; elle figure dans la configuration de démarrage de l’image.


## Développement et personnalisation

Les principales zones de personnalisation sont :

- `rootfs/` pour les fichiers intégrés à l’environnement live, les services, les scripts et les outils NonoOS ;
- `linux-6.18.tar.xz` pour les sources du noyau embarqué ;
- `build-live-iso.sh` pour les étapes de construction et les options de live-build ;
- `src/` pour les composants complémentaires, dont les outils Rust.

Après une modification de `rootfs/`, reconstruisez l’image pour l’intégrer à l’ISO. Les composants compilés ne sont pas nécessairement copiés depuis leur répertoire source directement : le script les construit dans le répertoire de travail puis les installe dans le système live.

## Notes de conception

Le dépôt mélange plusieurs catégories de composants :

- sources du noyau Linux ;
- scripts de construction de live system ;
- outils de bureau Qt ;
- outils système en Bash et Rust ;
- composants de sécurité et maintenance.

Cela permet de produire un système d’exploitation complet et personnalisable, tout en restant compatible avec la base Debian standard.

## Licence

Une copie de la licence **GNU GPL version 3** est actuellement présente dans
[`rootfs/LICENSE`](rootfs/LICENSE). Elle couvre donc les fichiers concernés
de l’arborescence `rootfs/` selon la déclaration appliquée par le projet.

Le dépôt ne possède pas encore de licence globale clairement associée à tous
ses composants. Les composants tiers, notamment Debian, le noyau Linux, Qt,
Rust, les applications intégrées et les paquets fournis par les distributions,
conservent leurs propres licences. Avant toute redistribution ou utilisation
commerciale, vérifiez donc séparément les droits applicables à chaque élément
et à l’ISO complète.
