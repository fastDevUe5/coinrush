# UE5 / CoinRush 踩坑清单

每条都来自本机实测，写明**症状 → 原因 → 正确做法 → 出处**。新增条目请保持同样格式，出处写源码路径、日志片段或退出码。

引擎：UE 5.8.4（`C:\Program Files\Epic Games\UE_5.8`）｜工具链：VS2026 / MSVC 14.52

---

## 1. `GetInputMouseDelta` 不在 `UPlayerInput` 上

- **症状**：`error C2039: "GetInputMouseDelta": 不是 "UPlayerInput" 的成员`
- **原因**：该函数属于 `APlayerController`，不是输入对象。
- **正确**：`PC->GetInputMouseDelta(dx, dy);`
- **出处**：`Source/CoinRush/CRCharacter.cpp` 编译日志；声明在 `Engine/Source/Runtime/Engine/Classes/GameFramework/PlayerController.h`

## 2. Python 不能给材质表达式输入赋值

- **症状**：`MaterialExpressionMultiply: Property 'A' for attribute 'a' ... is protected and cannot be set`
- **原因**：`FExpressionInput` 成员是 protected，`set_editor_property` 只对公开属性有效。
- **正确**：
  ```python
  unreal.MaterialEditingLibrary.connect_material_expressions(base, "", multiply, "A")
  unreal.MaterialEditingLibrary.connect_material_expressions(strength, "", multiply, "B")
  unreal.MaterialEditingLibrary.connect_material_property(multiply, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
  ```
  常量表达式本身仍可用 `set_editor_property("constant", unreal.LinearColor(r,g,b,1))`。
- **出处**：`Tools/build_level.py` 的 `make_material()`；异常发生在 PythonScriptCommandlet

## 3. 雾的组件属性叫 `enable_volumetric_fog`

- **症状**：`ExponentialHeightFogComponent: Failed to find property 'volumetric_fog'`
- **正确**：`set_editor_property("enable_volumetric_fog", False)`；同类还有 `fog_density`、`fog_height_falloff`。
- **教训**：Python 属性名是 UPROPERTY 去前缀后的名字，别按 C++ 变量名猜。
- **出处**：`Tools/build_level.py` 的 `build_lighting()`；属性声明在 `Engine/Source/Runtime/Engine/Classes/Components/ExponentialHeightFogComponent.h`

## 4. 编辑器开着时 UBT 拒绝编译

- **症状**：`Unable to build while Live Coding is active. Exit the editor and game...`（`Result: Failed (OtherCompilationError)`，退出码 6）
- **原因**：UBT 检测到目标可执行文件的 Live Coding 互斥体；由 `BuildConfiguration.bAllowHotReloadFromIDE` 控制。
- **正确**：构建命令加 `-NoHotReloadFromIDE`。**注意 `-NoHotReload` 无效**（它设的是 `HotReloadMode`，不解除该检查）。
- **出处**：`System/HotReload.cs:270-278`、`Configuration/BuildConfiguration.cs:160`；`Tools/BuildAll.ps1` 已带该参数

## 5. 打包必须先编译、再 `-skipbuild`

- **症状**：`RunUAT BuildCookRun ... -build` 直接失败（退出码 6），日志停在 Live Coding 那句。
- **原因**：`BuildCookRun` 不接收 `-NoHotReloadFromIDE`，它自己调 UBT 时又撞上同一个互斥。
- **正确**：两步走（`Tools/Package.ps1` 已封装）
  1. `Build.bat CoinRushEditor Win64 Development ... -NoHotReloadFromIDE`
  2. `Build.bat CoinRush Win64 Development ... -NoHotReloadFromIDE`
  3. `RunUAT BuildCookRun ... -skipbuild -cook -stage -pak -archive`
- **验证**：实测成功——497 个包、整包约 900 MB、`CoinRush.exe` 317 MB，启动日志到 `Engine is initialized`。
- **顺带结论**：MCP 插件虽标 `NoRedist`，但它是 EditorOnly，不参与 staging，不影响打包。

## 6. PowerShell 5.1 会把无 BOM 的 UTF-8 当 ANSI 读

- **症状**：`.ps1` 里的中文让解析器报 `The string is missing the terminator`，明明引号是成对的。
- **原因**：PS 5.1 默认按系统代码页（GBK）读无 BOM 文件，多字节字符被截断。
- **正确**：**所有 `.ps1` 保持纯 ASCII**（注释也写英文）。需要中文输出时用 `[System.IO.File]::ReadAllText($p, [Text.Encoding]::UTF8)` 读文件，或改用 `pwsh` 7。
- **出处**：`Tools/CheckAssets.ps1` 第一版；`Tools/*.ps1` 现已全部 ASCII（可用脚本检查非 ASCII 字符数）

## 7. `PrintWindow` 抓 DX12 窗口会挂起

- **症状**：对打包版游戏窗口调用 `PrintWindow(hwnd, hdc, 2)` 长时间不返回，脚本卡死。
- **正确**：别用它抓实时 3D 窗口；用 `Tools/RunGame.ps1` 那条路（启动游戏 → `CopyFromScreen` 抓客户区 → 关闭）。
- **出处**：`Tools/CaptureWindow.ps1`（保留作参考，但对 DX12 不可靠）

## 8. 编辑器重启后 MCP 会话失效

- **症状**：`Unknown session id 'xxxx' for 'tools/call'; client should reinitialize`
- **原因**：UE 的 MCP 服务每个编辑器进程一套会话；客户端保存的 session id 在新进程里无效，且不会自动重握手。
- **正确**：禁用再启用 DSH 的 `@local/unreal-mcp` bundle（强制重连），或重启 agent 会话。客户端配置里的 `reconnect` 只处理传输层断连，解决不了这个。
- **出处**：`ModelContextProtocolServer.cpp` 的会话校验；实测两次

## 9. MCP 服务默认不随编辑器启动

- **症状**：插件启用了，但 8000 端口不监听，日志没有 `Starting MCP server`。
- **原因**：`UModelContextProtocolSettings::bAutoStartServer` 默认 `false`（`config=EditorPerProjectUserSettings`）。
- **正确**：`-ModelContextProtocolStartServer` 启动，或在 `Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini` 写
  `[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings] bAutoStartServer=True`（本工程两者都做了）。
- **出处**：`ModelContextProtocolSettings.h:42`、`ModelContextProtocolSettings.cpp:32-61`

## 10. `CaptureViewport` 必须显式传 `captureTransform`

- **症状**：`Function "CaptureViewport", input param "captureTransform" needs a default value.`
- **正确**：显式传相机位姿（`location` + `rotation`），即使你刚用 `SetCameraTransform` 设过。
- **补充**：返回的 base64 PNG 通常 300~500 KB，会被 harness 落盘；用 `Tools/DecodeMcpCapture.ps1` 解码。

## 11. `new_level` 对已存在地图会失败

- **症状**：`LevelEditorSubsystem: Error: NewLevel. Failed to validate the destination. An asset already exists at this location.`
- **正确**：改成幂等重建——`load_level()` → 遍历 `get_all_level_actors()` 并 `destroy_actor()` → 重新生成 → `save_current_level()`。只有地图不存在时才 `new_level`。
- **出处**：`Tools/build_level.py` 的 `ensure_empty_level()`

## 12. 双击 `.uproject` 提示找不到引擎

- **原因**：`C:\ProgramData\Epic\UnrealEngineLauncher\LauncherInstalled.dat` 的 `InstallationList` 为空，引擎未登记。
- **正确**：用 `Open_Editor.bat` / `Run_Game.bat`（直接调 `UnrealEditor.exe`），或在注册表 `HKCU\Software\Epic Games\Unreal Engine\Builds` 登记。

## 13. PowerShell 的 `$PID` 是只读自动变量

- **症状**：枚举窗口时 `无法覆盖变量 PID，因为该变量为只读`。
- **正确**：用别的名字，例如 `$procId`。
- **出处**：`Tools/RunGame.ps1` 的 C# 互操作调用

## 14. VS Code IntelliSense 需要 UBT 生成的编译数据库

- **症状**：新克隆的工程在 VS Code 里 C++ 全红。
- **正确**：跑 `UnrealBuildTool -projectfiles -project=<uproject> -game -VSCode`（VS Code 任务“UE: 刷新 VS Code IntelliSense”已封装），生成 `.vscode/compileCommands_CoinRush.json`。
- **注意**：`compileCommands_*/` 目录里的 `.rsp` 和 `*.code-workspace` 含绝对路径，已 gitignore，别提交。

## 15. 单机编译器版本偏新

- **症状**：编译日志告警 `Visual Studio compiler version 14.52.37029 is newer than latest preferred version 14.50.35717`。
- **处理**：目前只影响告警，产物正常；如遇到无从解释的编译/链接错误，优先怀疑工具链版本，换 VS 2022 17.x 的 MSVC 14.38~14.44 复核。

## 16. 官方文档默认展示最新版本（5.9），不是本机的 5.8

- **症状**：照着 `dev.epicgames.com` 的页面配 MCP，结果 5.8 上服务不自启、工具里找不到 `describe_tools`。
- **原因**：文档页顶部/侧栏有版本选择器，默认给最新版；5.8 与 5.9 在 MCP 上有实质差异（自动启动默认值、元工具数量、助手面板、Origin 校验）。
- **正确**：查文档先确认版本；对 API 细节以 `Engine/Source` 为准。差异清单见
  [`Docs/UE5-Knowledge/unreal-mcp-practice.md`](UE5-Knowledge/unreal-mcp-practice.md) 的"官方文档（当前是 5.9 版）与本机 5.8.4 的差异"。
- **出处**：<https://dev.epicgames.com/documentation/unreal-engine/unreal-mcp-in-unreal-editor?lang=zh-CN>（页面标题为 5.9 文档）vs 本机 `ModelContextProtocolSettings.h:42-44`

## 17. `-game` 启动时 Python/ToolsetRegistry 报 AttributeError

- **症状**：游戏日志开头出现
  `module 'unreal' has no attribute 'PythonTestRunner'`、`has no attribute 'AgentSkill'`，来自
  `Engine/Plugins/Experimental/ToolsetRegistry/Content/Python/init_unreal.py`。
- **原因**：这些是编辑器插件的 Python 初始化脚本，在 `-game` 模式下运行时缺少编辑器 Python 绑定；属于噪音。
- **处理**：忽略即可，不影响游戏；如果嫌吵，可以在打包版里不启用这些插件（编辑器专用，不进包）。
- **出处**：`Saved\Logs\CoinRush.log` 启动段，2026-11-17 实测

## 18. `[SystemSettings]` 里的 `t.MaxFPS` 在工程 DefaultEngine.ini 不生效

- **症状**：在 `Config/DefaultEngine.ini` 写了
  ```
  [SystemSettings]
  t.MaxFPS=120
  ```
  启动后 `stat unit` 仍是 `Frame: 2.02 ms`（约 485fps，完全没被限制）；而同一文件 `[/Script/Engine.Engine] bSmoothFrameRate=False` 是生效的。
- **对照**：命令行 `-ExecCmds="t.MaxFPS 120, stat unit"` 立刻变成 `Frame: 8.33 ms`，说明 cvar 本身没问题。
- **做法**：帧率上限写到代码里（`ACRGameMode::StartPlay` 用 `IConsoleManager` 设），或用命令行；不要指望工程 ini 的 `[SystemSettings]`。
- **出处**：`Config/DefaultEngine.ini`、`Source/CoinRush/CRGameMode.cpp`、2026-11-17 三次对比实测

## 19. 运行时合成音频在 5.8 上不可靠，改用“生成 WAV + 导入”

- **症状**：想用 `USoundWaveProcedural` + `QueueAudio` 做零资产音效，但 5.8 里 `USoundWave` 的 `Duration`、`NumChannels`、`SampleRate` 已经私有化/改名，`GetDuration()` 是虚函数，一次性音效的时长语义不确定——不播、或播完不收尾都可能，且本地无法用耳朵验证。
- **正确**：用脚本生成标准 16-bit PCM WAV（`Tools/GenerateAudio.ps1`），再用 `AssetImportTask` 导入成真正的 `USoundWave` 资产（`Tools/import_audio.py`），C++ 里 `LoadObject` + `PlaySoundAtLocation`。这条路只用到稳定 API，音色可听、可调、可版本控制。
- **出处**：本工程 2026-11-17 实测；`Content/Audio/SFX_Coin.uasset`、`SFX_Land.uasset`

## 20. 运行时 `LoadObject` 的资产不会自动进包（打包后材质变灰、音效没声）

- **症状**：打包版能跑，但日志出现 `[CoinRush] SFX loaded: coin=0 land=0`；同类问题表现为金币/角色材质变成默认灰。
- **原因**：cooker 只收集**静态可见的引用**（关卡里的 Actor、CDO 上的 ConstructorHelpers/硬引用、软引用清单）。代码里
  `LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SFX_Coin.SFX_Coin"))` 用的是纯字符串路径，cooker 看不见，于是这些资产根本没被打进包。
- **正确**：在打包设置里显式声明目录，等价 ini：
  ```
  [/Script/UnrealEd.ProjectPackagingSettings]
  +DirectoriesToAlwaysCook=(Path="/Game/Audio")
  +DirectoriesToAlwaysCook=(Path="/Game/Materials")
  ```
  或者改用 `TSoftObjectPtr` / 资产注册表查询，让引用在 cook 时可见。
- **验证**：打包后检查 `%APPDATA%\Unreal Engine\AutomationTool\Logs\<Engine>\FinalCopyWin64_UFSFiles.txt` 里有没有对应 `.uasset`；
  再跑一次打包版，日志应为 `SFX loaded: coin=1 land=1`。
- **出处**：2026-11-17 实测——修复前 `coin=0 land=0`，加 `DirectoriesToAlwaysCook` 后 `coin=1 land=1`，cooked manifest 里出现
  `M_Coin/M_Character/M_Accent/SFX_Coin/SFX_Land`。

## 21. `IncludeDebugFiles` 管不到命令行打包，要去掉 pdb 得用 `-nodebuginfo`

- **症状**：在 `DefaultGame.ini` 写了 `[/Script/UnrealEd.ProjectPackagingSettings] IncludeDebugFiles=False`，
  `BuildCookRun` 打出来的 Shipping 包里仍然有 211 MB 的 `.pdb`。
- **原因**：该属性只在编辑器 **Turnkey**（平台菜单打包）里被读取（`TurnkeySupportModule.cpp:574`）；
  UAT 的 `BuildCookRun` 用的是 `ProjectParams.NoDebugInfo`（命令行 `-nodebuginfo`），两者互不相通。
- **正确**：给 `BuildCookRun` 加 `-nodebuginfo`。属性名也注意：是 `IncludeDebugFiles`，**没有 `b` 前缀**，写错会被静默忽略
  （以 `ProjectPackagingSettings.h:242` 的 UPROPERTY 名为准）。
- **验证**：Shipping 包里 `Get-ChildItem -Recurse -Filter *.pdb` 计数为 0，整包从 826 MB 降到 319 MB。
- **出处**：2026-11-17 实测；`AutomationUtils/ProjectParams.cs:2176-2185`、`DeveloperToolSettings/.../ProjectPackagingSettings.h:242`

## 22. Editor 目标没有 Shipping 配置

- **症状**：`Build.bat CoinRushEditor Win64 Shipping` 直接失败（退出码 6），打包脚本第一步就挂。
- **原因**：`CoinRushEditor` 只有 Debug/DebugGame/Development 等配置；Shipping 只对**游戏**目标有意义。
- **正确**：打 Shipping 包时，编辑器目标仍然编译 Development（cooker 只需要一个能跑的编辑器），游戏目标才用 Shipping。
  `Tools/Package.ps1` 已经按这个规则写死。
- **出处**：2026-11-17 实测；`Source/CoinRushEditor.Target.cs`
