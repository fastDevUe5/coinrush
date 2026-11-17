#requires -Version 5.1
# Packages CoinRush into a runnable Windows build.
#
# Why two steps instead of one BuildCookRun:
#   The editor's Live Coding holds a mutex on UnrealEditor.exe, so UBT refuses to
#   build ("Unable to build while Live Coding is active"). BuildCookRun does not
#   accept -NoHotReloadFromIDE, so we build both targets ourselves first and let
#   BuildCookRun -skipbuild only cook/stage/pak/archive.
#
# Usage:  powershell -ExecutionPolicy Bypass -File Tools\Package.ps1 [-Config Development]
# NOTE: keep this file ASCII-only (PowerShell 5.1 reads BOM-less files as ANSI).

param(
    [string]$Config = 'Development'
)

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$UE = 'C:\Program Files\Epic Games\UE_5.8'
$Project = Join-Path $Root 'CoinRush.uproject'
$Archive = Join-Path $Root 'Saved\Package'
$LogDir = Join-Path $Root 'Saved\Logs'
New-Item -ItemType Directory -Force -Path $LogDir | Out-Null

Write-Host "== 1/2  Building CoinRushEditor and CoinRush ($Config) =="
& (Join-Path $UE 'Engine\Build\BatchFiles\Build.bat') CoinRushEditor Win64 $Config "-Project=$Project" -WaitMutex -NoHotReloadFromIDE |
    Tee-Object -FilePath (Join-Path $LogDir 'package_build_editor.log') | Out-Null
if ($LASTEXITCODE -ne 0) { throw "Editor target build failed ($LASTEXITCODE)" }

& (Join-Path $UE 'Engine\Build\BatchFiles\Build.bat') CoinRush Win64 $Config "-Project=$Project" -WaitMutex -NoHotReloadFromIDE |
    Tee-Object -FilePath (Join-Path $LogDir 'package_build_game.log') | Out-Null
if ($LASTEXITCODE -ne 0) { throw "Game target build failed ($LASTEXITCODE)" }

Write-Host "== 2/2  Cook / Stage / Pak / Archive =="
& (Join-Path $UE 'Engine\Build\BatchFiles\RunUAT.bat') BuildCookRun `
    "-project=$Project" -noP4 -platform=Win64 "-clientconfig=$Config" `
    -skipbuild -cook -stage -pak -archive "-archivedirectory=$Archive" -utf8output |
    Tee-Object -FilePath (Join-Path $LogDir 'package_cook.log') | Out-Null
if ($LASTEXITCODE -ne 0) { throw "BuildCookRun failed ($LASTEXITCODE)" }

$exe = Join-Path $Archive "Windows\CoinRush\Binaries\Win64\CoinRush.exe"
Write-Host "Packaged: $exe"
if (Test-Path $exe) {
    Write-Host ("Executable size: {0:N1} MB" -f ((Get-Item $exe).Length / 1MB))
}
