#requires -Version 5.1
# 打包 CoinRush 为 Windows 可运行版本。
#
# 为什么不用一条 BuildCookRun 搞定：
#   Editor 的 Live Coding 会持有 UnrealEditor.exe 的互斥体，UBT 因此拒绝构建
#   ("Unable to build while Live Coding is active")。BuildCookRun 不接受
#   -NoHotReloadFromIDE，所以这里先自己编译（带该参数），再让 BuildCookRun
#   -skipbuild 只做 cook/stage/pak/archive。
#
# 用法： powershell -ExecutionPolicy Bypass -File Tools\Package.ps1 [-Config Development]

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

Write-Host "== 1/2  编译 CoinRush（Editor + Game, $Config）=="
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
Write-Host "打包完成：$exe"
if (Test-Path $exe) {
    Write-Host ("可执行文件大小：{0:N1} MB" -f ((Get-Item $exe).Length / 1MB))
}
