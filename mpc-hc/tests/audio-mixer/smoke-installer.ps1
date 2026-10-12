param([Parameter(Mandatory=$true)][string]$Installer)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$testDirectory = Join-Path $repo ('bin/installer-smoke-test-'+[Guid]::NewGuid().ToString('N'))
$registration = 'HKCU:\Software\RegisteredApplications'
$appKey = 'HKCU:\Software\MPC-HC Audio Mixer'
$progId = 'HKCU:\Software\Classes\MPCHCAudioMixer.Video'
$uninstallKey = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\{C5CC4C3F-45E4-49D5-81D0-512D33C18C52}_is1'
$shortcut = Join-Path ([Environment]::GetFolderPath('Programs')) 'MPC-HC Audio Mixer.lnk'
function CurrentDefault($ext) {
 $path = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\$ext\UserChoice"
 if (Test-Path -LiteralPath $path) { (Get-Item -LiteralPath $path).GetValue('ProgId') }
}
function Assert($condition,$message) { if (-not $condition) { throw $message } }
Assert (-not (Test-Path -LiteralPath $testDirectory)) 'Test directory already exists'
Assert (-not (Test-Path -LiteralPath $appKey)) 'Mixer is already installed; refusing to alter existing registration'
Assert (-not (Test-Path -LiteralPath $progId)) 'Mixer ProgID already exists'
Assert (-not (Test-Path -LiteralPath $uninstallKey)) 'Mixer uninstall entry already exists'
Assert (-not (Test-Path -LiteralPath $shortcut)) 'Mixer shortcut already exists'
Assert ($null -eq ((Get-Item -LiteralPath $registration).GetValue('MPCHCAudioMixer'))) 'Registered application already exists'
$defaults = @{}
foreach ($ext in @('.mp4','.mkv')) {
 $defaults[$ext] = CurrentDefault $ext
}
$installed = $false
try {
 $process = Start-Process -FilePath (Resolve-Path -LiteralPath $Installer).Path -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/SP-','/LANG=french',('/DIR="'+$testDirectory+'"'),'/TASKS=',('/LOG="'+(Join-Path $repo 'bin/mixer-tests/install.log')+'"')) -WindowStyle Hidden -PassThru
 Assert ($process.WaitForExit(30000)) 'Installer timed out'
 Assert ($process.ExitCode -eq 0) ("Installer failed: "+$process.ExitCode)
 $installed = $true
 Assert (Test-Path -LiteralPath $shortcut) 'Start menu shortcut missing'
 Assert ((Get-ItemPropertyValue -LiteralPath $registration -Name MPCHCAudioMixer) -eq 'Software\MPC-HC Audio Mixer\Capabilities') 'Default apps registration missing'
 foreach ($ext in @('.mp4','.mkv','.avi','.webm')) {
  Assert ((Get-ItemPropertyValue -LiteralPath ($appKey+'\Capabilities\FileAssociations') -Name $ext) -eq 'MPCHCAudioMixer.Video') 'Video format not registered'
 }
 $command = (Get-Item -LiteralPath ($progId+'\shell\open\command')).GetValue('')
 Assert ($command -eq ('"'+(Join-Path $testDirectory 'mpc-hc64.exe')+'" "%1"')) 'Incorrect file open command or quoting'
 $payload = Join-Path $repo 'bin/installer-payload'
 foreach ($file in Get-ChildItem -LiteralPath $payload -Recurse -File) {
  if ($file.Name -in @('SHA256SUMS.txt','build-info.json')) { continue }
  $relative = $file.FullName.Substring($payload.Length+1)
  Assert ((Get-FileHash -LiteralPath $file.FullName).Hash -eq (Get-FileHash -LiteralPath (Join-Path $testDirectory $relative)).Hash) "Installed payload differs: $relative"
 }
 & (Join-Path $PSScriptRoot 'smoke-panel.ps1') -BinaryPath (Join-Path $testDirectory 'mpc-hc64.exe') -MediaName three-tracks.mp4
 Set-Content -LiteralPath (Join-Path $testDirectory 'mpc-hc64.ini') -Value "[Settings]`r`nUpdaterAutoCheck=0`r`nInstallerTest=preserved" -Encoding Unicode
 $update = Start-Process -FilePath (Resolve-Path -LiteralPath $Installer).Path -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/SP-',('/DIR="'+$testDirectory+'"'),'/TASKS=') -WindowStyle Hidden -PassThru
 Assert ($update.WaitForExit(30000) -and $update.ExitCode -eq 0) 'Update failed'
 Assert ((Get-Content -LiteralPath (Join-Path $testDirectory 'mpc-hc64.ini') -Raw) -match 'InstallerTest=preserved') 'Update replaced user settings'
 foreach ($ext in @('.mp4','.mkv')) {
  $after = CurrentDefault $ext
  Assert ($after -eq $defaults[$ext]) 'Installer changed existing default without user selection'
 }
 'PASS: install, registered formats, shortcuts, payload hashes, installed player playback and mixer, update preserves settings and existing defaults'
} finally {
 if (Test-Path -LiteralPath (Join-Path $testDirectory 'unins000.exe')) {
  $remove = Start-Process -FilePath (Join-Path $testDirectory 'unins000.exe') -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART') -WindowStyle Hidden -PassThru
  if (-not $remove.WaitForExit(30000) -or $remove.ExitCode -ne 0) { throw 'Test uninstall failed' }
 }
}
Assert (-not (Test-Path -LiteralPath $appKey)) 'Uninstall left application registration'
Assert (-not (Test-Path -LiteralPath $progId)) 'Uninstall left ProgID'
Assert (-not (Test-Path -LiteralPath $shortcut)) 'Uninstall left shortcut'
Assert (-not (Test-Path -LiteralPath $uninstallKey)) 'Uninstall left entry'
Assert (-not (Test-Path -LiteralPath (Join-Path $testDirectory 'mpc-hc64.exe'))) 'Uninstall left executable'
'PASS: uninstall removes program, shortcut and registrations while preserving user settings'
