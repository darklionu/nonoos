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

Le projet nonoos contient le code source du noyau et un overlay `rootfs`, mais pas
encore une image système complète. Le script `build-live-iso.sh` assemble une
ISO live Debian 13 (trixie) amd64 avec `live-build`, copie les fichiers de
`rootfs` dans le système live et active les services de premier démarrage.
Le même overlay est intégré sous `linux-7.2.6/rootfs` ; la construction de
l'ISO continue d'utiliser le dossier `rootfs` à la racine du dépôt.
L'image inclut KDE Plasma et SDDM par défaut et démarre en session live ; elle
n'installe pas le système sur le disque.

Sur un hôte Debian ou Ubuntu avec une connexion Internet et au moins 15 Go
d'espace libre, installez les outils de construction puis lancez :

```bash
sudo apt update
sudo apt install live-build debootstrap xorriso squashfs-tools \
  grub-pc-bin grub-efi-amd64-bin mtools dosfstools
sudo bash build-live-iso.sh
```

L'image nonoos sera créée dans `dist/nonoos-live-amd64.iso`. KDE Plasma est inclus
dans l'image. Au premier démarrage, l'assistant propose de choisir la langue,
le clavier et les logiciels ; les paquets choisis sont téléchargés depuis les
dépôts Debian et, dans une session live, les modifications restent temporaires.

### Vérifier les mises à jour avec KDE Discover

KDE Discover n'est pas encore inclus dans la liste des paquets de
`build-live-iso.sh`. Pour l'essayer dans une session live, connectez-vous à
Internet puis installez-le depuis un terminal :

```bash
sudo apt update
apt-cache search '^plasma-discover'
sudo apt install plasma-discover
```

Ouvrez ensuite **Discover** depuis le menu des applications, choisissez
l'onglet **Mises à jour** et lancez la recherche. L'installation des mises à
jour demandera une autorisation administrateur. Pour inclure Discover dans
chaque nouvelle ISO, ajoutez `plasma-discover` à la liste des paquets dans
`build-live-iso.sh`, puis reconstruisez l'image. Vérifiez avec `apt-cache
search` si Debian propose un paquet de backend Discover distinct dans les
sources configurées.

Cette opération met à jour les paquets Debian installés, pas l'image ISO ni
directement le noyau source du dépôt. Comme le système démarre en session live
sans persistance, les paquets installés et leurs mises à jour sont perdus au
redémarrage.

Pour essayer l'image dans QEMU (4 Go de mémoire vive recommandés) :

```bash
qemu-system-x86_64 -m 4G -smp 2 \
  -cdrom dist/nonoos-live-amd64.iso \
  -boot d -nic user,model=virtio-net-pci
```

Le démarrage et l'installation de paquets nécessitent un accès réseau. Cette
ISO de test est construite avec le noyau Debian fourni par `live-build` ; le
noyau source `linux-7.2.6` du dépôt n'est pas encore compilé ni intégré à l'ISO.

## 4) Gestion des paquets

La gestion des paquets est déjà présente dans le système racine. Le script de démarrage [rootfs/etc/init.d/first-boot](rootfs/etc/init.d/first-boot) utilise notamment `apt-get` et `dpkg` pour installer les dépendances et logiciels nécessaires, comme `openssh-server`, `bluez`, `network-manager` et d’autres composants système.

Cela signifie que le projet n’a pas seulement le noyau Linux, mais aussi une couche d’installation logicielle déjà prévue pour l’environnement final.

---

## 5) Programme de surveillance mémoire et CPU

Le dépôt contient aussi un petit programme C++ de diagnostic système situé dans le dossier :

- rootfs/usr/src/memory/ram_memory.cpp

Ce programme lit les fichiers procfs suivants :
- /proc/meminfo pour afficher la RAM totale, la RAM disponible et la RAM utilisée
- /proc/stat pour mesurer l’utilisation du CPU sur une période d’une seconde

Exemple de sortie :

```text
RAM totale     : 3394 Mo
RAM disponible : 1683 Mo
RAM utilisee   : 1710 Mo
Usage CPU      : 2.52525 %
```

Compilation :

```bash
g++ -std=c++17 rootfs/usr/src/memory/ram_memory.cpp -o ram_memory
./ram_memory
```

Ce programme est utile pour vérifier rapidement l’état du système sans dépendre d’outils externes, et il complète l’environnement Linux du dépôt avec un outil de supervision simple et portable.
- gestion de l’isolation virtuelle

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
