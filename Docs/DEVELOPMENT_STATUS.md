# Current Phase

**P1 — 骑乘灰盒实现与自动验证完成；等待人工手感验收。**

状态更新：2026-09-11。构建/自动测试与基础渲染证据来自 2026-09-10；调试向量渲染于 2026-09-11 补充验证。停止在 P1，不进入 P2。

# Completed

- P0：Steppe C++ 工程、Game/Editor Target、Native Gameplay Tags、日志、Enhanced Input、GameMode/Controller、Debug Subsystem。
- 已发现并使用 `C:\Program Files\Epic Games\UE_5.8`，Build.version 为 5.8.2。此前“缺少 UE 5.8”的阻塞已解除。
- P1：Horse/Rider、独立 RiderIntent/HorseIntent、渐进响应、加减速、强制动、速度相关转向、步态滞回、冲刺与体力恢复门槛。
- 上下马：Socket/fallback 附着、安全落脚检测、高速拒绝下马、占用检查、对象销毁清理。
- Enhanced Input 自动运行时映射与 OnFoot/Riding Context 切换。
- 自由观察、速度 FOV/距离、加速与转向镜头偏移、Lag；只读动画数据和运行状态 Tags。
- HUD 遥测与可开关世界向量。
- Editor Python 创建真实地图、蓝图、运动 DataAsset 和灰盒材质；2 km 平地、绕桩、25 m 间隔标记与坡道。
- 补齐 README、架构、Editor 设置与本验收表。

# Build Result

**Succeeded — SteppeEditor Win64 Development，UE 5.8.2。**

使用 Visual Studio 2022 MSVC 14.44 与 Windows SDK 10.0.26100.0。UHT、C++ 编译和链接成功，Editor 加载并正常退出。未验证 Shipping 打包、其他平台或网络运行。

兼容修复：V7 build settings + Unreal5_8 include order。共享 PCH 导致本机后续编译停顿，模块改为 NoPCHs，脚本使用 -NoUBA；没有修改引擎文件。Live Coding 打开时外部构建会被拒绝，应先保存关闭 Editor。

# Automated Validation

**2 tests passed, 0 failed；其中 1 项带 2 条预期警告。**

- Steppe.P1.MathAndStamina：速度界限、加速度、制动不倒退、不超调、30/60/120 Hz 数学结果一致、步态滞回、转向曲线、体力上下限、单位转换、意图 Clamp/Reset。
- Steppe.P1.WorldMovementAndRiding：真实 UWorld/碰撞场景，渐进加速、Gallop、自然滑行与更强制动、冲刺消耗、耗尽限速、高速转向限制与压力、安全上下马、骑手意图传递、速度镜头变化、销毁坐骑释放骑手。
- 测量：1 秒后 180 cm/s，8 秒后 1200 cm/s。运动不是瞬间匹配输入。
- 两条警告均为占位马没有 RiderSeat Socket，按设计使用 fallback 座位。此前测试 World 初始化、BeginPlay/帧计数与 EndPlay 清理问题均已修复，最终无这些错误。

实际渲染启动：加载真实地图和 Blueprint GameMode，自动上马，固定 60 Hz 前进；日志记录 `Mounted=1 Speed=1200.0`，截图显示 Gallop、43.2 km/h、正常遥测，游戏正常退出。画面中的未构建光照提示已通过动态灯光和地图设置解决。

证据：

- `Docs/Validation/P1-Results.json`：精简测试结果与事件。
- `Docs/Validation/P1-Playground.png`：真实游戏截图。
- `Docs/Validation/P1-DebugVectors.png`：开启调试向量的真实游戏截图。
- `Saved/Automation/index.json`：完整临时自动测试报告。
- `Saved/Logs/P1-FinalTests.log` / `P1-FinalRender.log`：最终运行日志。

引擎启动阶段仍有引擎自身的分析 DLL、非 Windows 平台 SDK、内部测试/编辑器模块日志；它们不计作 Steppe 自动测试成功的证据，也不等同于项目源码报错。最终项目测试结果以报告中的两项 Steppe 测试为准。

# In Progress

仅剩人工玩法/手感验收：至少连续骑行 10 分钟，评价速度感、重量、转弯预判、低速控制、冲刺价值和镜头舒适度。当前没有把此项标为完成。

# Blocked

无当前构建阻塞。缺少最终马骨架/模型/动画不阻塞灰盒运动验证。

# Manual Editor Steps

打开 Steppe.uproject → 默认测试地图 → Play。无需额外输入资产。按键和具体调参路径见 EDITOR_SETUP.md。以后导入真实马模型、RiderSeat Socket 和 AnimBP；当前占位体是有意保留的原型表现。

# P1 Tuning Parameters

`/Game/Steppe/Data/Horses/DA_HorseLocomotion_Default`：Gaits、ResponseSeconds、EmergencyBrakeRate、SpeedTurnCurve、GaitHysteresis、ExhaustionThreshold、SprintResumeThreshold、SpeedFOVCurve、SpeedDistanceCurve、CameraBlendRate、CameraLagSpeed。

Horse Attributes：MaxSpeed、Acceleration、Deceleration、BaseTurnRate、Agility、MaxStamina、StaminaDrainRate、StaminaRecoveryRate。GameMode：StartMounted、HorseSpawnTransform、DebugEnabled、InfiniteStamina。

# P1 Acceptance Checklist

勾选表示功能已实现并经代码检查/相应自动验证；不替代最后的人工手感验收。向量绘制已通过真实游戏截图验证；实体键鼠完整操作与舒适度仍需 PIE 人工检查。

- [x] Project compiles — Editor Development 成功
- [x] Rider can mount horse — 集成测试与实际启动
- [x] Rider input becomes riding intent — 集成测试
- [x] Horse movement does not directly mirror raw input — 独立响应层与渐进运动
- [x] Horse accelerates progressively — 1 s / 8 s 测量
- [x] Horse decelerates progressively — 集成测试
- [x] Brake works — 集成测试
- [x] Low-speed turning is responsive — 曲线及实际低速转向率验证，舒适度待人工
- [x] High-speed turning is limited — 实际转向率比较
- [x] Gait is calculated — 滞回测试及画面 Gallop
- [x] Sprint consumes stamina — 集成测试
- [x] Low stamina limits sprint — 集成测试
- [x] Camera changes with speed — 集成测试
- [x] Camera supports free look — 观察与马朝向解耦、偏侧视角渲染；鼠标手感待人工
- [x] Horse animation data is exposed — Blueprint 只读结构
- [x] Horse debug information is available — 实际 HUD 截图
- [x] Debug vectors work — 已通过控制台开启并截图验证，开关与颜色见 README
- [x] Config is data-driven — 真实数据资产 + 安全默认值
- [x] No fake .uasset assets were generated — 由 Editor 保存
- [x] Manual editor work is documented
- [x] P2 systems were NOT implemented
- [ ] 连续至少 10 分钟人工骑乘手感验收

# Known Issues / Limits

- 方块马身与圆柱骑手，没有最终马动画、音效、UI 美术或草原生态。
- 无马骨架 Socket 时输出预期 Warning，使用可调 fallback。
- 没有倒退、绊倒/摔落玩法、地表材质牵引力或坡度体力惩罚；P1 仅基础 CMC 坡面和落地。
- 碰撞采用直立胶囊体，尚非马体轮廓；复杂地形需继续 P1 调试。
- 输入运行时创建，不是独立 Input .uasset；需要可复用编辑器输入资产时按 EDITOR_SETUP 替换。
- 仅验证 Editor Development 与 Editor -game 渲染；未验证 Shipping 包、多人预测或正式性能指标。

# Next Recommended Work

进行 P1 手感试玩并反馈，继续只调整 P1。核心参数满意后再另行讨论 P2。本次停止功能扩张。
