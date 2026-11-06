param([switch]$SingleTrack, [string]$MediaName = "long-30min.mp4", [string]$BinaryPath = "", [switch]$RestorePreferences, [string]$SavedIcon = "microphone", [int]$SavedVolume = 37, [int]$TestDpi = 96, [string]$SavedName = "Micro personnel", [switch]$ScrollRegression)
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
    [DllImport("user32.dll", CharSet=CharSet.Unicode, EntryPoint="SendMessageW")] public static extern IntPtr ReadText(IntPtr window, uint message, IntPtr count, System.Text.StringBuilder text);
    [DllImport("user32.dll", CharSet=CharSet.Unicode, EntryPoint="SendMessageW")] public static extern IntPtr TextMessage(IntPtr window, uint message, IntPtr wp, string text);
    [StructLayout(LayoutKind.Sequential)] public struct GuiInfo {
        public int Size, Flags;
        public IntPtr Active, Focus, Capture, MenuOwner, MoveSize, Caret;
        public Bounds CaretBounds;
    }
    [DllImport("user32.dll")] public static extern bool GetGUIThreadInfo(uint thread, ref GuiInfo info);
    public static IntPtr FocusFor(IntPtr window) {
        uint pid; uint thread=GetWindowThreadProcessId(window,out pid);
        var info=new GuiInfo(); info.Size=Marshal.SizeOf(info);
        return GetGUIThreadInfo(thread,ref info)?info.Focus:IntPtr.Zero;
    }
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr window, out uint pid);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr window, System.Text.StringBuilder name, int count);
    [DllImport("user32.dll")] public static extern int GetMenuItemCount(IntPtr menu);
    public static IntPtr PopupMenu(uint pid) {
        IntPtr found=IntPtr.Zero;
        EnumWindows((w,d)=>{ uint owner; GetWindowThreadProcessId(w,out owner); var name=new System.Text.StringBuilder(128); GetClassName(w,name,128); if(owner==pid && name.ToString()=="#32768") found=w; return true; },IntPtr.Zero);
        return found;
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
    Set-Content -LiteralPath $ini -Encoding Unicode -Value "[Settings]`r`nUpdaterAutoCheck=0`r`nDebugLogMask=3"
    if ($RestorePreferences) {
        Add-Content -LiteralPath $ini -Encoding Unicode -Value "`r`n[AudioMixer]`r`nHasProfile=1`r`nAutoApply=1`r`nTrack1Volume=$SavedVolume`r`nTrack1Mute=1`r`nTrack1Icon=$SavedIcon`r`nTrack1Name=$SavedName`r`nTrack2Volume=100`r`nTrack2Icon=gamepad`r`nTrack8Volume=73`r`nTrack8Icon=phone"
    }
    $media = Join-Path $repo $(if ($SingleTrack) { ('bin/mixer-tests/media/one-track'+[IO.Path]::GetExtension($MediaName)) } else { ("bin/mixer-tests/media/"+$MediaName) })
    $graphLog=Join-Path (Split-Path $binary) 'filtergraph.log'
    $graphsBefore=if(Test-Path $graphLog) { @(Select-String -LiteralPath $graphLog -Pattern 'Graph completed successfully').Count } else { 0 }
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
    if ($TestDpi -ne 96) {
        $dpiBounds = New-Object MixerWindows+Bounds
        [void][MixerWindows]::GetWindowRect($frame,[ref]$dpiBounds)
        $dpiPointer = [Runtime.InteropServices.Marshal]::AllocHGlobal(16)
        try {
            [Runtime.InteropServices.Marshal]::StructureToPtr($dpiBounds,$dpiPointer,$false)
            [void](Msg $frame 0x2e0 ($TestDpi -bor ($TestDpi -shl 16)) $dpiPointer)
        } finally { [Runtime.InteropServices.Marshal]::FreeHGlobal($dpiPointer) }
    }
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
    if ($RestorePreferences) {
        Assert ((Msg $slider 0x400) -eq $SavedVolume) 'Saved volume not restored'
        $restoredName=New-Object Text.StringBuilder 256
        [void][MixerWindows]::ReadText([MixerWindows]::Child($dialog,54000),0xd,[IntPtr]256,$restoredName)
        Assert ($restoredName.ToString() -eq $SavedName) 'Saved track name not restored'
        $iconControl = [MixerWindows]::Child($dialog,5103)
        Assert ($iconControl -ne [IntPtr]::Zero) 'Saved icon button missing'
        $iconText=New-Object Text.StringBuilder 256
        [void][MixerWindows]::GetWindowText($iconControl,$iconText,256)
        Assert ($iconText.Length -gt 0) 'Icon button has no accessible name'
        $muteText=New-Object Text.StringBuilder 256
        [void][MixerWindows]::GetWindowText([MixerWindows]::Child($dialog,5102),$muteText,256)
        Assert ($muteText.ToString().StartsWith('R')) 'Saved mute not restored'
        [void][MixerWindows]::PostMessage($frame,0x10,[IntPtr]::Zero,[IntPtr]::Zero)
        Assert ($player.WaitForExit(5000)) 'Player did not close'
        Assert (@(Select-String -LiteralPath $graphLog -Pattern 'Graph completed successfully').Count -eq ($graphsBefore+1)) 'Auto-apply failed: opening panel required graph reload'
        Assert ([IO.File]::ReadAllText($graphLog).Contains('Multi-track audio mixer')) 'Mixer source absent from startup graph'
        $savedText=[IO.File]::ReadAllText($ini)
        Assert ($savedText.Contains('Track1Volume='+$SavedVolume)) 'Volume lost on shutdown'
        Assert ($savedText.Contains('Track1Icon='+$SavedIcon)) 'Icon lost on shutdown'
        Assert ($savedText.Contains('Track1Name='+$SavedName)) 'Name lost on shutdown'
        Assert ($savedText.Contains('Track8Volume=73')) 'Absent track preference removed'
        "PASS: automatic restore, mute, icon $SavedIcon, volume $SavedVolume, absent tracks, DPI $TestDpi and shutdown persistence"
        return
    }
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
    if ($ScrollRegression) {
        Assert ([MixerWindows]::Child($dialog,5120) -ne [IntPtr]::Zero) 'Regression requires six tracks'
        [void](Msg $dialog 0x28 $slider 1)
        Assert ([MixerWindows]::FocusFor($dialog) -eq $slider) 'Could not focus first slider'
        foreach ($delta in @(-120,-120,120,120)) {
            $panelBounds=New-Object MixerWindows+Bounds; [void][MixerWindows]::GetWindowRect($dialog,[ref]$panelBounds)
            $px=$panelBounds.Left+20; $py=$panelBounds.Top+15
            $point=(($py -band 0xffff) -shl 16) -bor ($px -band 0xffff)
            [void][MixerWindows]::PostMessage($frame,0x20a,[IntPtr]($delta -shl 16),[IntPtr]$point)
            Start-Sleep -Milliseconds 150
            $focused=[MixerWindows]::FocusFor($dialog)
            $class=New-Object Text.StringBuilder 128
            [void][MixerWindows]::GetClassName($focused,$class,128)
            Assert ($class.ToString() -ne 'Edit') 'Wheel scrolling focused and selected a track name'
        }
        [void](Msg $dialog 0x28 $slider 1)
        foreach($code in @(7,6)) {
            [void](Msg $dialog 0x115 $code 0)
            $focused=[MixerWindows]::FocusFor($dialog)
            $class=New-Object Text.StringBuilder 128; [void][MixerWindows]::GetClassName($focused,$class,128)
            Assert ($class.ToString() -ne 'Edit') 'Scrollbar scrolling focused a track name'
        }
        [void](Msg $dialog 0x115 7 0)
        [void](Msg $dialog 0x111 2 0)
        [void][MixerWindows]::PostMessage($toolbar,0x111,[IntPtr]33465,$mixButton)
        Start-Sleep -Milliseconds 200
        $focused=[MixerWindows]::FocusFor($dialog)
        $class=New-Object Text.StringBuilder 128; [void][MixerWindows]::GetClassName($focused,$class,128)
        Assert ($class.ToString() -ne 'Edit') 'Reopening a scrolled panel focused a track name'
        [void](Msg $dialog 0x115 6 0)
        $editingName=[MixerWindows]::Child($dialog,54000)
        Assert ($editingName -ne [IntPtr]::Zero) 'Editing control disappeared after scrolling'
        [void](Msg $dialog 0x28 $editingName 1)
        [void][MixerWindows]::TextMessage($editingName,0xc,[IntPtr]::Zero,'Mon micro')
        $beforeScroll=New-Object Text.StringBuilder 256
        [void][MixerWindows]::ReadText($editingName,0xd,[IntPtr]256,$beforeScroll)
        Start-Sleep -Milliseconds 100
        [void](Msg $dialog 0x115 7 0)
        $focused=[MixerWindows]::FocusFor($dialog)
        $class=New-Object Text.StringBuilder 128; [void][MixerWindows]::GetClassName($focused,$class,128)
        Assert ($class.ToString() -ne 'Edit') 'Scrolling while renaming focused another title'
        $nameAfterScroll=New-Object Text.StringBuilder 256
        [void][MixerWindows]::ReadText($editingName,0xd,[IntPtr]256,$nameAfterScroll)
        Assert ($nameAfterScroll.ToString() -eq 'Mon micro') ('Scrolling discarded edited name: ['+$nameAfterScroll.ToString()+']')
        'PASS: wheel and scrollbar scrolling plus reopening keep focus outside track names'
        return
    }

    foreach ($id in @(5001,5002,5003,5004,5005)) {
        Assert ([MixerWindows]::Child($dialog,$id) -eq [IntPtr]::Zero) 'Removed control is still present'
    }
    Assert ([MixerWindows]::Child($dialog,5103) -ne [IntPtr]::Zero) 'Track icon selector missing'
    $nameControl=[MixerWindows]::Child($dialog,54000)
    Assert ($nameControl -ne [IntPtr]::Zero) 'Editable track name missing'
    $originalName=New-Object Text.StringBuilder 256
    [void][MixerWindows]::ReadText($nameControl,0xd,[IntPtr]256,$originalName)
    [void][MixerWindows]::TextMessage($nameControl,0xc,[IntPtr]::Zero,'  Micro équipe  ')
    Start-Sleep -Milliseconds 100
    $renamed=New-Object Text.StringBuilder 256
    [void][MixerWindows]::ReadText($nameControl,0xd,[IntPtr]256,$renamed)
    Assert ($renamed.ToString().Trim() -eq 'Micro équipe') ('Renamed track overwritten by refresh: ['+$renamed.ToString()+']')
    [void][MixerWindows]::TextMessage($nameControl,0xc,[IntPtr]::Zero,'')
    [void][MixerWindows]::PostMessage($nameControl,0x100,[IntPtr]13,[IntPtr]1)
    Start-Sleep -Milliseconds 100
    [void][MixerWindows]::ReadText($nameControl,0xd,[IntPtr]256,$renamed)
    Assert ($renamed.ToString() -eq $originalName.ToString()) 'Empty name did not restore source name'
    [void][MixerWindows]::TextMessage($nameControl,0xc,[IntPtr]::Zero,'Micro équipe')
    # Exercise the edit notifications and dialog key routing without global keyboard input.
    [void](Msg $dialog 0x111 (54000 -bor (0x100 -shl 16)) $nameControl)
    [void][MixerWindows]::TextMessage($nameControl,0xc,[IntPtr]::Zero,'Cancelled name')
    [void][MixerWindows]::PostMessage($nameControl,0x100,[IntPtr]27,[IntPtr]1)
    Start-Sleep -Milliseconds 150
    [void][MixerWindows]::ReadText($nameControl,0xd,[IntPtr]256,$renamed)
    Assert ($renamed.ToString().Trim() -eq 'Micro équipe') 'Escape did not restore name before editing'
    [void][MixerWindows]::TextMessage($nameControl,0xc,[IntPtr]::Zero,'Micro équipe')
    [void][MixerWindows]::PostMessage($nameControl,0x100,[IntPtr]13,[IntPtr]1)
    Start-Sleep -Milliseconds 100
    Assert (([MixerWindows]::GetWindowLong($dialog,-16) -band 0x10000000) -ne 0) 'Enter closed the mixer while editing a name'
    $iconControl=[MixerWindows]::Child($dialog,5103)
    $iconNames=@('Aucune icône','Micro','Manette','Personne','Groupe','Écran','Musique','Haut-parleur','Casque','Caméra','Navigateur','Téléphone')
    $iconIds=@('none','microphone','gamepad','person','group','screen','music','speaker','headphones','camera','browser','phone')
    for($choice=0;$choice -lt 12;++$choice) {
        [void][MixerWindows]::PostMessage($iconControl,0xf5,[IntPtr]::Zero,[IntPtr]::Zero)
        $deadline=[DateTime]::UtcNow.AddSeconds(3)
        do { Start-Sleep -Milliseconds 50; $popup=[MixerWindows]::PopupMenu($player.Id) } while($popup -eq [IntPtr]::Zero -and [DateTime]::UtcNow -lt $deadline)
        Assert ($popup -ne [IntPtr]::Zero) 'Icon choice menu missing'
        $menuHandle=[MixerWindows]::SendMessage($popup,0x1e1,[IntPtr]::Zero,[IntPtr]::Zero)
        Assert ([MixerWindows]::GetMenuItemCount($menuHandle) -eq 12) 'Expected twelve icon choices'
        [void][MixerWindows]::PostMessage($frame,0x100,[IntPtr]0x1b,[IntPtr]1)
        Start-Sleep -Milliseconds 50
        [void][MixerWindows]::PostMessage($dialog,0x111,[IntPtr](53000+$choice),[IntPtr]::Zero)
        Start-Sleep -Milliseconds 100
        $selectedName=New-Object Text.StringBuilder 256
        [void][MixerWindows]::GetWindowText($iconControl,$selectedName,256)
        Assert ($selectedName.ToString().Contains($iconNames[$choice])) ('Icon selection failed: '+$iconIds[$choice]+' actual='+$selectedName.ToString())
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
    $iconBounds=New-Object MixerWindows+Bounds
    [void][MixerWindows]::GetWindowRect($iconControl,[ref]$iconBounds)
    $iconPixels=0
    for($ix=$iconBounds.Left-$db.Left;$ix -lt $iconBounds.Right-$db.Left;++$ix) {
        for($iy=$iconBounds.Top-$db.Top;$iy -lt $iconBounds.Bottom-$db.Top;++$iy) {
            $pixel=$image.GetPixel($ix,$iy)
            if($pixel.R -gt 60 -and $pixel.G -gt 60 -and $pixel.B -gt 60) { ++$iconPixels }
        }
    }
    Assert ($iconPixels -gt 5) 'Selected icon is not visibly rendered'
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
    [void][MixerWindows]::PostMessage($frame,0x10,[IntPtr]::Zero,[IntPtr]::Zero)
    Assert ($player.WaitForExit(5000)) 'Shutdown timeout'
    $persisted=[IO.File]::ReadAllText($ini)
    Assert ($persisted.Contains('Track1Volume=42')) 'Changed volume was not saved'
    Assert ($persisted.Contains('Track1Icon=phone')) 'Changed icon was not saved'
    Assert ($persisted.Contains('Track1Name=Micro équipe')) 'Changed name was not saved'

    'PASS: compiled player toolbar access, owned popup, unchanged video area, activation/reload, automatic activation, simplified controls, native track sliders, independent master, owner movement, dismissal and wheel routing'
} finally {
    if ($player -and -not $player.HasExited) {
        [void][MixerWindows]::PostMessage($player.MainWindowHandle, 0x10, [IntPtr]::Zero, [IntPtr]::Zero)
        if (-not $player.WaitForExit(5000)) { $player.Kill(); $player.WaitForExit() }
    }
    if ($null -ne $previous) { [IO.File]::WriteAllBytes($ini,$previous) }
    elseif (Test-Path -LiteralPath $ini) { Remove-Item -LiteralPath $ini }
}
