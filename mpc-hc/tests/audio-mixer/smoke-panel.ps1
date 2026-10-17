param([switch]$SingleTrack, [string]$MediaName = "long-30min.mp4", [string]$BinaryPath = "")
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class MixerWindows {
    [StructLayout(LayoutKind.Sequential)] public struct Placement {
        public int Length, Flags, Show;
        public int MinX, MinY, MaxX, MaxY;
        public Bounds Normal;
    }
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr window, IntPtr dc, uint flags);
    [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr w, IntPtr after, int x, int y, int cx, int cy, uint flags);
    [DllImport("user32.dll")] public static extern bool SetWindowPlacement(IntPtr window, ref Placement placement);
    [StructLayout(LayoutKind.Sequential)] public struct Bounds { public int Left, Top, Right, Bottom; }
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr window, out Bounds bounds);
    public delegate bool EnumProc(IntPtr window, IntPtr data);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc proc, IntPtr data);
    [DllImport("user32.dll")] public static extern IntPtr GetWindow(IntPtr window, uint command);
    [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr window, EnumProc proc, IntPtr data);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowText(IntPtr window, System.Text.StringBuilder text, int count);
    [DllImport("user32.dll")] public static extern int GetDlgCtrlID(IntPtr window);
    [DllImport("user32.dll")] public static extern IntPtr GetParent(IntPtr window);
    [DllImport("user32.dll")] public static extern int GetWindowLong(IntPtr window, int index);
    [DllImport("user32.dll")] public static extern bool IsWindowEnabled(IntPtr window);
    [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr window, uint message, IntPtr wp, IntPtr lp);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr window, uint message, IntPtr wp, IntPtr lp);
    public static IntPtr OwnedMixer(IntPtr owner) {
        IntPtr found = IntPtr.Zero;
        EnumWindows((w,d) => { if (GetWindow(w,4)==owner && Child(w,5100)!=IntPtr.Zero) { found=w; return false; } return true; },IntPtr.Zero);
        return found;
    }
    public static IntPtr Child(IntPtr root, int id) {
        IntPtr found = IntPtr.Zero;
        EnumChildWindows(root, (w,d) => { if (GetDlgCtrlID(w)==id) { found=w; return false; } return true; }, IntPtr.Zero);
        return found;
    }
}
"@
function Assert($condition, $message) { if (-not $condition) { throw $message } }
function Msg($window, $message, $wp=0, $lp=0) {
    [MixerWindows]::SendMessage($window, $message, [IntPtr]$wp, [IntPtr]$lp).ToInt64()
}
$binary = if ($BinaryPath) { (Resolve-Path -LiteralPath $BinaryPath).Path } else { Join-Path $repo 'bin\mpc-hc_x64\mpc-hc64.exe' }
$ini = [IO.Path]::ChangeExtension($binary, '.ini')
$previous = if (Test-Path -LiteralPath $ini) { [IO.File]::ReadAllBytes($ini) } else { $null }
$player = $null
try {
    Set-Content -LiteralPath $ini -Encoding Unicode -Value "[Settings]`r`nUpdaterAutoCheck=0"
    $media = Join-Path $repo $(if ($SingleTrack) { ('bin/mixer-tests/media/one-track'+[IO.Path]::GetExtension($MediaName)) } else { ("bin/mixer-tests/media/"+$MediaName) })
    $player = Start-Process -FilePath $binary -ArgumentList @('/new','/nofocus','/fixedsize','1100,700,-25000,-25000','/volume','0','/play',('"'+$media+'"')) -WindowStyle Hidden -PassThru
    $deadline = [DateTime]::UtcNow.AddSeconds(15)
    do { Start-Sleep -Milliseconds 200; $player.Refresh(); $frame = $player.MainWindowHandle } while ($frame -eq [IntPtr]::Zero -and [DateTime]::UtcNow -lt $deadline)
    Assert ($frame -ne [IntPtr]::Zero) 'Player window not created'
    # Real client geometry without displaying or activating the test player onscreen.
    $placement = New-Object MixerWindows+Placement
    $placement.Length = [Runtime.InteropServices.Marshal]::SizeOf($placement)
    $placement.Show = 4
    $placement.Normal.Left = -25000; $placement.Normal.Top = -25000
    $placement.Normal.Right = -23900; $placement.Normal.Bottom = -24300
    [void][MixerWindows]::SetWindowPlacement($frame,[ref]$placement)
    [void][MixerWindows]::SetWindowPos($frame,[IntPtr]::Zero,-25000,-25000,1100,700,0x414)
    $toolbar = [MixerWindows]::Child($frame, 0xE800)
    Assert ($toolbar -ne [IntPtr]::Zero) 'Playback toolbar missing'
    Start-Sleep -Milliseconds 1500
    Assert ((Msg $toolbar 0x419 33465) -ge 0) 'Direct mixer toolbar button missing'
    $mixButton = [MixerWindows]::Child($toolbar,33465)
    Assert ($mixButton -ne [IntPtr]::Zero) 'Hosted Mixage button missing'
    if ($SingleTrack) {
        Assert (-not [MixerWindows]::IsWindowEnabled($mixButton)) 'Mixer button enabled for a single audio track'
        [void](Msg $frame 0x111 33465)
        Assert ([MixerWindows]::OwnedMixer($frame) -eq [IntPtr]::Zero) 'Command opened mixer for a single track'
        'PASS: single audio track disables both mixer button and command'
        return
    }
    Assert ([MixerWindows]::IsWindowEnabled($mixButton)) 'Mixer disabled despite multiple tracks'

    $mixBounds = New-Object MixerWindows+Bounds
    [void][MixerWindows]::GetWindowRect($mixButton,[ref]$mixBounds)
    Assert (($mixBounds.Right-$mixBounds.Left) -ge 24) 'Mixer icon would be clipped'
    $volumeBounds = New-Object MixerWindows+Bounds
    [void][MixerWindows]::GetWindowRect([MixerWindows]::Child($toolbar,11020),[ref]$volumeBounds)
    Assert (($volumeBounds.Left-$mixBounds.Right) -gt 0 -and ($volumeBounds.Left-$mixBounds.Right) -lt 80) 'Mixer icon is not beside volume on the right'
    $view = [MixerWindows]::Child($frame,0xe900)
    $viewBefore = New-Object MixerWindows+Bounds; [void][MixerWindows]::GetWindowRect($view,[ref]$viewBefore)
    $deadline = [DateTime]::UtcNow.AddSeconds(15)
    do {
        [void][MixerWindows]::PostMessage($toolbar, 0x111, [IntPtr]33465, $mixButton)
        Start-Sleep -Milliseconds 400
        $dialog = [MixerWindows]::OwnedMixer($frame)
        $slider = [MixerWindows]::Child($dialog, 5100)
    } while ($slider -eq [IntPtr]::Zero -and [DateTime]::UtcNow -lt $deadline)
    Assert ($slider -ne [IntPtr]::Zero) 'Attached mixer not created'
    $dialog = [MixerWindows]::GetParent($slider)
    $panel = $dialog
    Assert (([MixerWindows]::GetWindowLong($dialog,-16) -band 0x40000000) -eq 0) 'Mixer should be an owned popup'
    Assert ([MixerWindows]::GetWindow($dialog,4) -eq $frame) 'Popup is disconnected from player'
    Assert (([MixerWindows]::GetWindowLong($dialog,-20) -band 0x80) -ne 0) 'Popup would have a separate taskbar entry'
    $viewAfter = New-Object MixerWindows+Bounds; [void][MixerWindows]::GetWindowRect($view,[ref]$viewAfter)
    Assert (($viewAfter.Bottom-$viewAfter.Top) -eq ($viewBefore.Bottom-$viewBefore.Top)) 'Popup shrank video area'
    $deadline = [DateTime]::UtcNow.AddSeconds(15)
    do { Start-Sleep -Milliseconds 250; $slider = [MixerWindows]::Child($dialog,5100) } while (-not [MixerWindows]::IsWindowEnabled($slider) -and [DateTime]::UtcNow -lt $deadline)
    Assert ([MixerWindows]::IsWindowEnabled($slider)) 'Mixing did not activate after graph reload'
    foreach ($id in @(5001,5002,5003,5004,5005,5103)) {
        Assert ([MixerWindows]::Child($dialog,$id) -eq [IntPtr]::Zero) 'Removed control is still present'
    }
    $mute = [MixerWindows]::Child($dialog,5102)
    Assert ($mute -ne [IntPtr]::Zero) 'Per-track mute button missing'
    Assert ([MixerWindows]::IsWindowEnabled($mute)) 'Per-track mute disabled'
    $muteText = New-Object Text.StringBuilder 128
    $trackBefore = Msg $slider 0x400
    [void](Msg $mute 0xf5)
    Start-Sleep -Milliseconds 100
    [void][MixerWindows]::GetWindowText($mute,$muteText,128)
    Assert ($muteText.ToString() -like 'R*') 'Mute state did not toggle'
    Assert ((Msg $slider 0x400) -eq $trackBefore) 'Mute changed saved track volume'
    [void](Msg $mute 0xf5)
    Start-Sleep -Milliseconds 100
    [void][MixerWindows]::GetWindowText($mute,$muteText,128)
    Assert ($muteText.ToString() -like 'Couper*') 'Unmute state did not toggle back'
    Assert ((Msg $slider 0x400) -eq $trackBefore) 'Unmute changed saved track volume'
    [void](Msg $slider 0x405 1 37)
    [void][MixerWindows]::PostMessage($dialog, 0x114, [IntPtr]5, $slider)
    Start-Sleep -Milliseconds 200
    [void][MixerWindows]::PostMessage($frame, 0x111, [IntPtr]33465, [IntPtr]::Zero)
    Start-Sleep -Milliseconds 200
    Assert (([MixerWindows]::GetWindowLong($panel,-16) -band 0x10000000) -eq 0) 'Toolbar command did not collapse panel'
    [void][MixerWindows]::PostMessage($frame, 0x111, [IntPtr]33465, [IntPtr]::Zero)
    Start-Sleep -Milliseconds 200
    Assert (([MixerWindows]::GetWindowLong($panel,-16) -band 0x10000000) -ne 0) 'Toolbar command did not expand panel'
    Assert ((Msg $slider 0x400) -eq 37) 'Mix or volume lost across collapse'
    # Both event orders: activation dismisses the popup before the button releases,
    # or the owner is already active. Each click must toggle exactly once.
    foreach ($deactivate in @($true,$false,$true,$true,$false)) {
        [void](Msg $mixButton 0x21 0 (0x2010001))
        if ($deactivate) { [void](Msg $dialog 6 0 $frame) }
        [void](Msg $mixButton 0x201 1 (5 -bor (5 -shl 16)))
        [void](Msg $mixButton 0x202 0 (5 -bor (5 -shl 16)))
        Start-Sleep -Milliseconds 80
        Assert (([MixerWindows]::GetWindowLong($dialog,-16) -band 0x10000000) -eq 0) 'Icon close click reopened the popup'
        [void](Msg $mixButton 0x21 0 (0x2010001))
        [void](Msg $mixButton 0x201 1 (5 -bor (5 -shl 16)))
        [void](Msg $mixButton 0x202 0 (5 -bor (5 -shl 16)))
        Start-Sleep -Milliseconds 80
        Assert (([MixerWindows]::GetWindowLong($dialog,-16) -band 0x10000000) -ne 0) 'Next icon click did not reopen the popup'
    }
    $ownerBefore = New-Object MixerWindows+Bounds; [void][MixerWindows]::GetWindowRect($frame,[ref]$ownerBefore)
    $popupBefore = New-Object MixerWindows+Bounds; [void][MixerWindows]::GetWindowRect($dialog,[ref]$popupBefore)
    [void][MixerWindows]::SetWindowPos($frame,[IntPtr]::Zero,$ownerBefore.Left+30,$ownerBefore.Top+20,1100,700,0x414)
    Start-Sleep -Milliseconds 200
    $ownerAfter = New-Object MixerWindows+Bounds; [void][MixerWindows]::GetWindowRect($frame,[ref]$ownerAfter)
    $popupAfter = New-Object MixerWindows+Bounds; [void][MixerWindows]::GetWindowRect($dialog,[ref]$popupAfter)
    Assert (($popupAfter.Left-$popupBefore.Left) -eq ($ownerAfter.Left-$ownerBefore.Left)) 'Popup did not follow owner movement'
    $general = [MixerWindows]::Child($toolbar,11020)
    $generalBefore = Msg $general 0x400
    [void](Msg $slider 0x201 1 (100 -bor (10 -shl 16)))
    [void](Msg $slider 0x202 0 (100 -bor (10 -shl 16)))
    Start-Sleep -Milliseconds 100
    Assert ((Msg $general 0x400) -eq $generalBefore) 'Track click changed native master volume'
    [void](Msg $slider 0x405 1 37)
    [void](Msg $dialog 0x114 5 $slider)

    $bounds = New-Object MixerWindows+Bounds
    [void][MixerWindows]::GetWindowRect($slider,[ref]$bounds)
    $px = [int](($bounds.Left+$bounds.Right)/2); $py = [int](($bounds.Top+$bounds.Bottom)/2)
    $point = (($py -band 0xffff) -shl 16) -bor ($px -band 0xffff)
    [void][MixerWindows]::PostMessage($frame, 0x20a, [IntPtr](120 -shl 16), [IntPtr]$point)
    Start-Sleep -Milliseconds 250
    $db = New-Object MixerWindows+Bounds; [void][MixerWindows]::GetWindowRect($dialog,[ref]$db)
    Assert ((Msg $slider 0x400) -eq 42) 'Wheel routing did not change the hovered track volume'
    Assert ((Msg $general 0x400) -eq $generalBefore) 'Wheel over mixer changed general volume'
    $firstBounds = New-Object MixerWindows+Bounds
    [void][MixerWindows]::GetWindowRect($slider,[ref]$firstBounds)
    $lastBounds = $firstBounds
    for ($i=1; $i -lt 4; ++$i) {
        $row = [MixerWindows]::Child($dialog,(5100+$i*4))
        if ($row -ne [IntPtr]::Zero) {
            $lastBounds = New-Object MixerWindows+Bounds
            [void][MixerWindows]::GetWindowRect($row,[ref]$lastBounds)
        }
    }
    Assert ([Math]::Abs(($firstBounds.Top-$db.Top)-($db.Bottom-$lastBounds.Bottom)) -le 2) 'Top and bottom margins differ'
    $meter = [MixerWindows]::Child($dialog,5101)
    Assert ($meter -ne [IntPtr]::Zero) 'Horizontal level meter missing'
    $meterBounds = New-Object MixerWindows+Bounds
    [void][MixerWindows]::GetWindowRect($meter,[ref]$meterBounds)
    Assert (($meterBounds.Right-$meterBounds.Left) -gt ($meterBounds.Bottom-$meterBounds.Top)) 'Level meter is not horizontal'
    Assert ($meterBounds.Right -lt $firstBounds.Left) 'Level meter must precede volume slider'
    Add-Type -AssemblyName System.Drawing
    $image = New-Object Drawing.Bitmap(($db.Right-$db.Left),($db.Bottom-$db.Top))
    $graphics = [Drawing.Graphics]::FromImage($image)
    $dc = $graphics.GetHdc()
    try { [void][MixerWindows]::PrintWindow($dialog,$dc,2) } finally { $graphics.ReleaseHdc($dc) }
    $image.Save((Join-Path $repo 'bin/mixer-tests/compact-panel.png'),[Drawing.Imaging.ImageFormat]::Png)
    $graphics.Dispose(); $image.Dispose()
    [void](Msg $mixButton 0x200 0 (5 -bor (5 -shl 16)))
    Assert ((Msg $mixButton 0x20 0 (1 -bor (0x200 -shl 16))) -eq 1) 'Mixer hand cursor not handled'
    [void](Msg $mixButton 0xf)
    [void][MixerWindows]::GetWindowRect($toolbar,[ref]$db)
    $image = New-Object Drawing.Bitmap(($db.Right-$db.Left),($db.Bottom-$db.Top))
    $graphics = [Drawing.Graphics]::FromImage($image); $dc = $graphics.GetHdc()
    try { [void][MixerWindows]::PrintWindow($toolbar,$dc,2) } finally { $graphics.ReleaseHdc($dc) }
    $image.Save((Join-Path $repo 'bin/mixer-tests/compact-toolbar.png'),[Drawing.Imaging.ImageFormat]::Png)
    $graphics.Dispose(); $image.Dispose()
    [void](Msg $mixButton 0x2a3)
    [void](Msg $dialog 6 0 $frame)
    Assert (([MixerWindows]::GetWindowLong($dialog,-16) -band 0x10000000) -eq 0) 'Deactivation did not dismiss popup'
    Assert ((Msg $slider 0x400) -eq 42) 'Dismissal changed the mix'
    Assert (-not $player.HasExited) 'Player exited unexpectedly'
    'PASS: compiled player toolbar access, owned popup, unchanged video area, activation/reload, automatic activation, simplified controls, native track sliders, independent master, owner movement, dismissal and wheel routing'
} finally {
    if ($player -and -not $player.HasExited) {
        [void][MixerWindows]::PostMessage($player.MainWindowHandle, 0x10, [IntPtr]::Zero, [IntPtr]::Zero)
        if (-not $player.WaitForExit(5000)) { $player.Kill(); $player.WaitForExit() }
    }
    if ($null -ne $previous) { [IO.File]::WriteAllBytes($ini,$previous) }
    elseif (Test-Path -LiteralPath $ini) { Remove-Item -LiteralPath $ini }
}
