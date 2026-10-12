param(
 [Parameter(Mandatory=$true)][string]$PortableZip,
 [string]$Compiler = '',
 [string]$OutputDirectory = ''
)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
if (-not $Compiler) { $Compiler = Join-Path $repo '../tools/inno-setup/ISCC.exe' }
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $repo '../../deliverables' }
$zip = (Resolve-Path -LiteralPath $PortableZip).Path
$compilerPath = (Resolve-Path -LiteralPath $Compiler).Path
$outputPath = (Resolve-Path -LiteralPath $OutputDirectory).Path
$staging = Join-Path $repo ('bin/installer-'+[Guid]::NewGuid().ToString('N'))
Expand-Archive -LiteralPath $zip -DestinationPath $staging
$payloads = @(Get-ChildItem -LiteralPath $staging -Directory | Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName 'mpc-hc64.exe') })
if ($payloads.Count -ne 1) { throw 'Expected one portable player in archive' }
$payload = $payloads[0].FullName
$hashList = Join-Path $payload 'SHA256SUMS.txt'
foreach ($line in Get-Content -LiteralPath $hashList) {
 if (-not $line.Trim()) { continue }
 $digest,$relative = $line -split '\s+',2
 $file = [IO.Path]::GetFullPath((Join-Path $payload $relative))
 if (-not $file.StartsWith($payload+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Invalid payload path' }
 if ((Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash -ne $digest) { throw "Invalid payload hash: $relative" }
}
Copy-Item -LiteralPath (Join-Path $repo 'docs/AudioMixer.md') -Destination (Join-Path $payload 'LIRE-MOI.md') -Force
& $compilerPath /Qp ('/DPayloadDir='+$payload) ('/DOutputFolder='+$outputPath) (Join-Path $repo 'distrib/audio-mixer_setup.iss')
if ($LASTEXITCODE -ne 0) { throw "Installer compilation failed: $LASTEXITCODE" }
$setup = Join-Path $outputPath 'MPC-HC-AudioMixer-Setup-x64.exe'
$hash = Get-FileHash -LiteralPath $setup -Algorithm SHA256
"$($hash.Hash)  $([IO.Path]::GetFileName($setup))" | Set-Content -LiteralPath ($setup+'.sha256') -Encoding UTF8
$setup
