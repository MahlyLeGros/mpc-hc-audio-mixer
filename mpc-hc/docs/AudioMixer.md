# Mélangeur de pistes audio — version expérimentale

Cette version dédiée de MPC-HC ajoute le mixage simultané des pistes audio
internes des fichiers MKV, MP4, MOV, AVI et WebM locaux. Elle ne modifie jamais le média.

## Installation Windows

Lancer `MPC-HC-AudioMixer-Setup-x64.exe` et suivre l’assistant. Le lecteur
s’installe pour votre compte Windows dans un dossier dédié, avec un raccourci
dans le menu Démarrer et une entrée de désinstallation dans les applications.

À la fin, laisser cochée l’option pour choisir le lecteur vidéo par défaut.
Dans les paramètres Windows, sélectionner **MPC-HC Audio Mixer**, puis
**Définir par défaut**, ou choisir les extensions souhaitées (MP4, MKV…).
Les associations existantes ne changent qu’après votre sélection dans Windows.
Il n’est plus nécessaire de remplacer les fichiers d’un MPC-HC installé.

Les mises à jour utilisent le même installateur et conservent les réglages.
La désinstallation conserve le fichier de réglages personnel.

## Utilisation

1. Ouvrir une vidéo MKV, MP4, MOV, AVI ou WebM dans cette version de MPC-HC.
2. Cliquer sur l’**icône de mélangeur (trois curseurs)**, à droite dans la
   barre des commandes, juste à gauche du haut-parleur et du volume.
   Une petite fenêtre attachée au bouton s’ouvre au-dessus des commandes.
   Elle suit le lecteur et n’a pas d’entrée séparée dans la barre des tâches. Le menu
   **Lecture → Piste audio → Mélangeur audio…** reste aussi disponible.
3. Le bouton est disponible uniquement pour les fichiers contenant plusieurs pistes
   audio. Ouvrir le panneau active le mixage à la même position : la piste déjà
   sélectionnée reste à 100 %, les autres commencent à 0 %.
4. Régler chaque piste avec son curseur, identique au volume natif du lecteur.
   Le volume en bas à droite règle le volume général de toutes les pistes.

Le panneau affiche les noms, un niveau sonore horizontal par piste et les volumes.
Le niveau passe du vert au jaune puis au rouge près de la saturation.
Il mesure le signal source avant le volume : une piste à 0 % reste visible
pour identifier celles qui contiennent du son sans les écouter. Les niveaux sont
horodatés et affichés selon la position de lecture, pas l’avance du décodeur.
La mesure couvre 50 ms pour conserver les sons brefs ; affichage toutes les 33 ms.
Cliquer à nouveau sur l’icône, appuyer sur Échap ou cliquer ailleurs le masque
sans interrompre le mixage. Les réglages restent pendant les pauses et les
recherches et se réinitialisent au changement de fichier. Jusqu’à quatre pistes
sont visibles : la molette hors des curseurs et la barre verticale permettent
 d’accéder aux suivantes. Sur un curseur, la molette règle cette piste par pas
 de 5 %. Le limiteur intégré protège la somme des pistes contre la saturation.


## Limites de cette première version

- Sortie PCM stéréo, flottant 32 bits, 48 kHz ; pas de bitstream audio.
- Lecture à vitesse normale uniquement pendant le mixage.
- Pas de flux réseau, de fichiers audio externes ni d'export du mixage.
- Les DLL du dossier `LAVFilters64` doivent provenir de la même compilation.
  Le chargement vérifie les versions majeures de leurs interfaces.
- Chaque piste ouvre son propre décodeur et son propre lecteur de fichier :
  la charge augmente avec le nombre de pistes.
- Une piste illisible est signalée et devient silencieuse. Si aucune piste
  ou aucune sortie n'est utilisable, l'activation revient à l'audio habituel.

## Architecture

Le graphe vidéo habituel reste dans MPC-HC. En mode mixage, ses sorties audio
originales ne sont pas connectées. Une source audio interne DirectShow décode
les pistes avec les bibliothèques FFmpeg de LAV, convertit chaque piste avec
libswresample et fournit une unique sortie stéréo au circuit audio habituel.

Les horodatages sont ramenés à l'origine du conteneur et placés sur une
chronologie commune de 48 000 échantillons par seconde. Les trous et les fins
anticipées sont silencieux. Un déplacement vide les données décodées et
effectue une nouvelle recherche avec prélecture. Le rendu utilise l'horloge
du même graphe que la vidéo ; aucun lecteur audio externe n'est lancé.

L'interface interne `AudioMix::Session` protège les commandes et les niveaux
par mutex. `Probe` découvre les pistes, `Decoder` fournit les blocs horodatés,
`Mixer` applique volumes, Muet/Solo et limiteur, `AddSource` connecte la source.
Le gain évolue sur environ 5 ms ; le limiteur a une attaque instantanée et
un relâchement d'environ 50 ms.

## Compilation et tests

Suivre `Compilation.md` : Visual Studio Build Tools 2022 avec C++, ATL, MFC,
un SDK Windows et NASM. Le fichier local `build.user.bat` indique les chemins
des outils et le SDK installé. Les scripts reconnaissent aussi Build Tools.

```bat
build.bat x64 Release Silent Nocolors
tests\audio-mixer\run-core-tests.bat
python tests\audio-mixer\make-fixtures.py C:\chemin\ffmpeg.exe
tests\audio-mixer\run-decoder-tests.bat
tests\audio-mixer\run-graph-tests.bat
tests\audio-mixer\run-dialog-tests.bat
powershell -File tests\audio-mixer\smoke-panel.ps1
```

Les tests du décodeur utilisent des médias synthétiques à deux et trois pistes,
PCM/AAC/Opus, mono/stéréo, des fréquences différentes, une piste retardée et
courte, des recherches répétées et une chronologie de 30 minutes. Les tests
DirectShow vérifient les transitions de lecture et les horodatages des blocs.
Ces tests hors ligne ne mesurent pas le décalage physique entre l'écran et les
haut-parleurs. L'objectif de moins de 40 ms doit aussi être contrôlé sur une
vidéo avec repères audio/visuels, sur le matériel de destination.

## Licence

Les modifications sont sous GPL-3.0-or-later, comme MPC-HC. Conserver
`COPYING.txt` et les licences des dépendances lors de la redistribution.
Cette compilation expérimentale n'est pas une version officielle du projet
MPC-HC. Les sources modifiées accompagnent le dossier de développement.
Les mises à jour automatiques sont désactivées dans le paquet portable pour
éviter de remplacer cette variante par la version officielle sans mélangeur.

## Version 0.2.1

Bouton mute/unmute par piste entre le nom et le vumètre, utilisant les ressources SVG natives MPC-HC rendues à la taille exacte du bouton. Le niveau de volume est conservé pendant le mute. Correction des traces sur les barres de volume après utilisation de la molette par rafraîchissement complet du contrôle.
