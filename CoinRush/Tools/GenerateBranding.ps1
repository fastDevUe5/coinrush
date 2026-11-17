#requires -Version 5.1
# Generates the Windows app icon and the game splash screen (no external art).
#   Build\Windows\Application.ico   <- picked up by UBT automatically (UEBuildWindows.cs)
#   Content\Splash\Splash.bmp       <- game splash (WinPlatform.Automation.cs)
# Usage:  powershell -ExecutionPolicy Bypass -File Tools\GenerateBranding.ps1
# NOTE: keep ASCII-only (PowerShell 5.1 reads BOM-less files as ANSI).

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
Add-Type -AssemblyName System.Drawing

$Navy = [System.Drawing.Color]::FromArgb(255, 11, 26, 42)
$Gold = [System.Drawing.Color]::FromArgb(255, 255, 194, 63)
$GoldDark = [System.Drawing.Color]::FromArgb(255, 191, 122, 18)
$Paper = [System.Drawing.Color]::FromArgb(255, 235, 245, 255)

function New-CoinBitmap([int]$Size) {
    $bmp = New-Object System.Drawing.Bitmap($Size, $Size)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic

    $g.Clear($Navy)
    $pad = [int]($Size * 0.10)
    $coin = New-Object System.Drawing.Rectangle($pad, $pad, ($Size - 2 * $pad), ($Size - 2 * $pad))

    $brush = New-Object System.Drawing.SolidBrush($Gold)
    $g.FillEllipse($brush, $coin)

    $ring = New-Object System.Drawing.Pen($GoldDark, [Math]::Max(2, $Size * 0.035))
    $inner = New-Object System.Drawing.Rectangle(($pad + $Size * 0.06), ($pad + $Size * 0.06), ($Size - 2 * $pad - $Size * 0.12), ($Size - 2 * $pad - $Size * 0.12))
    $g.DrawEllipse($ring, $inner)

    $fontSize = [Math]::Max(8, $Size * 0.42)
    $font = New-Object System.Drawing.Font('Arial', $fontSize, [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
    $fmt = New-Object System.Drawing.StringFormat
    $fmt.Alignment = [System.Drawing.StringAlignment]::Center
    $fmt.LineAlignment = [System.Drawing.StringAlignment]::Center
    $textBrush = New-Object System.Drawing.SolidBrush($Navy)
    $g.DrawString('C', $font, $textBrush, (New-Object System.Drawing.RectangleF(0, 0, $Size, $Size)), $fmt)

    $g.Dispose()
    return $bmp
}

function Save-Ico([string]$Path, [System.Drawing.Bitmap]$Bitmap) {
    $w = $Bitmap.Width
    $h = $Bitmap.Height
    $xorSize = $w * $h * 4
    $maskSize = [int]($w * $h / 8)
    $imageSize = 40 + $xorSize + $maskSize

    $ms = New-Object System.IO.MemoryStream
    $bw = New-Object System.IO.BinaryWriter($ms)

    # ICONDIR
    $bw.Write([UInt16]0); $bw.Write([UInt16]1); $bw.Write([UInt16]1)
    # ICONDIRENTRY (0 means 256)
    $bw.Write([byte]0); $bw.Write([byte]0); $bw.Write([byte]0); $bw.Write([byte]0)
    $bw.Write([UInt16]1); $bw.Write([UInt16]32)
    $bw.Write([UInt32]$imageSize); $bw.Write([UInt32]22)
    # BITMAPINFOHEADER (height doubled to cover the AND mask)
    $bw.Write([UInt32]40); $bw.Write([Int32]$w); $bw.Write([Int32]($h * 2))
    $bw.Write([UInt16]1); $bw.Write([UInt16]32)
    $bw.Write([UInt32]0); $bw.Write([UInt32]$xorSize)
    $bw.Write([Int32]0); $bw.Write([Int32]0); $bw.Write([UInt32]0); $bw.Write([UInt32]0)
    # XOR bitmap, bottom-up BGRA
    for ($y = $h - 1; $y -ge 0; $y--) {
        for ($x = 0; $x -lt $w; $x++) {
            $c = $Bitmap.GetPixel($x, $y)
            $bw.Write([byte]$c.B); $bw.Write([byte]$c.G); $bw.Write([byte]$c.R); $bw.Write([byte]$c.A)
        }
    }
    # AND mask: all zero (opaque)
    $bw.Write((New-Object byte[] $maskSize))
    $bw.Flush()

    $dir = Split-Path -Parent $Path
    if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
    [System.IO.File]::WriteAllBytes($Path, $ms.ToArray())
    $bw.Dispose(); $ms.Dispose()
    Write-Host ("wrote {0} ({1:N0} bytes)" -f $Path, (Get-Item $Path).Length)
}

function Save-Splash([string]$Path, [int]$Width, [int]$Height) {
    $bmp = New-Object System.Drawing.Bitmap($Width, $Height)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias

    $bgBrush = New-Object System.Drawing.SolidBrush($Navy)
    $g.FillRectangle($bgBrush, 0, 0, $Width, $Height)

    $coinSize = [int]($Height * 0.34)
    $coinX = [int](($Width - $coinSize) / 2)
    $coinY = [int]($Height * 0.20)
    $goldBrush = New-Object System.Drawing.SolidBrush($Gold)
    $g.FillEllipse($goldBrush, $coinX, $coinY, $coinSize, $coinSize)

    $ringPen = New-Object System.Drawing.Pen($GoldDark, [Math]::Max(3, $Height * 0.006))
    $ringInset = [int]($coinSize * 0.10)
    $g.DrawEllipse($ringPen, ($coinX + $ringInset), ($coinY + $ringInset), ($coinSize - 2 * $ringInset), ($coinSize - 2 * $ringInset))

    $fontCoin = New-Object System.Drawing.Font('Arial', [int]($coinSize * 0.42), [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
    $coinFmt = New-Object System.Drawing.StringFormat
    $coinFmt.Alignment = [System.Drawing.StringAlignment]::Center
    $coinFmt.LineAlignment = [System.Drawing.StringAlignment]::Center
    $navyBrush = New-Object System.Drawing.SolidBrush($Navy)
    $g.DrawString('C', $fontCoin, $navyBrush, (New-Object System.Drawing.RectangleF($coinX, $coinY, $coinSize, $coinSize)), $coinFmt)

    $fontBig = New-Object System.Drawing.Font('Arial', [int]($Height * 0.09), [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
    $fontSmall = New-Object System.Drawing.Font('Arial', [int]($Height * 0.030), [System.Drawing.FontStyle]::Regular, [System.Drawing.GraphicsUnit]::Pixel)
    $paperBrush = New-Object System.Drawing.SolidBrush($Paper)
    $dimBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 150, 170, 190))

    $fmt = New-Object System.Drawing.StringFormat
    $fmt.Alignment = [System.Drawing.StringAlignment]::Center
    $g.DrawString('COIN RUSH', $fontBig, $paperBrush, (New-Object System.Drawing.RectangleF(0, ($Height * 0.60), $Width, ($Height * 0.15))), $fmt)
    $g.DrawString('collect all 8 coins before the clock runs out', $fontSmall, $dimBrush, (New-Object System.Drawing.RectangleF(0, ($Height * 0.76), $Width, ($Height * 0.08))), $fmt)

    # 24-bit BMP, bottom-up, rows padded to 4 bytes
    $rowBytes = [int]([Math]::Floor(($Width * 3 + 3) / 4) * 4)
    $pixelBytes = $rowBytes * $Height
    $fs = New-Object System.IO.MemoryStream
    $bw = New-Object System.IO.BinaryWriter($fs)
    $bw.Write([System.Text.Encoding]::ASCII.GetBytes('BM'))
    $bw.Write([UInt32](54 + $pixelBytes))
    $bw.Write([UInt32]0); $bw.Write([UInt32]54)
    $bw.Write([UInt32]40); $bw.Write([Int32]$Width); $bw.Write([Int32]$Height)
    $bw.Write([UInt16]1); $bw.Write([UInt16]24)
    $bw.Write([UInt32]0); $bw.Write([UInt32]$pixelBytes)
    $bw.Write([Int32]2835); $bw.Write([Int32]2835); $bw.Write([UInt32]0); $bw.Write([UInt32]0)

    $pad = $rowBytes - $Width * 3
    for ($y = $Height - 1; $y -ge 0; $y--) {
        for ($x = 0; $x -lt $Width; $x++) {
            $c = $bmp.GetPixel($x, $y)
            $bw.Write([byte]$c.B); $bw.Write([byte]$c.G); $bw.Write([byte]$c.R)
        }
        for ($p = 0; $p -lt $pad; $p++) { $bw.Write([byte]0) }
    }
    $bw.Flush()
    $dir = Split-Path -Parent $Path
    if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
    [System.IO.File]::WriteAllBytes($Path, $fs.ToArray())
    $bw.Dispose(); $fs.Dispose(); $g.Dispose(); $bmp.Dispose()
    Write-Host ("wrote {0} ({1:N0} bytes)" -f $Path, (Get-Item $Path).Length)
}

# 256x256 ICO is what Windows picks for large icons; Explorer scales the rest.
$icon = New-CoinBitmap 256
Save-Ico (Join-Path $Root 'Build\Windows\Application.ico') $icon
$icon.Dispose()

Save-Splash (Join-Path $Root 'Content\Splash\Splash.bmp') 1920 1080
