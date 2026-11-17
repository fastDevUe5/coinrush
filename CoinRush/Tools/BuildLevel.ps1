#requires -Version 5.1
# Regenerates /Game/Maps/CoinRush (materials + arena) without recompiling C++.
$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
$UE = 'C:\Program Files\Epic Games\UE_5.8'
$Project = Join-Path $Root 'CoinRush.uproject'
$Cmd = Join-Path $UE 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Py = Join-Path $Root 'Tools\build_level.py'

& $Cmd "$Project" -run=pythonscript -script="$Py" -unattended -nosplash -nop4 -stdout -FullStdOutLogOutput 2>&1 |
    Select-String -Pattern 'LogPython: \[CoinRush\]|LogPython: Error|Exception' |
    ForEach-Object { $_.Line }

exit $LASTEXITCODE
