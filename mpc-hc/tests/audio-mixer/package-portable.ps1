param([string]$Destination = "", [string]$VcRuntimeDirectory = "")
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
if (-not $Destination) { $Destination = Join-Path $repo '..\..\deliverables\MPC-HC-AudioMixer-x64' }
$output = [IO.Path]::GetFullPath($Destination)
if (Test-Path -LiteralPath $output) { throw "Destination already exists: $output" }
$binary = Join-Path $repo 'bin\mpc-hc_x64'
if (-not (Test-Path -LiteralPath (Join-Path $binary 'mpc-hc64.exe'))) { throw 'Build the x64 player first.' }
$lav = Join-Path $binary 'LAVFilters64'
foreach ($file in @('LAVSplitter.ax','LAVAudio.ax','LAVVideo.ax','avformat-lav-63.dll','avcodec-lav-63.dll','avutil-lav-61.dll','swresample-lav-7.dll')) {
    if (-not (Test-Path -LiteralPath (Join-Path $lav $file))) { throw "Missing runtime dependency: $file" }
}
New-Item -ItemType Directory -Path $output | Out-Null
Copy-Item -LiteralPath (Join-Path $binary 'mpc-hc64.exe') -Destination $output
Get-ChildItem -LiteralPath $binary -Filter *.dll | Copy-Item -Destination $output
if ($VcRuntimeDirectory) {
    Get-ChildItem -LiteralPath $VcRuntimeDirectory -Filter *.dll | Copy-Item -Destination $output
}
Copy-Item -LiteralPath $lav -Destination $output -Recurse
Get-ChildItem -LiteralPath (Join-Path $output 'LAVFilters64') -Filter *.pdb | Remove-Item
$languages = Join-Path $binary 'Lang'
if (Test-Path -LiteralPath $languages) {
    $langOutput = New-Item -ItemType Directory -Path (Join-Path $output 'Lang')
    Get-ChildItem -LiteralPath $languages -Filter *.dll | Copy-Item -Destination $langOutput.FullName
}
foreach ($folder in @('Shaders','Shaders11')) {
    $path = Join-Path $binary $folder
    if (Test-Path -LiteralPath $path) { Copy-Item -LiteralPath $path -Destination $output -Recurse }
}
Copy-Item -LiteralPath (Join-Path $repo 'COPYING.txt') -Destination $output
Copy-Item -LiteralPath (Join-Path $repo 'docs\AudioMixer.md') -Destination (Join-Path $output 'LIRE-MOI.md')
# Presence of an INI selects portable settings, separate from an installed player.
Set-Content -LiteralPath (Join-Path $output 'mpc-hc64.ini') -Value "[Settings]`r`nUpdaterAutoCheck=0" -Encoding Unicode
$revision = & git -C $repo rev-parse HEAD
@{
    upstream = 'https://github.com/clsid2/mpc-hc'
    base_commit = $revision
    variant = 'Experimental multi-track audio mixer'
    source_directory = $repo
    license = 'GPL-3.0-or-later'
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'build-info.json') -Encoding UTF8
Get-ChildItem -LiteralPath $output -File -Recurse | ForEach-Object {
    $hash = Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256
    "$($hash.Hash)  $($_.FullName.Substring($output.Length + 1))"
} | Set-Content -LiteralPath (Join-Path $output 'SHA256SUMS.txt') -Encoding UTF8
Compress-Archive -LiteralPath $output -DestinationPath ($output + '.zip')
Write-Output ($output + '.zip')
