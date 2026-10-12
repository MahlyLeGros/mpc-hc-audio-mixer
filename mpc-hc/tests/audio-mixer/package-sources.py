"""Package working sources and all initialized Git submodules, without Git metadata."""
from pathlib import Path
import hashlib, json, subprocess, sys, zipfile

repo = Path(__file__).resolve().parents[2]
output = Path(sys.argv[1]).resolve() if len(sys.argv)>1 else repo.parents[1]/'deliverables/MPC-HC-AudioMixer-Sources-completes.zip'
files = {}
modules = []
def git(path,*args):
    return subprocess.check_output(['git','-C',str(path),*args])
def visit(path,prefix=''):
    commit = git(path,'rev-parse','HEAD').decode().strip()
    modules.append({'path':prefix or '.', 'commit':commit})
    entries=git(path,'ls-files','--stage','-z').split(b'\0')
    for entry in entries:
        if not entry: continue
        metadata,name=entry.split(b'\t',1)
        mode,blob,stage=metadata.split()
        if stage != b'0': raise RuntimeError('Unresolved merge: '+name.decode())
        relative=name.decode('utf-8')
        local=path/relative
        archive=prefix+relative
        if mode==b'160000':
            if not (local/'.git').exists(): raise RuntimeError('Missing submodule: '+archive)
            visit(local,archive+'/')
        else:
            if not local.is_file(): raise RuntimeError('Missing tracked file: '+archive)
            files[archive]=local
visit(repo)
# Generated version headers preserve the exact released revision without a Git checkout.
for relative in ['build/version_rev.h','src/thirdparty/LAVFilters/src/common/version.h']:
    if (repo/relative).is_file(): files[relative]=repo/relative
extras={
 'SOURCES-MANIFEST.json':json.dumps({'upstream':'https://github.com/clsid2/mpc-hc','modules':modules,'description':'Working sources for MPC-HC Audio Mixer, including installer and recursive submodules'},indent=2).encode(),
 'LIRE-MOI-SOURCES.md':'''# Sources complètes — MPC-HC Audio Mixer

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
'''.encode('utf-8'),
 'build.user.example.bat':b'@echo off\r\nSET "MPCHC_WINSDK_VER=10.0.26100.0"\r\nSET "MPCHC_LAV_TOOLCHAIN=MSVC"\r\nSET "MPCHC_VS_PATH=C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools"\r\nREM Add your NASM and Python installation directories to PATH.\r\n'
}
output.parent.mkdir(parents=True,exist_ok=True)
hashes=[]
with zipfile.ZipFile(output,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=6,allowZip64=True) as archive:
    for name,path in sorted(files.items()):
        data=path.read_bytes()
        archive.writestr('MPC-HC-AudioMixer-Sources/'+name,data)
        hashes.append(hashlib.sha256(data).hexdigest()+'  '+name)
    for name,data in extras.items():
        archive.writestr('MPC-HC-AudioMixer-Sources/'+name,data)
        hashes.append(hashlib.sha256(data).hexdigest()+'  '+name)
    archive.writestr('MPC-HC-AudioMixer-Sources/SHA256SUMS-SOURCES.txt','\n'.join(hashes)+'\n')
with zipfile.ZipFile(output) as archive:
    bad=archive.testzip()
    if bad: raise RuntimeError('Invalid ZIP entry: '+bad)
    for line in archive.read('MPC-HC-AudioMixer-Sources/SHA256SUMS-SOURCES.txt').decode().splitlines():
        digest,name=line.split('  ',1)
        assert hashlib.sha256(archive.read('MPC-HC-AudioMixer-Sources/'+name)).hexdigest()==digest,name
output.with_suffix('.zip.sha256').write_text(hashlib.sha256(output.read_bytes()).hexdigest()+'  '+output.name+'\n')
print(json.dumps({'archive':str(output),'files':len(files)+len(extras)+1,'repositories':len(modules),'bytes':output.stat().st_size}))
