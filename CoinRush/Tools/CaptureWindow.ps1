#requires -Version 5.1
# Captures the main window of a running process into a PNG (works for standalone game windows).
#   .\CaptureWindow.ps1 -ProcessName CoinRush -OutFile shot.png -WaitSeconds 12

param(
    [Parameter(Mandatory = $true)][string]$ProcessName,
    [Parameter(Mandatory = $true)][string]$OutFile,
    [int]$WaitSeconds = 0
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Collections.Generic;

public static class WinCap
{
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr extra);

    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc cb, IntPtr extra);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint pid);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT r);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hWnd, out RECT r);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr hWnd, ref POINT p);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hwnd, IntPtr hdc, uint flags);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hWnd, int cmd);

    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }

    public static List<IntPtr> WindowsOf(int pid)
    {
        var list = new List<IntPtr>();
        EnumWindows((h, e) =>
        {
            uint w; GetWindowThreadProcessId(h, out w);
            if (w == pid && IsWindowVisible(h)) list.Add(h);
            return true;
        }, IntPtr.Zero);
        return list;
    }
}
"@

[void][WinCap]::SetProcessDPIAware()

if ($WaitSeconds -gt 0) { Start-Sleep -Seconds $WaitSeconds }

$proc = Get-Process -Name $ProcessName -ErrorAction SilentlyContinue | Sort-Object StartTime -Descending | Select-Object -First 1
if (-not $proc) { throw "process '$ProcessName' is not running" }

$best = [IntPtr]::Zero
$bestArea = 0
foreach ($h in [WinCap]::WindowsOf($proc.Id)) {
    $r = New-Object WinCap+RECT
    if ([WinCap]::GetWindowRect($h, [ref]$r)) {
        $area = ($r.Right - $r.Left) * ($r.Bottom - $r.Top)
        if ($area -gt $bestArea) { $bestArea = $area; $best = $h }
    }
}
if ($best -eq [IntPtr]::Zero) { throw "no visible window for '$ProcessName'" }

[void][WinCap]::ShowWindow($best, 9)
[void][WinCap]::SetForegroundWindow($best)
Start-Sleep -Milliseconds 700

$win = New-Object WinCap+RECT
[void][WinCap]::GetWindowRect($best, [ref]$win)
$client = New-Object WinCap+RECT
[void][WinCap]::GetClientRect($best, [ref]$client)
$origin = New-Object WinCap+POINT
[void][WinCap]::ClientToScreen($best, [ref]$origin)

$ww = $win.Right - $win.Left; $wh = $win.Bottom - $win.Top
$w = $client.Right - $client.Left; $h = $client.Bottom - $client.Top
$offX = [Math]::Max(0, $origin.X - $win.Left); $offY = [Math]::Max(0, $origin.Y - $win.Top)

$full = New-Object System.Drawing.Bitmap($ww, $wh)
$g = [System.Drawing.Graphics]::FromImage($full)
$hdc = $g.GetHdc()
[void][WinCap]::PrintWindow($best, $hdc, 2)
$g.ReleaseHdc($hdc)
$g.Dispose()

$dir = Split-Path -Parent $OutFile
if ($dir -and -not (Test-Path $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
$crop = New-Object System.Drawing.Rectangle($offX, $offY, $w, $h)
$bmp = $full.Clone($crop, $full.PixelFormat)
$bmp.Save($OutFile, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose(); $full.Dispose()

Write-Host "captured '$($proc.ProcessName)' ${w}x${h} -> $OutFile"
