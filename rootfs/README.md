# Contenu de `rootfs/`

Ce dossier regroupe les fichiers et outils personnalisés intégrés au système
live NonoOS. Lors de la construction, son contenu est copié dans
`config/includes.chroot/` par [`build-live-iso.sh`](../build-live-iso.sh) :
l’arborescence de `rootfs/` devient donc l’arborescence correspondante dans le
système de fichiers de l’ISO.

## Organisation

| Chemin | Contenu |
| --- | --- |
| `etc/` | Configuration du système, scripts de premier démarrage, services systemd, profils KDE et scripts d’administration. |
| `apps/` | Sources des applications NonoOS, dont le gestionnaire de démarrage. |
| `usr/local/bin/` | Commandes installées directement depuis `rootfs/`, dont le gestionnaire de profil utilisateur et l’inventaire matériel. |
| `update-center/` | Source de l’interface de logiciels et mises à jour. |
| `gestionnaire de sécurité/` | Source et fichiers du centre de sécurité. |
| `gestion-alimentations/` | Interface graphique et commande de gestion de l’alimentation. |
| `nonoos-info/` | Sources du tableau de bord et des informations système. |
| `usr/src/ia/` | Sources et documentation de l’assistant ChatIA. |
| `usr/src/USB/` | Composants liés au diagnostic et aux périphériques USB. |
| `usr/src/memory/` | Outils de mesure et de diagnostic de la mémoire. |
| `usr/share/internet/` | Module de détection Wi-Fi et documentation associée. |
| `usr/migration-assistant/` | Détection des systèmes installés et inventaire de fichiers. |
| `usr/share/applications/` | Lanceurs de bureau pour les outils NonoOS. |
| `usr/local/sbin/` | Commandes système installées pour la persistance et l’assistant de migration. |

Les outils compilés pendant la construction sont installés séparément dans
`config/includes.chroot/`. Les sources C++ et Rust correspondantes restent
dans `rootfs/` ou `src/` à la racine du dépôt.

## Démarrage du système

Les scripts et services de configuration se trouvent notamment dans :

- `etc/init.d/first-boot` : configuration initiale ;
- `etc/systemd/system/newdistrib-first-boot.service` : service de premier démarrage ;
- `etc/systemd/system/newdistrib-every-boot.service` : tâches exécutées au démarrage.

Les services concernés sont activés par le script de construction de l’ISO.
Après toute modification de ces fichiers, reconstruisez l’image pour la
prendre en compte.

## Profils utilisateur

Au premier démarrage, NonoOS propose un profil personnel **Standard**,
**Développeur** ou **Support technique**. Le choix est enregistré séparément
dans `~/.config/nonoos/profile` pour le compte configuré. Il peut être consulté
ou modifié depuis le lanceur « Profils utilisateur NonoOS », ou en terminal :

```bash
nonoos-profile --show
nonoos-profile
```

Le profil personnel est distinct des profils de logiciels sélectionnés au
premier démarrage : il ne modifie ni les privilèges du compte ni les paquets
installés. La valeur est réservée aux préférences et outils propres à chaque
utilisateur.

## Inventaire matériel

L’application **Détection du matériel NonoOS**, accessible depuis le menu du
bureau, affiche le modèle, l’architecture, le processeur, la mémoire, les
composants graphiques, les disques, les interfaces réseau et les périphériques
USB détectés. Le même inventaire est présenté pendant la configuration du
premier démarrage. Les commandes `lspci` et `lsusb` enrichissent les noms
affichés ; les informations disponibles dans `/proc` et `/sys` servent de
repli.

Depuis un terminal, vous pouvez également lancer :

```bash
nonoos-hardware
```

## Documentation des composants

Certains sous-dossiers ont leur propre documentation :

- [`usr/src/ia/README.md`](usr/src/ia/README.md) pour ChatIA ;
- [`usr/share/internet/README.md`](usr/share/internet/README.md) pour le module Wi-Fi ;
- [`usr/migration-assistant/README.md`](usr/migration-assistant/README.md) pour l’assistant de migration.

## Modifier et vérifier `rootfs/`

Conservez les chemins et permissions attendus dans le système cible : le
contenu est copié tel quel dans l’image. Pour vérifier les modifications,
exécutez depuis la racine du dépôt :

```bash
# Vérifier la syntaxe des scripts shell
bash -n rootfs/chemin/vers/script.sh

# Exécuter les vérifications CI (syntaxe C++ et tests de fumée Docker)
docker build --file docker/Dockerfile --tag nonoos-rootfs-tests .
docker run --rm --network none nonoos-rootfs-tests
```

Le workflow [`.github/workflows/ci.yml`](../.github/workflows/ci.yml) effectue
ces vérifications pour les changements dans `rootfs/`.
