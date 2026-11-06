# MPC-HC Audio Mixer 0.2.5

- Correction de la sélection involontaire d’un titre lorsque le défilement masque la ligne ayant le focus.
- La réouverture du panneau après défilement cible un contrôle visible, sans entrer dans un nom de piste.
- Renommage, volumes, Muet et pictogrammes persistants conservés.

## MPC-HC Audio Mixer 0.2.4

- Renommage direct des pistes dans le mélangeur, avec sauvegarde par numéro de piste.
- Entrée termine la saisie ; Échap annule la modification en cours.
- Un nom vide réaffiche le nom original de la vidéo, sans modifier le média.
- Noms conservés entre vidéos et après redémarrage ; volumes, Muet, icônes et timeline conservés.

## MPC-HC Audio Mixer 0.2.3

- Volumes, Muet et pictogrammes mémorisés par numéro de piste, entre vidéos et après redémarrage.
- Application automatique du mixage après sa première activation réussie.
- Sélecteur de 12 pictogrammes entre le nom et le bouton Muet.
- Conservation des réglages des pistes absentes ; les nouvelles pistes commencent à 0 %.
- Corrections de timeline de la version 0.2.2 conservées.

Fermer le lecteur avant de lancer le nouvel installateur ; les préférences existantes sont conservées.

## MPC-HC Audio Mixer 0.2.2

- Recherche précise par défaut, sans alignement automatique sur les images clés ; Maj conserve la recherche rapide.
- Ctrl + glisser règle la position actuelle avec au maximum 10 ms par pixel, même sur une longue vidéo.
- Millisecondes dans l’infobulle et dans le compteur pendant le déplacement.
- Application de la position finale même après un très petit déplacement.

Validation : compilation Release x64 ; tests des déplacements fractionnaires, des limites et des clips courts ; lecture MKV et MP4 avec le lecteur compilé. Le geste Ctrl + glisser reste à confirmer manuellement.

Fermer le lecteur et lancer l’installateur pour mettre à jour.

## MPC-HC Audio Mixer 0.2.1

- Bouton mute/unmute indépendant pour chaque piste, entre son nom et son vumètre.
- Icônes SVG natives MPC-HC rendues à la taille exacte du bouton, avec contours lissés et transparence.
- Conservation du volume réglé lors du mute/unmute.
- Correction des traces sur les barres de volume après utilisation de la molette.

Validation : compilation Release x64 ; tests du moteur audio ; tests du lecteur avec plusieurs pistes et une seule piste ; basculement mute/unmute, conservation du volume et indépendance du volume général.

## Installation ou mise à jour

Fermer le lecteur et lancer **MPC-HC-AudioMixer-Setup-x64.exe**. Windows 10/11, 64 bits.

**MPC-HC-AudioMixer-Sources-completes.zip** contient les sources modifiées et les dépendances. Les fichiers SHA-256 permettent de vérifier les téléchargements.

Version personnalisée indépendante du projet officiel MPC-HC, sous GPL-3.0-or-later.
