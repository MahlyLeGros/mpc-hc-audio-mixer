# MPC-HC Audio Mixer

Version personnalisée de [MPC-HC de clsid2](https://github.com/clsid2/mpc-hc),
avec un mélangeur pour écouter plusieurs pistes audio d’une vidéo en même temps.
Ce projet est indépendant du projet officiel MPC-HC.

## Télécharger et installer

Ouvrir **Releases** à droite de cette page, puis télécharger
**MPC-HC-AudioMixer-Setup-x64.exe** dans **Assets**.
L’installateur contient le lecteur et tous les composants nécessaires.
Windows 10 ou 11, 64 bits. Fermer le lecteur avant une mise à jour.

À la fin de l’installation, laisser cochée l’option de choix du lecteur par
 défaut. Dans les paramètres Windows, sélectionner **MPC-HC Audio Mixer**,
puis **Définir par défaut**, ou choisir les formats vidéo souhaités.

## Mélangeur

- Ouvrir une vidéo MKV, MP4, MOV, AVI ou WebM contenant plusieurs pistes audio.
- Cliquer sur l’icône à trois curseurs, près du volume en bas à droite.
- Couper ou rétablir chaque piste avec son bouton haut-parleur ; son volume est conservé.
- Régler chaque piste indépendamment ; le volume natif reste le volume général.
- Les niveaux horizontaux affichent le signal source, même si la piste est à 0 %.
- Le panneau affiche quatre pistes à la fois : utiliser la molette hors des
  curseurs ou la barre de défilement pour accéder aux autres.
- Le mixage reste actif quand on ferme le panneau. Les réglages se réinitialisent
  au changement de fichier.

Première ouverture : la piste sélectionnée reste à 100 %, les autres à 0 %.
Les niveaux sont horodatés selon la lecture ; ils ne suivent pas l’avance du décodeur.

## Sources et compilation

Le dossier [mpc-hc](mpc-hc) contient les sources modifiées et toutes les
 dépendances sources, y compris les sous-modules d’origine déjà inclus.
Les modifications sont appliquées : aucun patch à ajouter.

Voir [le guide des sources](mpc-hc/LIRE-MOI-SOURCES.md),
[la compilation](mpc-hc/docs/Compilation.md) et
[le mélangeur](mpc-hc/docs/AudioMixer.md).
Le script de l’installateur est [audio-mixer_setup.iss](mpc-hc/distrib/audio-mixer_setup.iss).
La base exacte et les révisions des dépendances figurent dans
[SOURCES-MANIFEST.json](mpc-hc/SOURCES-MANIFEST.json).

## Limites

Le mixage simultané concerne les pistes internes des MKV/MP4/MOV/AVI/WebM locaux,
avec une sortie stéréo à vitesse normale. Les autres formats utilisent
la lecture habituelle. Aucun export du mixage ni réglage mémorisé entre fichiers.

## Licence

MPC-HC et les modifications sont distribués sous **GPL-3.0-or-later** :
[licence](LICENSE). Les dépendances conservent leurs licences et auteurs
 dans leurs dossiers. Les sources complètes sont aussi proposées dans Releases.
