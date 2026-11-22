#requires -Version 5.1
# End-to-end check of a packaged CoinRush build:
#   launch -> drive a short input sequence -> screenshot -> wait for round end ->
#   screenshot end screen -> report whether the save file was written.
# Usage: powershell -ExecutionPolicy Bypass -File Tools\VerifyPackage.ps1
# NOTE: keep this file ASCII-only (PowerShell 5.1 reads BOM-less files as ANSI).

param(
    [string]$PackageRoot = 'Saved\Package\Shipping\Windows',
    [string]$ExeName = 'CoinRush-Win64-Shipping.exe',
    [string]$Seq = 'E:0.82|W:1.3',
    [int]$StartDelaySeconds = 10,
    [int]$RoundSeconds = 75,
    [int]$ExtraWaitSeconds = 12
)

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$Pkg = Join-Path $Root $PackageRoot
$Exe = Join-Path $Pkg ('CoinRush\Binaries\Win64\' + $ExeName)
$Shots = Join-Path $Root 'Saved\Screenshots'
New-Item -ItemType Directory -Force -Path $Shots | Out-Null

if (-not (Test-Path $Exe)) { throw "packaged exe not found: $Exe" }

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
public static class PkgWin {
    public delegate bool EnumProc(IntPtr h, IntPtr p);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr p);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int c);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint msg, IntPtr w, IntPtr l);

    public static List<IntPtr> ForPid(uint target) {
        var list = new List<IntPtr>();
        EnumWindows((h, p) => {
            uint pid; GetWindowThreadProcessId(h, out pid);
            if (pid == target && IsWindowVisible(h)) list.Add(h);
            return true;
        }, IntPtr.Zero);
        return list;
    }

    public static IntPtr Largest(uint pid) {
        IntPtr best = IntPtr.Zero; int area = 0;
        foreach (IntPtr h in ForPid(pid)) {
            RECT r; if (!GetWindowRect(h, out r)) continue;
            int a = (r.Right - r.Left) * (r.Bottom - r.Top);
            if (a > area) { area = a; best = h; }
        }
        return best;
    }

    public static void Key(IntPtr h, int vk, bool down) {
        uint msg = down ? 0x0100u : 0x0101u;
        PostMessage(h, msg, (IntPtr)vk, (IntPtr)0);
    }

    public static bool Capture(IntPtr h, string path) {
        RECT r; if (!GetWindowRect(h, out r)) return false;
        int w = r.Right - r.Left, ht = r.Bottom - r.Top;
        if (w <= 0 || ht <= 0) return false;
        using (var bmp = new System.Drawing.Bitmap(w, ht)) {
            using (var g = System.Drawing.Graphics.FromImage(bmp)) {
                g.CopyFromScreen(r.Left, r.Top, 0, 0, bmp.Size);
            }
            bmp.Save(path, System.Drawing.Imaging.ImageFormat.Png);
        }
        return true;
    }
}
"@ -ReferencedAssemblies System.Drawing

$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = $Exe
$psi.Arguments = '-windowed -resx=1280 -resy=720'
$psi.WorkingDirectory = Split-Path -Parent $Exe
$psi.UseShellExecute = $false
$proc = [System.Diagnostics.Process]::Start($psi)
$start = Get-Date
Write-Host "launched pid=$($proc.Id) at $($start.ToString('HH:mm:ss'))"

Start-Sleep -Seconds $StartDelaySeconds
if ($proc.HasExited) { throw "game exited during startup (code $($proc.ExitCode))" }

$hwnd = [PkgWin]::Largest([uint32]$proc.Id)
Write-Host "window=$hwnd"
[void][PkgWin]::ShowWindow($hwnd, 9)
[void][PkgWin]::SetForegroundWindow($hwnd)
Start-Sleep -Milliseconds 800

# Sequence "E:0.82|W:1.3" -> turn right, then walk.
foreach ($step in $Seq.Split('|')) {
    $parts = $step.Split(':')
    $keys = $parts[0].ToUpper()
    $duration = [double]::Parse($parts[1], [Globalization.CultureInfo]::InvariantCulture)
    $vks = @()
    foreach ($ch in $keys.ToCharArray()) { $vks += [int][char]$ch }
    Write-Host "  hold $keys for $duration s"
    foreach ($vk in $vks) { [PkgWin]::Key($hwnd, $vk, $true) }
    Start-Sleep -Milliseconds ([int]($duration * 1000))
    foreach ($vk in $vks) { [PkgWin]::Key($hwnd, $vk, $false) }
    Start-Sleep -Milliseconds 250
}

$mid = Join-Path $Shots 'Pkg_MidRun.png'
Start-Sleep -Milliseconds 500
Write-Host ("mid screenshot: " + [PkgWin]::Capture($hwnd, $mid))

$elapsed = ((Get-Date) - $start).TotalSeconds
$remaining = [Math]::Max(0, $RoundSeconds + $ExtraWaitSeconds - $elapsed)
Write-Host ("waiting $([int]$remaining) s for round end ...")
Start-Sleep -Seconds ([int]$remaining)

$end = Join-Path $Shots 'Pkg_EndScreen.png'
$alive = -not $proc.HasExited
Write-Host "alive_after_round=$alive"
if ($alive) { Write-Host ("end screenshot: " + [PkgWin]::Capture([PkgWin]::Largest([uint32]$proc.Id), $end)) }

$sav = Get-ChildItem $Pkg -Recurse -Filter '*.sav' -ErrorAction SilentlyContinue
Write-Host ("save_file=" + (($sav | ForEach-Object { $_.FullName + ' (' + $_.Length + ' bytes)' }) -join '; '))

if (-not $proc.HasExited) { $proc.Kill(); Write-Host 'game closed' }
Write-Host 'DONE'
