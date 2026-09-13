# 《套马的汉子》 / Project STEPPE

UE **5.8.2** 的 C++ 骑乘游戏原型。当前已完成 P9 捕获后安全接近；骑手可在限时任务中驱赶马群、切出目标、投掷套索、控制张力、下马安抚，并以第一次接触完成结算。

## 打开与试玩

1. 关闭正在运行的 Steppe Editor，在项目根目录执行：

```powershell
.\Scripts\Build.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.8'
```

2. 用 UE 5.8.2 打开 `Steppe.uproject`。默认加载 `L_Prototype_Grassland`。
3. 点击 Play，默认已骑上占位马。前方约 38 m 有五匹浅色野马，可缓慢接近、加速追逐，并用 Q 选择一匹尝试切出。无需下载模型或手工创建输入资产。

当前是灰盒原型：方块马身、圆柱骑手、2 km 平地、距离标记、绕桩与坡道；没有最终动画、音效或美术。代码通过与手感满意是不同验收，追逐、控绳和捕获后接近仍需持续人工试玩调参。

## 按键

| 按键 | 行为 |
| --- | --- |
| W | 前进意图；逐步加速至 Gallop |
| S | 制动，不倒车 |
| A / D | 缰绳转向，高速转向更慢 |
| 鼠标 | 独立自由观察，不改变马朝向 |
| 左 Shift + W | 请求 Sprint；受体力限制 |
| 左 Ctrl | 强制动 |
| E | 上马 / 下马；捕获后徒步靠近并平静停留，再按 E 完成第一次接触 |
| Q | 选择视线前方的野马；再次选择同一匹可取消 |
| 鼠标右键 | 隔离目标后按住瞄准套索 |
| 鼠标左键 | 瞄准时投掷；附着后释放 |
| 空格 | 套中后按住稳绳，在有效张力区间累计控制进度 |
| C | 目标达到 Subdued 后确认捕获 |
| F1 | 显示 / 隐藏马遥测 |
| F2 | 重试当前场景 |

松开 W 自然减速。Walk/Trot/Canter/Gallop/Sprint 根据实际速度和滞回计算，不是瞬间换挡。上下马切换 Enhanced Input Context。

## 调试与调参

控制台：`steppe.Debug.Horse 1` 显示状态、步态、速度、意图、转向、体力；`steppe.Debug.Movement 1` 显示向量。绿色=当前朝向，青色=速度，黄色=期望朝向，紫色=期望运动。`SteppeToggleDebug` 切换遥测。

主要调参资产：`/Game/Steppe/Data/Horses/DA_HorseLocomotion_Default`。包含各步态速度/加减速度、转向曲线、体力门槛、FOV/距离曲线、镜头 Lag。Horse Blueprint 的 Attributes 组件提供个体速度上限、敏捷、体力和质量；GameMode Blueprint 提供 Start Mounted、Horse Spawn Transform、Debug Enabled、Infinite Stamina。

## 验证

```powershell
.\Scripts\RunEditor.ps1 -Tests -ExpectedTests 13 -Commands 'Automation RunTests Steppe' -LogName P9-Automation
.\Scripts\RunEditor.ps1 -Game -Render -Smoke -PostCaptureSmoke -Commands '' -LogName P9-FinalRender
.\Scripts\RunEditor.ps1 -Game -Render -Smoke -VerticalSliceFailureSmoke -Commands '' -LogName P9-FailureRegression
```

第一条运行全部数学和真实 UWorld 集成测试，报告位于 `Saved/Automation/index.json`。第二条启动实际游戏，自动走完捕获、下马、平静接近和第一次接触并截图；第三条验证紧迫提示与超时失败。这些烟测不用于人工试玩。

当前构建使用 V7 / Unreal5_8 IncludeOrder；因本机共享 PCH 编译停顿，模块禁用 PCH，构建脚本传入 `-NoUBA` 禁用 detouring。没有修改引擎安装。Editor 开启 Live Coding 时应先保存关闭再运行外部构建。

后续开发先阅读 `Docs/GAME_DESIGN_VISION.md`、`Docs/DESIGN_BASELINE.md`、`Docs/PROTOTYPE_GDD.md` 和 `Docs/DEVELOPMENT_ROADMAP.md`。真实结果见 `Docs/DEVELOPMENT_STATUS.md`，工程分层见 `Docs/ARCHITECTURE.md`，P9 实现规格与验证依据见 `Docs/P9_CAPTURE_AFTERMATH_SPEC.md`；下一阶段按路线图进入 P10 牵回营地与命名。
