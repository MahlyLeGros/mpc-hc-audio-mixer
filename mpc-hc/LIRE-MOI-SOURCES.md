# Sources complètes — MPC-HC Audio Mixer

Cette archive correspond au lecteur de MPC-HC-AudioMixer-Setup-x64.exe.
Elle contient le code MPC-HC modifié, les sous-modules récursifs (LAVFilters,
FFmpeg et bibliothèques), les licences, les scripts de compilation et
l'installateur. Les révisions de base figurent dans SOURCES-MANIFEST.json.
Les fichiers modifiés sont déjà appliqués : aucun patch séparé n'est nécessaire.

## Compiler

Consulter docs/Compilation.md pour les prérequis et la procédure complète.
Installer Visual Studio 2022 avec C++ x64, ATL, MFC et le SDK Windows,
NASM, Python et Git. Copier build.user.example.bat vers build.user.bat et
adapter les chemins. Cette version a été construite avec le SDK 10.0.26100.0,
Visual Studio 2022 et NASM 2.16.03 ; les codecs utilisent le chemin MSVC.
Les dépendances sources sont présentes : aucune récupération Git des
sous-modules n'est nécessaire. Garder l'arborescence de cette archive.
Lancer build.bat Release x64 depuis la racine (voir build.bat /help).
Les binaires sont produits dans bin/mpc-hc_x64.
Les outils propriétaires Microsoft et leurs bibliothèques redistribuables
s'obtiennent auprès de Microsoft ; ils ne font pas partie de ces sources.

## Mélangeur et installateur

Lire docs/AudioMixer.md. Les tests sont dans tests/audio-mixer.
Pour produire un portable, utiliser tests/audio-mixer/package-portable.ps1
avec le répertoire redistribuable Visual C++ de votre installation.
Puis installer Inno Setup 6.7.3 et lancer :

    powershell -File tests/audio-mixer/package-installer.ps1 -PortableZip CHEMIN_DU_ZIP -Compiler CHEMIN_ISCC_EXE -OutputDirectory DOSSIER_SORTIE

Le script Inno Setup est distrib/audio-mixer_setup.iss. Le script peut être
compilé avec ISCC en définissant PayloadDir et OutputFolder.

## Redistribution

MPC-HC et nos modifications sont sous GPL version 3 ou ultérieure (COPYING.txt).
Les dépendances conservent leurs licences respectives dans leur dossier.
Les auteurs et mentions d'origine sont conservés. Vous pouvez partager cette
archive avec l'installateur ; elle doit rester accessible à ses destinataires.

L'archive ne contient ni fichiers vidéo, ni réglages personnels, ni journaux,
ni cache de compilation, ni données Git privées. SHA256SUMS-SOURCES.txt
permet de vérifier chaque fichier inclus.
