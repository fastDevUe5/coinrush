# CoinRush —— 极简 3D 收集金币游戏（Unreal Engine 5.8 + C++）

在 75 秒内跑跳穿过竞技场，收集全部 8 枚漂浮金币。
玩法逻辑全部是 C++，关卡与材质由编辑器 Python 脚本从代码生成——没有蓝图、没有外部美术资源，整份工程可以从零复现。

![游戏画面](Docs/screenshot_gameplay.png)

## 快速开始

| 想做什么 | 双击 |
| --- | --- |
| 直接玩 | `Run_Game.bat` |
| 用编辑器打开工程 / 关卡 | `Open_Editor.bat` |
| 改完 C++ 后重新编译 + 重新生成关卡 | `Rebuild_All.bat` |

> 本机已装好 UE 5.8.4（`C:\Program Files\Epic Games\UE_5.8`）和 VS2026 C++ 工具链。
> 该引擎没有在 Epic 启动器里登记，所以直接双击 `.uproject` 可能提示找不到引擎；请用上面两个 `.bat` 启动。

## 操作

| 按键 | 动作 |
| --- | --- |
| W A S D / 方向键 | 移动（相对镜头方向） |
| 鼠标拖动 / Q E | 左右转镜头 |
| R F | 镜头俯仰 |
| Space | 跳跃 |
| 按住 Shift | 冲刺（FOV 会拉宽） |
| Enter | 重开一局 |
| Esc | 退出 |

右上角会显示 `SHIFT = SPRINT`，冲刺时变成高亮的 `SPRINT`。按 Enter 后关卡重载、计时和金币全部重置。

## 玩法规则

- 8 枚金币分布在地面、中央高台和两侧平台上，75 秒倒计时。
- **连击**：两次拾取间隔 ≤ 4 秒算连击，第 2 枚起每枚 **+1.5 秒**；HUD 显示 `STREAK x3 +1.5s`，结算页显示本局最佳连击。
  金币间距约 11 米（走路 2.4s / 冲刺 1.3s），所以连击需要提前规划路线而不是乱跑。
- 拾取全部金币 → 胜利画面 `ALL COINS COLLECTED!`
- 时间归零 → `TIME UP!`，显示本局收集数。
- 结束后按 Enter 立即再来一局。

![时间到结算画面](Docs/screenshot_timeup.png)

## 工程结构

```
CoinRush/
├─ CoinRush.uproject          工程描述（启用 PythonScriptPlugin / EditorScriptingUtilities）
├─ Config/
│  ├─ DefaultEngine.ini       默认地图、GameMode、渲染开关（关闭 Lumen / 虚拟阴影以加快启动）
│  └─ DefaultGame.ini         工程名与版本
├─ Source/
│  ├─ CoinRush.Target.cs          Game 目标
│  ├─ CoinRushEditor.Target.cs    Editor 目标
│  └─ CoinRush/
│     ├─ CoinRush.Build.cs
│     ├─ CoinRush.h / .cpp        模块入口
│     ├─ CRCharacter.h / .cpp     第三人称角色（弹簧臂相机 + 基础几何体拼装）
│     ├─ CRCoin.h / .cpp          金币：旋转、上下浮动、点光源、Overlap 拾取
│     ├─ CRGameMode.h / .cpp      回合流程：生成金币、计时、计分、胜负
│     └─ CRHUD.h / .cpp           Canvas HUD：分数、倒计时、操作提示、结算层
├─ Content/                   生成物（Maps/CoinRush.umap、Materials/*.uasset）
├─ Tools/
│  ├─ build_level.py          编辑器 Python：建材质 + 建关卡 + 灯光 + PlayerStart
│  ├─ BuildAll.ps1            编译 C++ → 运行 Python 生成关卡
│  ├─ RunGame.ps1             启动游戏、可脚本化按键、自动截图、抓日志
│  └─ inspect_level.py        打印关卡里所有 actor（调试用）
├─ Run_Game.bat / Open_Editor.bat / Rebuild_All.bat
└─ Docs/                      截图
```

## 实现要点

- **纯 C++ 玩法**：`ACRCharacter` 继承 `ACharacter`；相机用 `USpringArmComponent` + `UCameraComponent`，弹簧臂用绝对旋转跟随自定义的 yaw/pitch，这样角色转向时镜头不会跟着甩。角色外观由引擎自带的 Cylinder / Sphere / Cube 拼成（不需要骨骼网格）。
- **不依赖输入资产**：`Tick` 里轮询 `APlayerController::IsInputKeyDown`，鼠标增量用 `GetInputMouseDelta`。这样工程里一张 InputAction / InputMappingContext 都不需要，也避开了 Enhanced Input 与旧映射的兼容问题。
- **金币**：球体 + 点光源，`Tick` 里自转和正弦浮动；`OnComponentBeginOverlap` 只认 `ACRCharacter`，通知 GameMode 加分后销毁。
- **HUD**：继承 `AHUD` 直接往 `Canvas` 画文字和色块（没有 UMG），所以整个 UI 也是零资产。
- **关卡生成**：`Tools/build_level.py` 通过 `EditorScriptingUtilities` 创建 5 个材质（地板 / 平台 / 金币 / 角色 / 发光件）和 34 个 actor，然后 `save_current_level()`。脚本是幂等的：已存在关卡会先清空所有 actor 再重建。
- **调试开关**：控制台执行 `cr.DebugInfo 1`，HUD 左下角会显示角色与相机坐标，方便调相机和关卡。
- **本机适配**：`Rebuild_All.bat` 调 UBT 时带 `-NoHotReloadFromIDE`，否则编辑器开着 Live Coding 时 UBT 会拒绝编译。

## 已验证的内容

在本机（i5-14600KF / Win11 / UE 5.8.4 / VS2026）实际跑过：

1. `Build.bat` 编译 `CoinRushEditor` 目标，无错误（仅引擎自带的第三方头文件警告）。
2. `UnrealEditor-Cmd -run=pythonscript` 生成关卡与材质，日志输出 `Done. 34 actors`。
3. `UnrealEditor.exe -game` 独立运行：HUD、倒计时、走路、镜头转向正常。
4. `Tools/RunGame.ps1` 用 Windows 消息模拟按键：走到金币旁后计数器从 `0 / 8` 变成 `1 / 8`。
5. 按 Space 起跳：角色离地（调试 HUD 显示 Z 从 90 变为 162），影子与角色分离。
6. 空放 75 秒后出现 `TIME UP!` 结算层。
7. 结算/中途按 Enter：日志出现第二条 `Round started`，计时重置为 75 秒、金币恢复 8 枚。
8. 用 `UnrealEditor.exe CoinRush.uproject` 打开编辑器，自动加载 `CoinRush` 关卡，大纲里 34 个 actor 齐全。
9. MCP 接线（UE 5.8 官方 Unreal MCP + DSH `dsh-mcp-client`）：
   - `list_toolsets` 返回 21 个工具集；`GetVisibleActors` 读出当前关卡 actor；
   - `SetCameraTransform` 设置视口机位，`CaptureViewport` 取回视口画面；
   - `add_to_scene_from_asset` 生成一个测试方块 → 截图里可见 → `remove_from_scene` 删除还原；
   - `StartPIE` → `IsPIERunning=true` → `StopPIE`，能在编辑器里起停 PIE；
   - 重启编辑器（不带命令行参数）后服务按配置自动启动，会话失效后重新初始化即恢复。

![编辑器里打开工程](Docs/screenshot_editor.png)

> 截图和日志都在 `Saved/Screenshots`、`Saved/Logs` 下，由 `Tools/RunGame.ps1` 自动产生。

## 下一步可以怎么玩

- **换成 Lumen / 虚拟阴影贴图**：删掉 `Config/DefaultEngine.ini` 里 `r.DynamicGlobalIlluminationMethod=0`、`r.Shadow.Virtual.Enable=0` 两行，画面会更好，但首次启动要等更久的着色器编译。
- **打包成 exe**：编辑器里 `平台 → Windows → 打包项目`（本工程没有额外插件依赖，直接能出包）。
- **加内容**：在 `Tools/build_level.py` 里加方块就是加地形；金币位置在 `CRGameMode.cpp` 的 `GCoinSpots` 数组里改。
- **改造玩法**：`CRGameMode::FinishRound` 是胜负唯一的出口，想加计分、连击、多关卡都从这里扩展。

## 用 MCP 驱动编辑器（已接线并验证）

本工程已启用 UE 5.8 的官方实验性 MCP 插件，编辑器开着时由我（或任何 MCP 客户端）直接操作它。

- **启用的插件**（见 `CoinRush.uproject`）：`ModelContextProtocol`（Unreal MCP）、`ToolsetRegistry`、`EditorToolset`、`AutomationTestToolset`、`SlateInspectorToolset`。
- **服务地址**：`http://127.0.0.1:8000/mcp`（Streamable HTTP，仅本机）。
- **自动启动**：`Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini` 里
  `[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]` 的 `bAutoStartServer=True`；
  也可以启动时加 `-ModelContextProtocolStartServer`（`Open_Editor.bat` 已带，双保险）。
  端口可用 `-ModelContextProtocolPort=N` 覆盖。
- **Harness 侧连接**：配置型 bundle 在 `C:\Users\tk\Desktop\UE5\dsh-bundles\unreal-mcp`（包名 `@local/unreal-mcp`）。
  装好之后我会多出 `mcp__unreal__list_toolsets / describe_toolset / call_tool` 三个入口——
  因为插件开了 tool search，实际工具集（21 个）通过 `call_tool` 按需调用，不会一次性灌满上下文。
- **截图**：`EditorToolset.EditorAppToolset.CaptureViewport` 返回 base64 PNG；结果太大被落盘时，用
  `Tools/DecodeMcpCapture.ps1 -SpillPath <溢出文件> -OutFile view.png` 解出来看。
- **已知摩擦**：编辑器重启后旧的 MCP 会话 ID 会失效（服务端回 "client should reinitialize"），
  客户端不会自动重握手；把 bundle 禁用再启用（或重启 DSH 会话）即可恢复。
- **安全**：只监听 `127.0.0.1`，但它等于把"任意编辑器操作 + 脚本执行"交给代理，别改到公网监听。

## 版本控制与 VS Code

仓库根在 `C:\Users\tk\Desktop\UE5`（同时收进 `CoinRush` 和 `dsh-bundles`），已配置 **Git LFS**：

- `.gitattributes` 把 `.uasset/.umap/.ubulk/.uexp/.pak/.png/.wav/.fbx...` 全部交给 LFS，二进制资产不会撑爆仓库。
- `.gitignore`（根 + `CoinRush/`）忽略 `Binaries/ Intermediate/ Saved/ DerivedDataCache/ .vs/` 和 UBT 生成的 `compileCommands_*.json`。
- 首次提交：`ea7b957 Initial import: CoinRush UE5 project, MCP bridge bundle, tooling and docs`。
- 提交身份暂时是本机占位（`tk <tk@localhost>`），要改成自己的：
  ```
  git config user.name "你的名字"
  git config user.email "you@example.com"
  git commit --amend --reset-author --no-edit
  ```

**VS Code**：用 VS Code 打开 `CoinRush` **目录**（不是仓库根），`.vscode/` 已经配好：

| 任务（Ctrl+Shift+B 里的默认项） | 作用 |
| --- | --- |
| `UE: 编译 + 重生成关卡` | 调用 `Tools/BuildAll.ps1`，等价于 `Rebuild_All.bat` |
| `UE: 重生成关卡` | 只跑 Python 生成器 |
| `UE: 运行游戏` / `UE: 打开编辑器` | 两个 bat |
| `UE: 启动并截图` | 启动、等 30 秒、截图、关闭 |
| `UE: 刷新 VS Code IntelliSense` | 重新生成 `compileCommands_*.json`（新增/删除源文件后跑一次） |

- C++ IntelliSense 走 UBT 生成的 `compileCommands_CoinRush.json`，头文件路径和编译宏都由引擎给出，不用手工配 include。
- 调试：`launch.json` 里可以选择 `启动游戏 (-game)` 或在编辑器运行时 `附加到 UnrealEditor`。
- 扩展建议在 `.vscode/extensions.json`（C/C++、Python、PowerShell）。

> UBT 还会顺手生成一个 `CoinRush.code-workspace`（含引擎目录 + 全套 Debug/Development/Shipping 构建与调试配置，里面全是绝对路径）。它以会重新生成为代价，已被 `.gitignore` 排除。
> 想用那套完整配置就打开 `.code-workspace`；想用上面这 5 个精简任务就打开文件夹——**两者任务不会同时生效**，按需选一个。

## 自动化测试（已接入）

```powershell
powershell -ExecutionPolicy Bypass -File Tools\RunTests.ps1
```

- 覆盖**回合纯逻辑**：集齐全部金币判胜、计时归零判负、结束后再拾取/再计时被忽略。
- 逻辑被抽到 `Source/CoinRush/CRRoundModel.h`（不碰 UObject/World），所以测试不需要创建关卡，30 秒跑完。
- 测试代码在 `Source/CoinRush/Private/Tests/CRRoundModelTests.cpp`，过滤名 `CoinRush.RoundModel`。
- 输出：`Saved\Logs\tests.log`，每行 `Test Completed. Result={Success|Fail} Path={...}`。

改到计分/计时/胜负时必须跑一遍——这是目前唯一的回归网。

## 手感反馈（juice，已实现）

全部零资产、纯代码，改完只需 `Rebuild_All.bat`：

| 反馈 | 实现 | 调参位置 |
| --- | --- | --- |
| 金币拾取“弹一下” | 拾取后放大 2.1 倍并在 0.18s 内消失，不再瞬间销毁 | `CRCoin.h` 的 `PopDuration` |
| 拾取粒子 | 8 个小型发光球向外飞散 + 落地重力 + 一盏会衰减的点光 | `CRCoinBurst` 的 `NumShards` / `LifeSeconds` |
| 落地震屏 | 落地时相机下沉 + 轻微俯仰，0.22s 指数衰减 | `ACRCharacter::Landed` / `LandShakeDuration` |
| 计分跳动 | 金币计数变化时字号放大 1.55 倍并 0.35s 回弹 | `CRHUD.cpp` 的 `ScorePulse` |
| 连击提示 | 连击 ≥2 时显示 `STREAK xN +1.5s`，颜色随拾取脉冲由白转金 | `CRHUD.cpp` 的连击块 |
| 冲刺 FOV | 按住 Shift 时 FOV 从 88 拉到 99 | `CRCharacter.h` 的 `SprintFieldOfView` |

验证方式：`RunGame.ps1 -Sequence "E:0.8|W:0.75" -SettleMs 60`，日志出现
`Collect burst spawned` 且紧接 `Coin 1/8`，截图可见角色周围的发光碎片。

## 音效（程序生成，已接入）

没有任何外部素材——两个音效都是脚本合成的 16-bit PCM WAV：

| 环节 | 文件 |
| --- | --- |
| 合成 | `Tools/GenerateAudio.ps1`（金币：1568/2093/3136Hz 三个分音指数衰减；落地：72Hz 正弦 + 噪声） |
| 源文件 | `Tools/AudioSrc/SFX_Coin.wav`、`SFX_Land.wav` |
| 导入 | `Tools/import_audio.py` → `/Game/Audio/SFX_Coin`、`SFX_Land`（`BuildAll.ps1` 第 2 步自动跑） |
| 播放 | `ACRGameMode::PlayCoinSfx`（空间音源 + 随机音高 0.95~1.06）、`PlayLandSfx`（2D） |

调音色：改 `GenerateAudio.ps1` 里的频率与衰减系数，然后跑 `BuildAll.ps1`。
验证：日志出现 `SFX loaded: coin=1 land=1`，拾取时紧跟 `SFX coin`。

## 发布包（已生成）

```powershell
powershell -ExecutionPolicy Bypass -File Tools\MakeRelease.ps1 -Version v0.1.0
```

产物：`Saved\Release\CoinRush-v0.1.0-win64.zip`（**203 MB**，解压后 319 MB），内含：

```
Binaries\Win64\CoinRush-Win64-Shipping.exe   (158 MB, 含 CRT 与第三方 DLL)
Binaries\Win64\*.dll                        (VC++ 运行时 + tbb)
Content\Paks\*.pak / .ucas / .utoc          (cooked 内容)
Content\Splash\Splash.bmp
README.txt                                  (操作说明 + 依赖说明)
```

脚本会剔除 `Saved/`、`.pdb`、`d3d12SDKLayers.dll`，并在包内写一份英文 `README.txt`（操作、依赖、构建信息）。

**验证状态（诚实说明）**：

- zip 内文件与 `Saved\Package\Shipping\` 下的文件**逐字节一致**（SHA256 相同、文件数 22 对 22）；
- 原始目录 `Saved\Package\Shipping\Windows\CoinRush\` 里的 exe **实跑通过**（进程存活、窗口出现）；
- 但我把同一份文件**复制到别的路径**（或改名）后运行，会立即以退出码 1 结束、且不产生日志。
  这台机器上没查到崩溃事件、没有 MoTW 标记、Defender 也没报毒。**原因未定**（怀疑与启动方式或安全软件对“新复制的大体积 exe”的处理有关）。
  所以 zip 的“解压即可运行”还需要你手动双击确认一次——如果解压后双击也失败，我们再一起查。

## 品牌资源（图标 / 启动画面，已实测）

两个资源都是脚本画的，没有外部素材：

| 资源 | 路径 | 谁用它 |
| --- | --- | --- |
| 应用图标 | `Build\Windows\Application.ico`（256×256，32bpp） | UBT 自动读取（`UEBuildWindows.cs` 里的 `GetWindowsApplicationIcon`），嵌进 exe |
| 游戏启动画面 | `Content\Splash\Splash.bmp`（1920×1080，24bpp） | 打包时被 stage 进 `Content\Splash\`（`WinPlatform.Automation.cs`） |

重新生成：`powershell -File Tools\GenerateBranding.ps1`（改颜色/文字就改脚本里的常量）。

验证图标确实进了 exe：

```powershell
[System.Drawing.Icon]::ExtractAssociatedIcon("Saved\Package\Shipping\Windows\CoinRush\Binaries\Win64\CoinRush-Win64-Shipping.exe")
```

![打包 exe 提取出的图标](Saved/Screenshots/Packaged_Exe_Icon.png)

**已知限制**：exe 的 `FileVersion` / `ProductVersion` 目前显示的是引擎版本（`++UE5+Release-5.8-CL-...`），
`DefaultGame.ini` 里的 `ProjectVersion` 只用于 UWP/启动器显示，不会写进桌面 exe 的版本资源。
真要自定义需要额外的版本资源（`Build/Windows/Resources` 自定义 `.rc`），暂未做。

## 性能基线（2026-11-17 实测）

i5-14600KF / Windows 11 / 1280×720 / 开发构建（`-game`），测量方式：`RunGame.ps1 -ExecCmds "stat unit"` 截图读数。

| 配置 | Frame | Game | Draw | RHIT | Draws | Prims |
| --- | --- | --- | --- | --- | --- | --- |
| 引擎默认平滑（62fps 上限） | 16.13 ms | 1.53 | 1.94 | 1.38 | 82 | 36.2K |
| 关平滑、无上限 | 2.02 ms | 1.60 | 2.02 | 1.07 | 78 | 36.2K |
| **关平滑 + 120fps 上限（当前）** | **8.33 ms** | **1.57** | **1.85** | **1.28** | **78** | **36.2K** |

结论：

- 距离真实瓶颈很远——Game / Draw / RHI 都在 2ms 以内，帧时间完全由**帧率上限**决定，不是性能问题。
- 引擎默认的 `bSmoothFrameRate` 会把上限压到 62fps（16.13ms），在高刷屏上体感发涩；已关闭，并把上限设为 120fps（`CRGameMode::StartPlay`）。
- 等资产量、灯光、特效上来后才需要真正优化，顺序参考：Draw call → 材质复杂度 → 阴影 → 后处理。

![性能基线](Docs/screenshot_perf_baseline.png)

## 打包（已实测通过）

```powershell
powershell -ExecutionPolicy Bypass -File Tools\Package.ps1               # Development
powershell -ExecutionPolicy Bypass -File Tools\Package.ps1 -Config Shipping
```

两个配置都实测过：

| 配置 | 产物目录 | 整包 | 可执行文件 | 调试符号 |
| --- | --- | --- | --- | --- |
| Development | `Saved\Package\Development\` | 约 826 MB | 317 MB（含符号的 `CoinRush.exe`） | 有 |
| **Shipping** | `Saved\Package\Shipping\` | **319 MB** | **158 MB**（`CoinRush-Win64-Shipping.exe`） | **无** |

Shipping 版的注意点：

- 包里没有 `.pdb`——靠 UAT 的 `-nodebuginfo` 才能做到（见下面的坑）；
- Shipping 关闭了 UE 日志，所以只能用“进程存活 + 窗口出现”判断是否启动成功，看日志会以为什么都没发生；
- 想保留符号又不想进包，符号仍留在工程的 `Binaries\Win64\*.pdb` 里，可以单独归档。

产物：`Saved\Package\Windows\CoinRush\Binaries\Win64\CoinRush.exe`（约 317 MB，整包约 900 MB），日志里出现 `Engine is initialized` 即启动成功。

**为什么脚本要拆成两步**：编辑器开着 Live Coding 时，它会持有 `UnrealEditor.exe` 的互斥体，UBT 直接拒绝构建（`Unable to build while Live Coding is active`），而 `BuildCookRun` 不接受 `-NoHotReloadFromIDE`。所以脚本先用带该参数的 `Build.bat` 编译 Editor + Game 两个目标，再让 `BuildCookRun -skipbuild` 只做 cook/stage/pak/archive。

顺带验证过：MCP 插件虽然是 `NoRedist`，但它是 EditorOnly，不参与 staging，**不会阻断打包**。

**打包版必须验证两件事**（踩过坑，见 Gotchas 第 20 条）：

1. 运行时 `LoadObject` 的资产有没有进包——`/Game/Audio` 和 `/Game/Materials` 已写进 `DefaultGame.ini` 的 `DirectoriesToAlwaysCook`；
   打包后跑一次，日志应出现 `SFX loaded: coin=1 land=1`。
2. 也可以直接查 cook 清单：`%APPDATA%\Unreal Engine\AutomationTool\Logs\<Engine>\FinalCopyWin64_UFSFiles.txt` 里搜索 `M_Coin`、`SFX_Coin`。

**产物是否依赖本机的 UE / VS？** 不依赖，已实测：

- `dumpbin /dependents` 显示 exe 只引用系统 DLL 和包内 DLL，没有引擎路径；
- 启动打包版后检查进程模块：**128 个已加载 DLL 中，0 个来自 UE 安装目录或 VS 安装目录**；
- `VCRUNTIME140.dll` / `VCRUNTIME140_1.dll` / `MSVCP140.dll` 是从包内 `Binaries\Win64` 加载的（`Package.ps1` 会把 VC++ 运行时一起拷进去；
  UE 开发构建用动态 CRT，不捆的话干净机器会报“找不到 VCRUNTIME140.dll”）；
- 环境要求只剩：Windows 10/11 x64 + 支持 D3D12 的显卡驱动。
- 包里的 `CoinRush.pdb`（350 MB）是调试符号，对外发布时可以删掉。

## 给 AI agent 的约定

换任何客户端（Claude Code / Cursor / Codex / DSH）都先读这两份：

- [`AGENTS.md`](AGENTS.md)：环境事实、硬约束（Live Coding 互斥、MCP 重连、PS 脚本纯 ASCII、LFS）、命令清单、代码地图、完成定义。
- [`Docs/UE5-Gotchas.md`](Docs/UE5-Gotchas.md)：15 条实测踩坑记录，每条都带出处（源码路径 / 日志 / 退出码）。

原则：**规则进脚本，结论进文档，每次改完跑一遍检查**。新踩的坑追加到 Gotchas，并写明出处。

## 常见问题

**Q：双击 `.uproject` 提示 "找不到引擎"？**
本机引擎未在 Epic 启动器登记（`LauncherInstalled.dat` 是空的）。用 `Open_Editor.bat` / `Run_Game.bat`，它们直接调用 `UnrealEditor.exe`。

**Q：编译报 "Unable to build while Live Coding is active"？**
编辑器正开着且启用了 Live Coding。本工程的 `Tools/BuildAll.ps1` 已加 `-NoHotReloadFromIDE` 绕过；手工编译时也请带上这个参数，或先关掉编辑器。

**Q：第一次启动黑屏很久？**
首次运行需要编译着色器（DX12/SM5 管线），之后会走缓存。工程已关闭 Lumen 与虚拟阴影以减少编译量。

**Q：画面偏暗/偏亮？**
调 `Tools/build_level.py` 里 `build_lighting()` 的太阳强度（当前 12）与 `auto_exposure_bias`（当前 -1.4），然后跑 `Rebuild_All.bat` 重生成关卡。

**Q：换了机器 / 新克隆后 VS Code 里 C++ 报一堆红？**
跑一次任务 `UE: 刷新 VS Code IntelliSense`（或手工执行 UBT 的 `-projectfiles -VSCode`），生成 `.vscode/compileCommands_CoinRush.json` 即可。

**Q：仓库里怎么没有 `Binaries/`、`Saved/`？**
被 `.gitignore` 排除了。二进制产物每次本地编译生成；`Saved/Config` 里的 MCP 自动启动设置也因此不进仓库，但 `Open_Editor.bat` 已带 `-ModelContextProtocolStartServer`，所以换机器也能用。
