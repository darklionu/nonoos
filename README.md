# nonoos — Noyau Linux 7.2.6

Ce dépôt contient le code source du noyau Linux version 7.2.6, utilisé pour construire **nonoos**, un système d’exploitation basé sur Debian. Il comprend le noyau, ses pilotes, ses sous-systèmes, la gestion mémoire, le réseau, les systèmes de fichiers et les outils de développement.

Le noyau Linux est le cœur du système. Il pilote le matériel, gère les ressources du système, répartit les tâches, accède aux périphériques, gère les fichiers et les réseaux, et fournit les services de base utilisés par les programmes utilisateur.

---

## 1) Fichiers racine du projet

Les fichiers placés directement à la racine du dépôt servent à configurer, construire et documenter le noyau.

- .clang-format : règles de formatage du code C pour garder une présentation cohérente.
- .clippy.toml : configuration du linter Rust Clippy (si du code Rust est utilisé).
- .cocciconfig : configuration de Coccinelle, outil d’analyse et de refactoring du code.
- .editorconfig : règles d’édition et d’indentation pour les éditeurs.
- .get_maintainer.ignore : fichiers ignorés par l’outil de détection des mainteneurs.
- .gitattributes : gestion des attributs Git pour les fichiers du dépôt.
- .gitignore : fichiers et dossiers exclus du suivi Git.
- .mailmap : association des noms/e-mails des contributeurs pour normaliser les signatures.
- .pylintrc : configuration du linter Python (pour les scripts du dépôt).
- .rustfmt.toml : formatage automatique du code Rust.
- COPYING : licence du noyau Linux (GPLv2, avec mentions spécifiques).
- CREDITS : remerciements et contributions de développeurs et mainteneurs.
- Kbuild : mécanisme de construction du noyau basé sur make.
- Kconfig : système de configuration des options du noyau.
- MAINTAINERS : liste des mainteneurs par sous-système et par fichier.
- Makefile : point d’entrée principal pour compiler le noyau.
- README : présentation générale du projet.
- arch/ : code spécifique à chaque architecture CPU et plate-forme.
- block/ : couche de gestion des blocs, des disques et du stockage.
- certs/ : certificats et clés utilisées pour signer des modules et vérifier l’intégrité.
- crypto/ : algorithmes de chiffrement, hachage, compression et sécurité.
- Documentation/ : documentation technique du noyau, guides, API et procédures.
- drivers/ : pilotes matériels pour cartes réseau, USB, GPU, stockage, etc.
- fs/ : systèmes de fichiers (ext4, xfs, btrfs, procfs, sysfs, etc.).
- include/ : fichiers d’en-tête partagés utilisés dans le noyau.
- init/ : code de démarrage initial du noyau.
- io_uring/ : sous-système d’I/O asynchrone moderne.
- ipc/ : mécanismes inter-processus (pipes, semaphores, files de messages, mémoire partagée).
- kernel/ : cœur du noyau, ordonnancement, gestion des processus, temporisation, etc.
- lib/ : bibliothèques utilitaires du noyau.
- mm/ : gestion de la mémoire virtuelle et physique.
- net/ : pile réseau du noyau (IPv4, IPv6, TCP/IP, sockets, etc.).
- rust/ : support et composants écrits en Rust dans le noyau.
- samples/ : exemples de code pour démontrer des APIs ou des modules.
- scripts/ : scripts de génération, compilation, analyse et aide au développement.
- security/ : sécurité du noyau, LSM, intégrité, sandbox, contrôle d’accès.
- sound/ : sous-système audio (ALSA, drivers son, mixeurs, interfaces).
- tools/ : outils de diagnostics, de test, d’analyse et d’aide au développement.
- usr/ : fichiers intégrés pour usr/gen_init_cpio, initramfs, etc.
- virt/ : support de la virtualisation et de l’émulation.

---

## 2) Rôle des dossiers principaux

### arch/
C’est le cœur de l’architecture cible. Chaque sous-dossier correspond à une famille de processeurs ou à une plateforme.

Exemples :
- alpha/ : support Alpha.
- arm/ : architecture ARM 32 bits.
- arm64/ : architecture ARM 64 bits.
- riscv/ : architecture RISC-V.
- x86/ : architecture Intel/AMD x86 et x86_64.
- um/ : architecture User Mode Linux.
- sparc/, mips/, powerpc/, s390/, loongarch/, etc. : autres architectures.

Chaque dossier contient le code spécifique au CPU, le démarrage, les pilotes de plate-forme, les fichiers de configuration, le support de boot et les routines assembly spécifiques.

### block/
Gère la couche de stockage au niveau bloc. C’est là que le noyau coordonne les E/S de disque, les files d’attente d’I/O, les demandes de lecture/écriture et les planificateurs de bloc.

- bdev.c : gestion des blocs de périphériques.
- bio.c : structures et opérations de bio (block I/O requests).
- blk-core.c : noyau du système de blocs.
- blk-mq.c : gestion des files d’attente multiqueue.
- elevator.c : ordonnanceurs de I/O (CFQ, mq-deadline, BFQ, etc.).

### certs/
Contient les certificats et clés utilisées pour sécuriser les modules chargés et vérifier les signatures numériques.

### crypto/
Implémente les algorithmes cryptographiques utilisés par le noyau, comme :
- chiffrement symétrique
- hachage
- authentification
- compression
- modules de sécurité et VPN

### Documentation/
Ce dossier regroupe la documentation officielle du noyau.

Il contient :
- guides de développement
- documentation des sous-systèmes
- tutoriels de compilation
- API du noyau
- procédures de contribution
- documentation système et d’administration

### drivers/
C’est le plus volumineux dossier du noyau. Il contient tous les pilotes matériels, classés par type de périphérique ou de bus.

Exemples :
- gpu/
- net/
- usb/
- scsi/
- block/
- char/
- i2c/
- sound/
- platform/

Chaque pilote permet au noyau de communiquer avec un matériel spécifique : carte réseau, disque, clavier, caméra, GPU, Wi‑Fi, USB, etc.

### fs/
Contient les implémentations des systèmes de fichiers supportés par Linux.

Exemples :
- ext4/
- xfs/
- btrfs/
- fat/
- proc/
- sysfs/
- debugfs/

La logique qui lit, écrit, monte et interprète les fichiers est centralisée ici.

### include/
Dossier des fichiers d’en-tête publics du noyau. Il contient les structures, macros, définitions et interfaces utilisées par le code du noyau.

- include/linux/ : API générale du noyau
- include/uapi/ : interfaces entre noyau et espace utilisateur
- include/asm-*/ : headers spécifiques à chaque architecture

### init/
Contient le code exécuté au démarrage du noyau :
- initialisation du système
- passage en mode utilisateur
- lancement du premier processus
- configuration des sous-systèmes au boot

### io_uring/
Sous-système moderne d’entrées/sorties asynchrones. Il permet à des applications d’effectuer des I/O de manière plus performante et plus scalable que les appels classiques.

### ipc/
Implémente les mécanismes de communication entre processus.

Exemples :
- sémaphores
- files de messages
- mémoire partagée
- IPC en mode noyau

### kernel/
C’est le cœur du noyau. Ici se trouvent :
- ordonnanceur de tâches
- gestion des processus
- signaux
- timers
- modules
- systèmes de verrouillage
- gestion du temps

### lib/
Bibliothèques utilitaires du noyau, plus légères que les bibliothèques utilisateur. Exemples :
- listes
- tables de hachage
- routines de chaîne
- CRC
- utilitaires génériques

### mm/
Gestion de la mémoire du système.

Il contient :
- allocation mémoire
- pagination
- mémoire virtuelle
- swap
- page cache
- gestion des zones mémoire
- mécanismes de protection

### net/
Pile réseau du noyau. Elle inclut :
- sockets
- protocoles IP, TCP, UDP
- gestion des interfaces
- filtrage
- VLAN, bridges
- firewall et règles réseau

### rust/
Support de Rust dans le noyau. Il contient des composants, bibliothèques ou abstractions du noyau écrits avec le langage Rust, conformément à la prise en charge progressive de Rust par Linux.

### samples/
Exemples de code utilisables pour apprendre ou tester certaines APIs du noyau. Par exemple :
- module de démonstration
- exemple d’API
- code de référence simplifié

### scripts/
Scripts de construction, d’analyse et d’automatisation. On y trouve :
- génération de fichiers
- vérification de style
- intégration de modules
- outils d’assemblage du noyau
- scripts de build et de debug

### security/
Contient la couche de sécurité du noyau :
- LSM (Linux Security Modules)
- SELinux, AppArmor, Landlock, etc.
- contrôle d’accès
- intégrité du système
- mécanismes d’isolation

### sound/
Sous-système audio du noyau, surtout avec ALSA.

Il gère :
- cartes son
- pilotes audio
- mixage
- interfaces d’interaction avec les programmes

### tools/
Outils destinés aux développeurs et administrateurs système.

Exemples :
- diagnostics
- analyse de performance
- outils de tracing
- scripts de test

### usr/
Regroupe des éléments utiles pour la génération d’initramfs, de contenus intégrés au système ou d’images système minimales.

### virt/
Sous-système de virtualisation et d’émulation du noyau.

Il contient des composants liés à :
- KVM
- hyperviseurs
- abstraction de machines virtuelles

---

## 3) Construire et tester une ISO live

Le projet nonoos contient le code source du noyau Linux 7.2.6 et un overlay
`rootfs`. Le script `build-live-iso.sh` compile ce noyau pour amd64, installe
ses modules et son initramfs dans une ISO live Debian 13 (trixie) assemblée
avec `live-build`. Il copie également les fichiers de `rootfs` et active les
services de premier démarrage.
Le même overlay est intégré sous `linux-7.2.6/rootfs` ; la construction de
l'ISO continue d'utiliser le dossier `rootfs` à la racine du dépôt.
L'image inclut KDE Plasma et SDDM et propose une session live ainsi que
l'installateur graphique Debian pour installer le système sur un disque.
L'installateur copie le système live, y compris le noyau NonoOS compilé dans ce
dépôt, puis installe GRUB pour le démarrage en BIOS ou UEFI.

Sur un hôte Debian ou Ubuntu avec une connexion Internet et au moins 25 Go
d'espace libre, installez les outils de construction puis lancez :

```bash
sudo apt update
sudo apt install live-build debootstrap xorriso squashfs-tools \
  grub-pc-bin grub-efi-amd64-bin mtools dosfstools build-essential \
  bc bison flex libelf-dev libssl-dev openssl perl python3 kmod \
  qt6-base-dev
sudo bash build-live-iso.sh
```

L'image NonoOS sera créée dans `dist/nonoos-live-amd64.iso`. Au démarrage,
choisissez l'entrée **Graphical install** de Debian Installer pour lancer
l'installation guidée, ou l'entrée live pour essayer le système sans modifier
le disque. L'installateur permet de choisir le disque et le partitionnement ;
les opérations de partitionnement sont destructrices pour les partitions
sélectionnées et doivent être vérifiées attentivement avant confirmation.

Après installation, le système, les fichiers et les paquets installés sont
conservés sur le disque. Au premier démarrage installé, l'assistant NonoOS
propose de choisir la langue, le clavier, les services et les profils logiciels.
Le compte créé par Debian Installer est conservé ; l'assistant ne crée pas de
compte supplémentaire sur le système installé.
Les paquets sélectionnés sont téléchargés depuis les dépôts Debian. Si le profil
« Serveur » est sélectionné, l'assistant configure un démarrage en mode console
(`multi-user.target`), sans interface graphique. En mode live, les changements
restent temporaires et disparaissent au redémarrage.

### Centre graphique de logiciels et mises à jour

L'application **Logiciels et mises à jour NonoOS** est disponible dans le menu
du bureau KDE. Elle fournit deux fonctions :

- **Mises à jour** : actualise les index APT, affiche les paquets pouvant être
  mis à niveau, puis propose une confirmation avant d'exécuter
  `apt-get upgrade`. L'opération demande une autorisation administrateur via
  PolicyKit, conserve les configurations locales et ne supprime pas de paquets
  pour résoudre les dépendances.
- **Applications** : affiche les paquets décrits dans les profils de
  `rootfs/usr/share/newdistrib/packages/`, permet de filtrer par profil et nom,
  puis installe ou supprime les paquets sélectionnés après confirmation et
  autorisation administrateur. La suppression ne lance pas `autoremove` :
  les dépendances ne sont pas retirées automatiquement.

Les opérations utilisent les dépôts APT configurés sur le système. APT vérifie
les signatures des dépôts et paquets selon sa configuration Debian. Configurez
uniquement des dépôts de confiance. Cette fonction met à jour les paquets
Debian installés ; le noyau NonoOS compilé directement dans l’ISO n’est pas un
paquet APT et ne peut donc pas être mis à jour par cette interface. Une mise à
jour du noyau NonoOS nécessitera de publier et signer un paquet noyau dans un
dépôt configuré.

Les changements effectués dans la session live sont temporaires et disparaissent
au redémarrage. Après installation sur disque, les mises à jour et applications
installées restent présentes. La compilation de l'interface depuis le dépôt
nécessite Qt 6 Widgets sur l'hôte de construction ; le script
`build-live-iso.sh` inclut le binaire et les composants PolicyKit nécessaires
dans l'ISO.

Pour essayer l'image dans QEMU (4 Go de mémoire vive recommandés) :

```bash
qemu-system-x86_64 -m 4G -smp 2 \
  -cdrom dist/nonoos-live-amd64.iso \
  -boot d -nic user,model=virtio-net-pci
```

Le démarrage et l'installation de paquets nécessitent un accès réseau. L'ISO
démarre avec le noyau amd64 compilé depuis `linux-7.2.6` ; `live-build`
n'installe pas de noyau Debian en parallèle. Le build utilise la configuration
`x86_64_defconfig`, avec les systèmes de fichiers et pilotes nécessaires au
démarrage live activés en modules. L'image inclut aussi Debian Installer en
mode « live », qui installe une copie persistante de ce système sur le disque ;
il ne s'agit pas d'une installation Debian générique qui remplacerait NonoOS.

### Tests rapides avec Docker

Pour compiler et vérifier les fonctions essentielles de ChatIA (salutation,
calcul et détection USB) et de l'outil mémoire (RAM et CPU), depuis la racine
du dépôt :

```bash
docker build -f docker/Dockerfile -t nonoos-smoke .
docker run --rm nonoos-smoke
```

Ces tests valident les programmes en espace utilisateur dans Debian ; ils ne
compilent ni ne démarrent le noyau Linux. Les ISO générées dans `dist/` et les
binaires de test compilés à la racine sont ignorés par Git.

## 4) Gestion des paquets

Au premier démarrage, l'assistant [rootfs/etc/init.d/first-boot](rootfs/etc/init.d/first-boot) détecte et affiche le modèle de la machine, le processeur, la mémoire vive, les composants graphiques, les disques, les interfaces réseau et les périphériques USB. Le rapport est également ajouté à `/var/log/newdistrib-first-boot.log`. L'assistant permet ensuite de sélectionner un ou plusieurs profils : bureautique, développement, cybersécurité, vie privée, serveur, multimédia, éducation, jeux et personnalisé. Les paquets sélectionnés sont installés avec `apt-get`.

Chaque profil correspond à une liste de paquets dans [rootfs/usr/share/newdistrib/packages/](rootfs/usr/share/newdistrib/packages/). Pour personnaliser les applications proposées, modifiez le fichier `.list` correspondant avant de construire l'ISO. Le profil `custom` charge `custom.list`, prévu pour vos propres paquets. Une ligne contient un nom de paquet ; les lignes vides et celles commençant par `#` sont ignorées. Plusieurs profils peuvent être choisis et les doublons de paquets sont supprimés lors de l'installation.

La gestion des paquets système est également utilisée pour installer les composants optionnels, comme `openssh-server`, `bluez` et `network-manager`.

En cas d'échec de création du compte, d'activation d'un service ou d'installation,
la configuration n'est pas marquée comme terminée et le script de premier
démarrage est conservé. Si seule l'installation des paquets a échoué après la
configuration du compte et des services, relancez-la avec
`sudo /etc/init.d/first-boot --install-only`. Si une autre étape a échoué,
relancez `/etc/init.d/first-boot` sans option pour reprendre l'assistant. Une
nouvelle tentative n'est considérée comme terminée qu'après la réussite des
étapes requises.

Cela signifie que le projet n’a pas seulement le noyau Linux, mais aussi une couche d’installation logicielle déjà prévue pour l’environnement final.

---

## Compiler le centre graphique depuis les sources

Sur Debian ou Ubuntu, avec Qt 6 Widgets installé, compilez l'application depuis
la racine du dépôt :

```bash
sudo apt install g++ pkg-config qt6-base-dev
g++ -std=c++17 -Wall -Wextra -pedantic \
  "rootfs/gestionnaire de mise à jour/interface_graphique.cpp" \
  $(pkg-config --cflags --libs Qt6Widgets) \
  -o nonoos-software-center
./nonoos-software-center
```

Pour exécuter les opérations APT depuis cette compilation locale, le système
doit également disposer de `pkexec`, `polkitd` et d'un agent d'authentification
PolicyKit dans la session graphique.

---

## 5) Centre de contrôle mémoire

Le dépôt contient un tableau de bord C++/Qt qui affiche la RAM utilisée et les
processus triés par consommation mémoire :

- `linux-7.2.6/nonoos-info/système _principal.cpp`
- `rootfs/usr/src/memory/ram_memory.h`

Les mesures sont lues dans `/proc/meminfo` et `/proc/<PID>/status`. Les
exécutables sont identifiés avec `/proc/<PID>/exe` lorsqu'ils sont accessibles.
Installez Qt 6 Widgets et compilez depuis la racine du dépôt :

```bash
sudo apt install g++ pkg-config qt6-base-dev
g++ -std=c++17 -Wall -Wextra -pedantic \
  -I rootfs/usr/src/memory \
  "linux-7.2.6/nonoos-info/système _principal.cpp" \
  $(pkg-config --cflags --libs Qt6Widgets) \
  -o nonoos-info
./nonoos-info
```

La fenêtre s’actualise automatiquement toutes les deux secondes. Les processus
dont les informations ne sont pas accessibles à l’utilisateur courant peuvent
être absents de la liste.

L’outil en ligne de commande RAM/CPU reste disponible séparément :

```bash
g++ -std=c++17 -Wall -Wextra -pedantic \
  -I rootfs/usr/src/memory rootfs/usr/src/memory/ram_memory.cpp \
  -o ram-memory
./ram-memory
```

---

## 6) Les fichiers de configuration et de build

Voici les fichiers les plus importants pour compiler et configurer le noyau :

- Makefile : commande principale pour construire le noyau.
- Kconfig : arbre de configuration pour activer/désactiver fonctionnalités.
- Kbuild : mécanisme de génération des fichiers objets et de la compilation.
- scripts/ : aides de compilation, génération, validation.
- Documentation/ : guides de build et d’utilisation.

Pour configurer le noyau :
- make menuconfig
- make olddefconfig
- make xconfig
- make defconfig

Pour compiler :
- make
- make modules
- make install

---

## 7) Vue d’ensemble du rôle du noyau Linux

Le noyau Linux est composé de plusieurs sous-systèmes :

- gestion du CPU et des processus
- mémoire virtuelle et physique
- fichiers et systèmes de fichiers
- réseau
- pilotes matériels
- sécurité
- audio
- stockage
- virtualisation

Chaque dossier du projet correspond à un de ces sous-systèmes, et les fichiers qui y vivent implémentent les mécanismes du noyau nécessaires au bon fonctionnement d’un système Linux.

---

## 8) Résumé très court

- arch/ : code spécifique au matériel
- block/ : I/O bloc
- drivers/ : pilotes matériels
- fs/ : systèmes de fichiers
- include/ : entêtes du noyau
- kernel/ : cœur du noyau
- mm/ : mémoire
- net/ : réseau
- security/ : sécurité
- sound/ : audio
- Documentation/ : documentation
- scripts/ : outils et automatisation

---

## 9) À retenir

Ce dépôt n’est pas un simple programme : c’est l’arbre source complet du noyau Linux. Il sert à construire un système d’exploitation, à adapter Linux à des architectures spécifiques, à ajouter des pilotes matériels, à étendre la sécurité, à gérer les E/S et à exposer des services à l’espace utilisateur.

Ce README est une vue d’ensemble pratique du rôle des dossiers. Pour des détails précis sur un sous-système donné, il faut se référer au dossier correspondant et à la documentation dans Documentation/.
