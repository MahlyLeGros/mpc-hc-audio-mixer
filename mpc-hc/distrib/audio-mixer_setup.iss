; SPDX-License-Identifier: GPL-3.0-or-later
#ifndef PayloadDir
  #error Define PayloadDir to the verified portable package directory
#endif
#ifndef OutputFolder
  #define OutputFolder "..\..\..\deliverables"
#endif
[Setup]
AppId={{C5CC4C3F-45E4-49D5-81D0-512D33C18C52}
AppName=MPC-HC Audio Mixer
AppVersion=0.2.1
AppVerName=MPC-HC Audio Mixer 0.2.1
AppPublisher=MPC-HC Audio Mixer - version personnalisée
AppPublisherURL=https://github.com/clsid2/mpc-hc
DefaultDirName={localappdata}\Programs\MPC-HC Audio Mixer
DefaultGroupName=MPC-HC Audio Mixer
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
OutputDir={#OutputFolder}
OutputBaseFilename=MPC-HC-AudioMixer-Setup-x64
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
SetupIconFile=..\src\mpc-hc\res\icon.ico
UninstallDisplayIcon={app}\mpc-hc64.exe
LicenseFile=..\COPYING.txt
ChangesAssociations=yes
CloseApplications=yes
RestartApplications=no
[Languages]
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"
[Tasks]
Name: "desktopicon"; Description: "Créer un raccourci sur le Bureau"; Flags: unchecked
[Files]
Source: "{#PayloadDir}\*"; DestDir: "{app}"; Excludes: "mpc-hc64.ini,SHA256SUMS.txt,build-info.json"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#PayloadDir}\mpc-hc64.ini"; DestDir: "{app}"; Flags: onlyifdoesntexist uninsneveruninstall
[Icons]
Name: "{autoprograms}\MPC-HC Audio Mixer"; Filename: "{app}\mpc-hc64.exe"
Name: "{autodesktop}\MPC-HC Audio Mixer"; Filename: "{app}\mpc-hc64.exe"; Tasks: desktopicon
[Registry]
Root: HKCU; Subkey: "Software\MPC-HC Audio Mixer"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\MPC-HC Audio Mixer\Capabilities"; ValueType: string; ValueName: "ApplicationName"; ValueData: "MPC-HC Audio Mixer"
Root: HKCU; Subkey: "Software\MPC-HC Audio Mixer\Capabilities"; ValueType: string; ValueName: "ApplicationDescription"; ValueData: "Lecteur vidéo avec mélangeur de pistes audio simultanées"
Root: HKCU; Subkey: "Software\MPC-HC Audio Mixer\Capabilities"; ValueType: string; ValueName: "ApplicationIcon"; ValueData: """{app}\mpc-hc64.exe"",0"
Root: HKCU; Subkey: "Software\RegisteredApplications"; ValueType: string; ValueName: "MPCHCAudioMixer"; ValueData: "Software\MPC-HC Audio Mixer\Capabilities"; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\MPCHCAudioMixer.Video"; ValueType: string; ValueData: "Vidéo - MPC-HC Audio Mixer"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\MPCHCAudioMixer.Video\DefaultIcon"; ValueType: string; ValueData: """{app}\mpc-hc64.exe"",0"
Root: HKCU; Subkey: "Software\Classes\MPCHCAudioMixer.Video\shell\open\command"; ValueType: string; ValueData: """{app}\mpc-hc64.exe"" ""%1"""
Root: HKCU; Subkey: "Software\MPC-HC Audio Mixer\Capabilities\FileAssociations"; ValueType: string; ValueName: ".mp4"; ValueData: "MPCHCAudioMixer.Video"
Root: HKCU; Subkey: "Software\Classes\.mp4\OpenWithProgids"; ValueType: string; ValueName: "MPCHCAudioMixer.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\MPC-HC Audio Mixer\Capabilities\FileAssociations"; ValueType: string; ValueName: ".mkv"; ValueData: "MPCHCAudioMixer.Video"
Root: HKCU; Subkey: "Software\Classes\.mkv\OpenWithProgids"; ValueType: string; ValueName: "MPCHCAudioMixer.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\MPC-HC Audio Mixer\Capabilities\FileAssociations"; ValueType: string; ValueName: ".avi"; ValueData: "MPCHCAudioMixer.Video"
Root: HKCU; Subkey: "Software\Classes\.avi\OpenWithProgids"; ValueType: string; ValueName: "MPCHCAudioMixer.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\MPC-HC Audio Mixer\Capabilities\FileAssociations"; ValueType: string; ValueName: ".mov"; ValueData: "MPCHCAudioMixer.Video"
Root: HKCU; Subkey: "Software\Classes\.mov\OpenWithProgids"; ValueType: string; ValueName: "MPCHCAudioMixer.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\MPC-HC Audio Mixer\Capabilities\FileAssociations"; ValueType: string; ValueName: ".wmv"; ValueData: "MPCHCAudioMixer.Video"
Root: HKCU; Subkey: "Software\Classes\.wmv\OpenWithProgids"; ValueType: string; ValueName: "MPCHCAudioMixer.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\MPC-HC Audio Mixer\Capabilities\FileAssociations"; ValueType: string; ValueName: ".webm"; ValueData: "MPCHCAudioMixer.Video"
Root: HKCU; Subkey: "Software\Classes\.webm\OpenWithProgids"; ValueType: string; ValueName: "MPCHCAudioMixer.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\MPC-HC Audio Mixer\Capabilities\FileAssociations"; ValueType: string; ValueName: ".m4v"; ValueData: "MPCHCAudioMixer.Video"
Root: HKCU; Subkey: "Software\Classes\.m4v\OpenWithProgids"; ValueType: string; ValueName: "MPCHCAudioMixer.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\MPC-HC Audio Mixer\Capabilities\FileAssociations"; ValueType: string; ValueName: ".mpg"; ValueData: "MPCHCAudioMixer.Video"
Root: HKCU; Subkey: "Software\Classes\.mpg\OpenWithProgids"; ValueType: string; ValueName: "MPCHCAudioMixer.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\MPC-HC Audio Mixer\Capabilities\FileAssociations"; ValueType: string; ValueName: ".mpeg"; ValueData: "MPCHCAudioMixer.Video"
Root: HKCU; Subkey: "Software\Classes\.mpeg\OpenWithProgids"; ValueType: string; ValueName: "MPCHCAudioMixer.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\MPC-HC Audio Mixer\Capabilities\FileAssociations"; ValueType: string; ValueName: ".ts"; ValueData: "MPCHCAudioMixer.Video"
Root: HKCU; Subkey: "Software\Classes\.ts\OpenWithProgids"; ValueType: string; ValueName: "MPCHCAudioMixer.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\MPC-HC Audio Mixer\Capabilities\FileAssociations"; ValueType: string; ValueName: ".m2ts"; ValueData: "MPCHCAudioMixer.Video"
Root: HKCU; Subkey: "Software\Classes\.m2ts\OpenWithProgids"; ValueType: string; ValueName: "MPCHCAudioMixer.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\MPC-HC Audio Mixer\Capabilities\FileAssociations"; ValueType: string; ValueName: ".mts"; ValueData: "MPCHCAudioMixer.Video"
Root: HKCU; Subkey: "Software\Classes\.mts\OpenWithProgids"; ValueType: string; ValueName: "MPCHCAudioMixer.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\MPC-HC Audio Mixer\Capabilities\FileAssociations"; ValueType: string; ValueName: ".vob"; ValueData: "MPCHCAudioMixer.Video"
Root: HKCU; Subkey: "Software\Classes\.vob\OpenWithProgids"; ValueType: string; ValueName: "MPCHCAudioMixer.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\MPC-HC Audio Mixer\Capabilities\FileAssociations"; ValueType: string; ValueName: ".ogm"; ValueData: "MPCHCAudioMixer.Video"
Root: HKCU; Subkey: "Software\Classes\.ogm\OpenWithProgids"; ValueType: string; ValueName: "MPCHCAudioMixer.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\MPC-HC Audio Mixer\Capabilities\FileAssociations"; ValueType: string; ValueName: ".ogv"; ValueData: "MPCHCAudioMixer.Video"
Root: HKCU; Subkey: "Software\Classes\.ogv\OpenWithProgids"; ValueType: string; ValueName: "MPCHCAudioMixer.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\MPC-HC Audio Mixer\Capabilities\FileAssociations"; ValueType: string; ValueName: ".3gp"; ValueData: "MPCHCAudioMixer.Video"
Root: HKCU; Subkey: "Software\Classes\.3gp\OpenWithProgids"; ValueType: string; ValueName: "MPCHCAudioMixer.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\MPC-HC Audio Mixer\Capabilities\FileAssociations"; ValueType: string; ValueName: ".flv"; ValueData: "MPCHCAudioMixer.Video"
Root: HKCU; Subkey: "Software\Classes\.flv\OpenWithProgids"; ValueType: string; ValueName: "MPCHCAudioMixer.Video"; ValueData: ""; Flags: uninsdeletevalue
[Run]
Filename: "ms-settings:defaultapps?registeredAppUser=MPCHCAudioMixer"; Description: "Choisir MPC-HC Audio Mixer comme lecteur vidéo par défaut"; Flags: shellexec postinstall skipifsilent
Filename: "{app}\mpc-hc64.exe"; Description: "Ouvrir MPC-HC Audio Mixer"; Flags: nowait postinstall skipifsilent unchecked
