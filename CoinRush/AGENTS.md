# AGENTS.md — CoinRush 协作约定

任何 AI agent（Claude Code / Cursor / Codex / DSH）进入本工程先读这一份。**约定优先于猜测**：
这里写的路径、命令、禁忌都是实测过的，不要凭记忆改写。

## 0. 环境事实

| 项 | 值 |
| --- | --- |
| 引擎 | Unreal Engine **5.8.4**，`C:\Program Files\Epic Games\UE_5.8`（**未**在 Epic 启动器登记） |
| 工程 | `C:\Users\tk\Desktop\UE5\CoinRush\CoinRush.uproject`（仓库根在上一级） |
| 工具链 | VS2026 Community（MSVC 14.52），dotnet 10（引擎自带） |
| MCP | 编辑器内置 Unreal MCP：`http://127.0.0.1:8000/mcp`，仅本机 |

## 1. 硬约束（违反会浪费时间或弄坏工程）

0. **先查官方文档，再动手**。遇到“某个设置在哪、某个功能怎么开、某个报错怎么解”时，按下面的顺序找答案：
   1. `dev.epicgames.com/documentation` 的 **5.8 版本页**（页面默认展示最新版 5.9，先切版本选择器）；
   2. 本机 `Engine/Source`（文档没写或与版本不符时，以源码为准，行号记进 Gotchas）；
   3. 本工程实测（跑一次、看日志、截一次图）。
   常见入口：[打包项目](https://dev.epicgames.com/documentation/en-us/unreal-engine/packaging-unreal-engine-projects)、[编译配置参考](https://dev.epicgames.com/documentation/en-us/unreal-engine/build-configurations-reference-for-unreal-engine)、[Unreal MCP](https://dev.epicgames.com/documentation/unreal-engine/unreal-mcp-in-unreal-editor?lang=zh-CN)。

1. **不要双击 `.uproject`**——引擎没登记，会提示找不到引擎。用 `Open_Editor.bat` / `Run_Game.bat`。
2. **编译前确认编辑器已关闭**。编辑器开着 Live Coding 时会持有 `UnrealEditor.exe` 互斥体，UBT 会拒绝构建
   （`Unable to build while Live Coding is active`）。所有脚本已带 `-NoHotReloadFromIDE`，但**打包必须走 `Tools\Package.ps1`**，
   因为 `BuildCookRun` 不接受该参数。
3. **同一时间只允许一个 agent 操作编辑器**。MCP 会话、资产锁、Live Coding 都会冲突。
4. **编辑器重启后 MCP 会话失效**（服务端回 `Unknown session id ... client should reinitialize`，客户端不会自动重握手）。
   恢复办法：禁用再启用 DSH 的 `@local/unreal-mcp` bundle，或重启 agent 会话。
5. **`.ps1` 一律纯 ASCII**。PowerShell 5.1 会把无 BOM 的 UTF-8 当 ANSI 读，中文字面量会导致语法错误。
6. **二进制资产必须走 LFS**（`.gitattributes` 已配）。新增资产类型时先补 LFS 规则再提交。
7. **不要提交** `Binaries/ Intermediate/ Saved/ DerivedDataCache/ .vs/` 和 UBT 生成的 `compileCommands_*/*.rsp`、`*.code-workspace`。
8. **改完 C++ 必须跑一次 `Tools\BuildAll.ps1`** 并确认关卡仍能生成（Python 生成器依赖引擎 API，会用异常告诉你哪里变了）。

## 2. 命令清单（照抄即可）

```powershell
# 编译 Editor 目标 + 重新生成关卡（改 C++ 后的标准动作）
powershell -ExecutionPolicy Bypass -File Tools\BuildAll.ps1

# 只重新生成关卡（只改了 Tools\build_level.py）
powershell -ExecutionPolicy Bypass -File Tools\BuildLevel.ps1

# 启动游戏（独立窗口 1280x720）
Run_Game.bat

# 启动 → 等 N 秒 → 截游戏窗口 → 关闭（自动化验证用）
powershell -ExecutionPolicy Bypass -File Tools\RunGame.ps1 -Seconds 30

# 模拟按键走一段再截图（验证玩法）
powershell -ExecutionPolicy Bypass -File Tools\RunGame.ps1 -Seconds 26 -Sequence "E:0.8|W:1.0"

# 打包 Windows 版本 → Saved\Package\Windows\CoinRush\Binaries\Win64\CoinRush.exe
powershell -ExecutionPolicy Bypass -File Tools\Package.ps1

# 资产检查（pre-commit 钩子会自动跑）
powershell -ExecutionPolicy Bypass -File Tools\CheckAssets.ps1

# 跑自动化测试（纯逻辑，约 30 秒，改到回合逻辑必跑）
powershell -ExecutionPolicy Bypass -File Tools\RunTests.ps1

# 重新合成并导入音效（改 Tools\GenerateAudio.ps1 后跑；BuildAll 也会跑）
powershell -ExecutionPolicy Bypass -File Tools\GenerateAudio.ps1

# 打印关卡里所有 actor（确认生成结果）
"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "CoinRush.uproject" -run=pythonscript -script="Tools\inspect_level.py" -unattended -nosplash -nop4 -stdout

# 解码 MCP 的 CaptureViewport 结果（大结果会落盘）
powershell -ExecutionPolicy Bypass -File Tools\DecodeMcpCapture.ps1 -SpillPath <溢出文件> -OutFile view.png
```

## 3. 代码地图

| 文件 | 职责 | 常改的点 |
| --- | --- | --- |
| `Source/CoinRush/CRCharacter.*` | 第三人称角色：弹簧臂相机、轮询输入、冲刺/跳跃 | 相机距离/俯仰、速度、跳跃高度 |
| `Source/CoinRush/CRCoin.*` | 金币：自转、浮动、点光源、Overlap 拾取 | 半径、发光强度、浮动幅度 |
| `Source/CoinRush/CRGameMode.*` | 回合流程：生成金币、计时、计分、胜负 | `GCoinSpots`（金币坐标）、`RoundSeconds`、`FinishRound` |
| `Source/CoinRush/CRHUD.*` | Canvas HUD（无 UMG） | 文案、布局、`cr.DebugInfo` 调试行 |
| `Tools/build_level.py` | 关卡 + 材质生成（幂等重建） | `build_arena` / `build_props` / `build_lighting` / `make_materials` |
| `Config/DefaultEngine.ini` | 默认地图、GameMode、渲染开关 | 关 Lumen/VSM 的两行不要随手删 |

调试开关：控制台 `cr.DebugInfo 1` 会在 HUD 左下角显示角色/相机坐标。

## 4. 完成定义（Definition of Done）

一个改动算完成，必须同时满足：

1. `BuildAll.ps1` 退出码 0；
2. 日志出现 `[CoinRush] Done. 34 actors`（或明确的预期 actor 数）；
3. `Run_Game.bat` 或 `RunGame.ps1` 能启动并截到非黑画面；
4. 涉及玩法的改动，用 `-Sequence` 模拟按键验证一次（例：走到金币旁应看到 `COINS 1 / 8`）；
5. `CheckAssets.ps1` 通过（提交时自动跑）；
6. **改到回合逻辑（计分/计时/胜负/连击）必须跑 `RunTests.ps1` 且 4 个测试全绿**；
7. 提交信息说清“改了什么 + 验证了什么”。

## 5. 已知坑

全部记录在 [`Docs/UE5-Gotchas.md`](Docs/UE5-Gotchas.md)：API 记错、属性名变更、编码陷阱、打包互斥、MCP 会话、截图挂起等。
**遇到新坑请追加到那里，并写明出处（源码文件/日志/退出码）**——这份清单是本工程最值钱的资产。

系统性的知识（机制、取舍、可复制片段）放在 [`Docs/UE5-Knowledge/`](Docs/UE5-Knowledge/README.md)，
按“引擎机制 / 编辑器自动化 / 构建发布 / 渲染性能”四类组织；**结论必须标引擎版本与出处**，冲突时以源码和实测为准。

已就绪的三篇（P0）：

- [`gameplay-framework-lifecycle.md`](Docs/UE5-Knowledge/gameplay-framework-lifecycle.md) —— Pawn/HUD 在 BeginPlay 之前就生成（5.8.4 源码核对）
- [`editor-python-boundaries.md`](Docs/UE5-Knowledge/editor-python-boundaries.md) —— Python 能做什么、不能做什么
- [`unreal-mcp-practice.md`](Docs/UE5-Knowledge/unreal-mcp-practice.md) —— MCP 启用、调用模型、会话失效与恢复

## 6. 不要做的事

- 不要为了"更省事"改 `Config/DefaultEngine.ini` 里的 `r.DynamicGlobalIlluminationMethod=0` / `r.Shadow.Virtual.Enable=0` 而不测启动时间；
- 不要把玩法逻辑写进蓝图（本工程的设计前提是**零蓝图依赖**，全部 C++ + Python）；
- 不要用 `-log` 启动游戏后靠截屏判断（日志窗口可能挡住画面），用 `RunGame.ps1`；
- 不要在构建产物目录里手工改文件（`Binaries/`、`Intermediate/` 下次编译就没了）。
