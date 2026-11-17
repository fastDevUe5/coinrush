@echo off
rem 用 UE5 编辑器打开 CoinRush 工程（含已生成的 CoinRush 关卡）
rem -ModelContextProtocolStartServer 让编辑器启动内置 MCP 服务（127.0.0.1:8000/mcp）
setlocal
set "UE=C:\Program Files\Epic Games\UE_5.8"
set "PROJECT=%~dp0CoinRush.uproject"
start "" "%UE%\Engine\Binaries\Win64\UnrealEditor.exe" "%PROJECT%" -ModelContextProtocolStartServer
