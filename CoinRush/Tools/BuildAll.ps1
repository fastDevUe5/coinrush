#requires -Version 5.1
# Builds the CoinRush C++ module and regenerates the arena level from Python.
$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
$UE = 'C:\Program Files\Epic Games\UE_5.8'
$Project = Join-Path $Root 'CoinRush.uproject'
$LogDir = Join-Path $Root 'Saved\Logs'
New-Item -ItemType Directory -Force -Path $LogDir | Out-Null

Write-Host "== 1/2  Building CoinRushEditor =="
$buildLog = Join-Path $LogDir 'build.log'
$buildScript = Join-Path $UE 'Engine\Build\BatchFiles\Build.bat'
& $buildScript CoinRushEditor Win64 Development "-Project=$Project" -WaitMutex -NoHotReloadFromIDE *>&1 | Out-File -FilePath $buildLog -Encoding utf8
$buildExit = $LASTEXITCODE
Select-String -Path $buildLog -Pattern 'error|warning C|Result:|Output binary' | Select-Object -Last 25 | ForEach-Object { $_.Line }
Write-Host "build exit code: $buildExit"
if ($buildExit -ne 0) {
    Write-Host "BUILD FAILED - see $buildLog"
    exit $buildExit
}

Write-Host "== 2/2  Generating /Game/Maps/CoinRush with Python =="
$levelLog = Join-Path $LogDir 'levelgen.log'
$cmd = Join-Path $UE 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$py = Join-Path $Root 'Tools\build_level.py'
& $cmd "$Project" -run=pythonscript -script="$py" -unattended -nosplash -nop4 -stdout -FullStdOutLogOutput *>&1 | Out-File -FilePath $levelLog -Encoding utf8
$levelExit = $LASTEXITCODE
Select-String -Path $levelLog -Pattern 'LogPython: \[CoinRush\]|LogPython: Error|Exception|Error:' | Select-Object -Last 25 | ForEach-Object { $_.Line }
Write-Host "levelgen exit code: $levelExit"
Write-Host "levelgen exit code: $levelExit"

Write-Host "== summary =="
Select-String -Path $levelLog -Pattern '\[CoinRush\]' | ForEach-Object { $_.Line }
if ($levelExit -ne 0) { exit $levelExit }
