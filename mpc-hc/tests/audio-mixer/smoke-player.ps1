$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$binary = Join-Path $repo 'bin\mpc-hc_x64\mpc-hc64.exe'
$ini = [IO.Path]::ChangeExtension($binary, '.ini')
$previous = if (Test-Path -LiteralPath $ini) { [IO.File]::ReadAllBytes($ini) } else { $null }
try {
    Set-Content -LiteralPath $ini -Encoding Unicode -Value "[Settings]`r`nUpdaterAutoCheck=0`r`nDebugLogMask=3"
    foreach ($name in @('three-tracks.mkv', 'three-tracks.mp4')) {
        $media = Join-Path $repo "bin\mixer-tests\media\$name"
        $elapsed = [Diagnostics.Stopwatch]::StartNew()
        $process = Start-Process -FilePath $binary -ArgumentList @('/new','/minimized','/volume','0','/play','/close',('"' + $media + '"')) -WindowStyle Hidden -PassThru
        if (-not $process.WaitForExit(20000)) {
            $process.Kill()
            throw "Player did not finish: $name"
        }
        $elapsed.Stop()
        $process.Refresh()
        if ($process.ExitCode -ne 0 -or $elapsed.Elapsed.TotalSeconds -lt 7) {
            throw "Playback failed or ended prematurely: $name, exit=$($process.ExitCode), elapsed=$($elapsed.Elapsed.TotalSeconds)"
        }
        "PASS: normal playback with mixer disabled: $name, $([Math]::Round($elapsed.Elapsed.TotalSeconds,2))s, exit=$($process.ExitCode)"
    }
} finally {
    if ($null -ne $previous) { [IO.File]::WriteAllBytes($ini, $previous) }
    elseif (Test-Path -LiteralPath $ini) { Remove-Item -LiteralPath $ini }
}
