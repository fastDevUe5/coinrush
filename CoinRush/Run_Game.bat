@echo off
rem 启动 CoinRush（独立游戏窗口，1280x720 窗口化）
setlocal
set "UE=C:\Program Files\Epic Games\UE_5.8"
set "PROJECT=%~dp0CoinRush.uproject"
if not exist "%PROJECT%" (
    echo CoinRush.uproject not found.
    pause
    exit /b 1
)
start "" "%UE%\Engine\Binaries\Win64\UnrealEditor.exe" "%PROJECT%" -game -windowed -resx=1280 -resy=720
