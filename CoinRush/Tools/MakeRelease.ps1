#requires -Version 5.1
# Builds a distributable zip from the Shipping package.
#   Output: Saved\Release\CoinRush-<Version>-win64.zip
# Usage:  powershell -ExecutionPolicy Bypass -File Tools\MakeRelease.ps1 [-Version v0.1.0]
# NOTE: keep this file ASCII-only (PowerShell 5.1 reads BOM-less files as ANSI).

param(
    [string]$Version = 'v0.1.0'
)

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$Package = Join-Path $Root 'Saved\Package\Shipping\Windows\CoinRush'
$ReleaseRoot = Join-Path $Root 'Saved\Release'
$Stage = Join-Path $ReleaseRoot ("CoinRush-" + $Version)
$Zip = Join-Path $ReleaseRoot ("CoinRush-" + $Version + "-win64.zip")

if (-not (Test-Path $Package)) {
    throw "Shipping package not found: $Package  (run Tools\Package.ps1 -Config Shipping first)"
}

if (Test-Path $Stage) { Remove-Item $Stage -Recurse -Force }
New-Item -ItemType Directory -Force -Path $Stage | Out-Null

Write-Host "== Staging $Version =="
Copy-Item (Join-Path $Package '*') -Destination $Stage -Recurse -Force

# Runtime-only: drop logs/config and any stray debug artifacts.
foreach ($junk in @('Saved', 'CoinRush.pdb', 'CoinRush-Win64-Shipping.pdb', 'd3d12SDKLayers.dll')) {
    $path = Join-Path $Stage $junk
    if (Test-Path $path) { Remove-Item $path -Recurse -Force }
}

$readme = @"
CoinRush $Version (Windows x64)

HOW TO PLAY
  Unpack the zip, run Binaries\Win64\CoinRush-Win64-Shipping.exe
  (the CoinRush.exe launcher also works if present)

  WASD / arrow keys   move
  Mouse, Q / E        turn camera
  R / F               camera pitch
  Space               jump
  Shift (hold)        sprint
  Enter               restart
  Esc                 quit

GOAL
  Collect all 8 coins before the clock runs out.
  Pick up coins within 4 seconds of each other to build a streak:
  every chained coin adds 1.5 seconds.

REQUIREMENTS
  Windows 10/11 x64, a GPU with a recent driver (D3D12).
  No Unreal Engine or Visual Studio installation is required;
  the MSVC runtime is bundled next to the executable.

BUILD INFO
  Unreal Engine 5.8.4, Shipping configuration.
  Source and tools: see the project repository.
"@
Set-Content -Path (Join-Path $Stage 'README.txt') -Value $readme -Encoding ASCII

Write-Host "== Zipping =="
if (Test-Path $Zip) { Remove-Item $Zip -Force }
Compress-Archive -Path (Join-Path $Stage '*') -DestinationPath $Zip -CompressionLevel Optimal

$zipMb = (Get-Item $Zip).Length / 1MB
$stageMb = (Get-ChildItem $Stage -Recurse -File | Measure-Object Length -Sum).Sum / 1MB
Write-Host ("Release: {0}" -f $Zip)
Write-Host ("  uncompressed {0:N0} MB -> zip {1:N0} MB" -f $stageMb, $zipMb)
