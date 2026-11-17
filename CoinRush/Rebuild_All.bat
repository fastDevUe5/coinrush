@echo off
rem 重新编译 C++ 并用 Python 重新生成 /Game/Maps/CoinRush 关卡
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Tools\BuildAll.ps1"
pause
