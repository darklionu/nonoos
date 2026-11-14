# Assistant de migration

## État actuel

Ce dossier contient un module C++ de **détection et d'inventaire**, pas encore
un outil qui copie ou migre des données. Il peut inventorier des partitions,
rechercher des marqueurs de systèmes Linux, Windows et macOS et lister les
fichiers d'un dossier source.

Le module ne formate pas de partition, ne supprime pas de fichiers et ne copie
aucune donnée.

## Fonctionnalités disponibles

L'interface publique se trouve dans [`detector.hpp`](./detector.hpp) et son
implémentation dans [`detector.cpp`](./detector.cpp). Le filtrage des
extensions est isolé dans [`extension_filter.hpp`](./extension_filter.hpp) et
[`extension_filter.cpp`](./extension_filter.cpp).

- `detect_partitions()` utilise `lsblk` pour trouver les partitions et volumes
  Linux, LVM et chiffrés déjà déverrouillés qu'il sait examiner.
- `detect_installed_os()` recherche les marqueurs suivants :
  - Linux : `/etc/os-release` ou `/usr/lib/os-release` ;
  - Windows : `Windows/System32/config/SOFTWARE` ;
  - macOS : `System/Library/CoreServices/SystemVersion.plist`.
- `DetectionReport` contient les systèmes trouvés et les erreurs rencontrées
  sur des partitions individuelles.
- `list_files(root, extensions)` parcourt récursivement les fichiers ordinaires
  sous `root`. Les chemins retournés sont relatifs à cette racine, les liens
  symboliques ne sont pas suivis et `FileListReport` conserve les erreurs
  d'accès rencontrées. Une liste d'extensions vide retourne tous les fichiers ;
  sinon seules les extensions indiquées sont conservées, avec ou sans point et
  sans distinction entre majuscules et minuscules.
- Une partition non montée est montée temporairement avec des options de
  lecture seule, puis démontée après l'inspection.
- Les volumes LUKS verrouillés ne sont pas déverrouillés automatiquement.

La détection de Windows et macOS confirme uniquement la présence d'un marqueur :
elle ne lit pas encore leur version. Le détecteur ne garantit pas non plus que
tous les systèmes de fichiers ou toutes les configurations de démarrage soient
reconnus.

### Attention aux partitions déjà montées

Si une partition est déjà montée, le détecteur inspecte son point de montage
existant et ne la remonte pas lui-même en lecture seule. Il ne vérifie pas si ce
montage existant est en lecture seule. Ne présentez donc pas l'inspection
complète comme garantie sans écriture sur le disque source.

## Utilisation

Inclure l'en-tête dans le code appelant :

```cpp
#include "detector.hpp"

DetectionReport report = detect_installed_os();
for (const auto& system : report.systems) {
    // Afficher system.name, system.version et system.device.
}
for (const auto& error : report.errors) {
    // Présenter l'erreur à l'utilisateur.
}

FileListReport files = list_files("/chemin/vers/partition-montee");
for (const auto& file : files.files) {
    // Afficher le chemin relatif du fichier.
}

FileListReport documents = list_files(
    "/chemin/vers/partition-montee",
    {".pdf", "odt", "DOCX"});
for (const auto& file : documents.files) {
    // Seuls les PDF, ODT et DOCX sont retournés.
}
for (const auto& error : files.errors) {
    // Présenter les dossiers ou fichiers qui n'ont pas pu être lus.
}
```

Le programme interactif `extension_filter.cpp` demande à l'utilisateur les
extensions à rechercher, séparées par des espaces (par exemple `.pdf odt
DOCX`). Une ligne vide désactive le filtre et retourne tous les fichiers.

Pour inventorier une partition non montée, montez-la avec
`mount_partition_readonly()`, appelez `list_files()` avec le point de montage,
puis appelez `unmount_partition()`. Le parcours ne suit pas les liens
symboliques et ne retourne que les fichiers ordinaires ; il ne copie, ne
supprime et ne modifie aucun fichier. Les résultats peuvent être volumineux
pour une partition entière.

Le programme doit être compilé avec le module de détection et les utilitaires
associés, en C++17 ou plus récent. Depuis la racine du dépôt, une vérification
minimale de compilation est possible avec :

```sh
g++ -std=c++17 -Wall -Wextra -pedantic \
  "rootfs/usr/migration-assistant/detector.cpp" \
  "rootfs/usr/migration-assistant/extension.cpp" \
  "rootfs/usr/migration-assistant/extengest.cpp" \
  "rootfs/usr/migration-assistant/extension_filter.cpp" \
  -o /tmp/migration-filter
```

`lsblk` doit être disponible. Les montages temporaires nécessitent les
privilèges appropriés (généralement root) et peuvent échouer si le système de
fichiers n'est pas pris en charge ou est endommagé. Les erreurs sont alors
ajoutées au rapport.

## Objectif : migrer les fichiers personnels

Le premier objectif recommandé est la copie de fichiers personnels depuis une
installation Linux vers un dossier choisi sur NonoOS. Ce périmètre est plus
réaliste et vérifiable que la migration complète d'un système, qui impliquerait
aussi les applications, les pilotes, les comptes et le démarrage.

Un parcours utilisateur devrait permettre de :

1. choisir une installation source détectée ;
2. sélectionner les catégories à transférer, par exemple Documents, Images,
   Vidéos, Musique et Bureau ;
3. choisir un dossier de destination et vérifier l'espace disponible ;
4. prévisualiser les fichiers et la taille estimée sans rien copier ;
5. confirmer explicitement l'opération ;
6. suivre la progression, annuler proprement et consulter le rapport final.

### Composants à ajouter

Séparer la planification de l'exécution permettra de tester l'aperçu sans
modifier de données :

- **Plan de migration** : sources, destination, éléments sélectionnés et
  estimation de l'espace ;
- **Planificateur** : validation de la source et de la destination, détection des
  conflits et génération d'un aperçu ;
- **Exécuteur** : copie contrôlée, progression, annulation et vérification des
  fichiers copiés ;
- **Rapport** : fichiers copiés, ignorés, en conflit ou en erreur.

### Garde-fous requis avant toute copie

- Garder la source en lecture seule et ne jamais supprimer ou déplacer ses
  fichiers dans la première version.
- Refuser une destination située sur la source ou à l'intérieur de celle-ci.
- Afficher et résoudre explicitement les conflits de noms ; ne pas écraser en
  silence.
- Vérifier l'espace disponible avant de commencer, tout en traitant les erreurs
  qui peuvent survenir pendant la copie.
- Ne pas construire de commande shell à partir des chemins fournis par
  l'utilisateur. Utiliser des API de fichiers ou exécuter un programme avec des
  arguments séparés.
- Rapporter les échecs de copie et vérifier les résultats, au minimum en
  comparant les tailles et idéalement avec des sommes de contrôle.
- Ne pas annoncer une sauvegarde tant qu'elle n'a pas été vérifiée et testée
  comme telle.

## Tests recommandés

Avant d'ajouter la copie, couvrir la détection et la planification par des tests
qui ne nécessitent ni privilèges root ni disques réels. Prévoir des cas Linux,
Windows et macOS, des systèmes absents ou incomplets, des volumes chiffrés
verrouillés et des erreurs de lecture.

Les tests de l'exécuteur devront couvrir les destinations insuffisantes en
espace, les conflits, l'annulation, les erreurs d'accès et la vérification des
fichiers. Les tests ne doivent jamais utiliser de vraies partitions sans
environnement isolé et explicitement prévu à cet effet.
