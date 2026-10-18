# ChatIA

ChatIA est un petit assistant conversationnel écrit en C++ pour répondre en français à des demandes simples telles que :

- salutation
- présentation du bot
- météo d'une ville
- heure et date
- remerciements
- au revoir
- calculs simples et expressions mathématiques

Le projet repose sur une détection d'intention à partir du texte saisi par l'utilisateur, puis sur un moteur de réponse adapté.

## Fonctionnalités

- normalisation du texte (accents, espaces, minuscules)
- détection de mots-clés et similarité approximative pour gérer les fautes de frappe
- identification d'intentions : bonjour, météo, heure, date, merci, au revoir, qui es-tu, calcul, usb
- calcul d'expressions arithmétiques avec `+`, `-`, `*`, `/`, `%`, `^` et parenthèses
- détection et liste des périphériques USB connectés sur le système, avec identification des ports série (`ttyUSB*`, `ttyACM*`)
- récupération de la météo via l'API Open-Meteo
- interface en ligne de commande simple
- lancement explicite de commandes `sudo` avec confirmation
- utilisation d'un module dédié `calcul.h` pour centraliser les opérations mathématiques

## Structure du projet

- `main.cpp` : point d'entrée du programme
- `Chatbot.h` / `Chatbot.cpp` : logique principale du chatbot et intégration de la calculatrice
- `IntentDetector.h` / `IntentDetector.cpp` : détection des intentions utilisateur, y compris les expressions mathématiques
- `TextProcessing.h` / `TextProcessing.cpp` : normalisation et traitement du texte
- `calcul.h` : fonctions mathématiques réutilisables et primitives de calcul
- `WeatherService.h` / `WeatherService.CPP` : recherche de ville et récupération de la météo

## Prérequis

Avant de compiler le projet, vérifie que les dépendances suivantes sont installées :

- compilateur C++ (`g++` ou `clang++`)
- bibliothèque `libcurl`
- bibliothèque JSON de type `nlohmann/json`
- Qt 6 Widgets et Qt Concurrent pour l'interface graphique

Sur Ubuntu/Debian, tu peux installer les paquets avec :

```bash
sudo apt update
sudo apt install g++ libcurl4-openssl-dev nlohmann-json3-dev qt6-base-dev
```

## Compilation

Depuis le dossier du projet :

```bash
g++ -std=c++17 -Wall -Wextra -pedantic -I. \
    main.cpp Chatbot.cpp IntentDetector.cpp TextProcessing.cpp WeatherService.CPP UsbService.cpp \
    -o chatia -lcurl
```

Pour compiler l'interface graphique Qt 6 :

```bash
g++ -std=c++17 -Wall -Wextra -pedantic -I. \
    $(pkg-config --cflags Qt6Widgets Qt6Concurrent libcurl) \
    -x c++ INTERFACE Chatbot.cpp IntentDetector.cpp TextProcessing.cpp \
    WeatherService.CPP UsbService.cpp \
    -o chatia-gui \
    $(pkg-config --libs Qt6Widgets Qt6Concurrent libcurl)
```

## Utilisation

Lance l'exécutable :

```bash
./chatia
```

Pour lancer l'interface graphique, utilise `./chatia-gui` depuis une session
de bureau.

Puis entre des messages comme :

```text
bonjour
quelle heure est-il ?
quelle est la météo à Lyon ?
2 + 2
calcule 12 * 7
combien fait 8 * (3 + 2) ?
merci
au revoir
```

La calculatrice intégrée accepte également des formulations plus naturelles, par exemple :

```text
combien fait 10 plus 5 ?
8 fois 7
12 divise par 3
2 puissance 10
```

Pour demander une commande administrateur, saisis-la directement, par exemple :

```text
sudo apt update
```

ChatIA affiche la commande puis attend que tu tapes exactement `oui` avant de la
lancer. `sudo` peut ensuite demander ton mot de passe dans le terminal ; ChatIA
ne le lit pas et ne le stocke pas. Les arguments sont passés directement à la
commande, sans interpréteur shell : les guillemets simples ou doubles regroupent
les arguments, mais les pipelines (`|`) et redirections (`>`) ne sont pas
interprétés. Toute commande lancée ainsi peut modifier le système ; vérifie-la
attentivement avant de confirmer.

## Exemple de sortie

```text
bonjour
Bonjour ! Je suis ton robot, comment puis-je t'aider ?
quelle heure est-il ?
Il est 14:32:15.
2 + 2
Le résultat est 4.
combien fait 8 * (3 + 2) ?
Le résultat est 40.
quelle est la météo à Paris ?
Météo à Paris : 18.4°C, ciel clair.
```

## Remarques

- Le service météo utilise Open-Meteo et dépend d'une connexion Internet valide.
- La détection d'intentions est volontairement simple et centrée sur des mots-clés, ce qui la rend facile à étendre.
- La calculatrice peut accepter des expressions simples, des parenthèses et des mots-clés du langage naturel.
- Le projet est bien adapté comme base pour un assistant vocal ou textuel plus avancé.



## Licence

Ce projet est fourni sans licence spécifique dans ce dépôt ; adapte-la selon le contexte de ton usage.
