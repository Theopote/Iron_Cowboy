# Current Phase

**P3.1 — 马群个体差异、正确出生位置、动态避让与脱困已实现。**

2026-09-11：用户试玩 P1 并反馈基本可用，随后授权按单匹野马计划继续。P1 基线记录保存在 P1_BASELINE.md。原始提示词的 P1 停止条件没有被自动越过，本次 P2 来自后续明确授权。

# Completed

- 新增 SteppeWildHorseCharacter、HorseBrainComponent、WildHorseConfig。
- Roaming / Alert / Fleeing / Recovering 四状态，警觉积累、视线遮挡、相对靠近速度、最低状态持续时间与威胁记忆。
- AI 只生成 FHorseMovementIntent，复用现有 CMC 加减速、转向、体力与碰撞。
- 局部障碍探测和前方落脚检查；无可行方向时请求制动。
- 禁止 E 直接骑走野马；原坐骑上下马不受影响。
- GameMode 默认生成一匹野马，并显式绑定玩家骑手为感知目标。
- 新增行为遥测与橙色方向/目标显示，避免世界标签遮盖 HUD。
- Editor 创建真实野马 Blueprint、行为 DataAsset、占位材质，并保存引用到现有 GameMode；不重建已有地图。

# Build Result

**Succeeded — UE 5.8.2 / SteppeEditor Win64 Development。**

UHT、C++ 编译和链接成功。仍使用 P1 已验证的 NoPCHs / -NoUBA 本地构建设置，没有修改引擎，没有增加 AIModule、NavigationSystem、Mass 或 GAS 依赖。

# Validation

自动测试：**6 passed, 0 failed**。

| 测试 | 结果 |
| --- | --- |
| Steppe.P1.MathAndStamina | 通过 |
| Steppe.P1.WorldMovementAndRiding | 通过，2 条预期的占位 RiderSeat fallback 警告 |
| Steppe.P2.PerceptionAndState | 通过，无警告/错误 |
| Steppe.P2.MovementOwnershipAndObstacles | 通过，无警告/错误 |

P2 测试覆盖：同距离慢/快接近差异、警觉到逃离、极近威胁、遮挡、短期记忆、恢复、威胁对象销毁、真实渐进加速与远离、AI/Rider 意图分离、野马不可上马、围堵时制动。

实际游戏已启动并正常退出。记录到野马 Alert → Fleeing，截图时 Awareness=1.00、Visible=1、Speed=442.7 cm/s（仍在加速阶段），玩家坐骑为 Gallop 1200 cm/s。运行日志确认配置来自 `/Game/Steppe/Data/Horses/DA_WildHorse_Default`，不是仅使用代码默认值。HUD、方向向量和浅色野马在实际截图中可见。

证据：Docs/Validation/P2-Results.json、P2-Playground.png；原始临时日志在 Saved/Logs/P2-FinalTests.log 与 P2-FinalRender.log。

# Manual Steps

打开 Steppe.uproject → Play。前方约 38 m 是由 5 个浅色占位体组成的野马群。先轻点 W 缓慢接近，再按 F2 重试并按住 W 高速接近，对比个体响应顺序。拉开距离后观察成员分别恢复。

按键继承 P1：W/S、A/D、鼠标、Shift、Ctrl、E、F1、F2。`steppe.Debug.Movement 1` 显示个体方向和紫色群体中心；野马不能 E 上马。完整流程与参数见 P3_SMALL_HERD.md。

# Known Limits

- 仅 5 匹近距离完整 Actor 与一个显式玩家目标；无远距离简化、领头马社会结构、套索、捕获、多人。
- 仍是灰盒占位模型，没有真实马动画、听觉、鸣叫或奖励闭环。
- 局部探测不是全局寻路，复杂地形可能停住或绕行不理想；不保证任意障碍都能绕开。
- 感知为距离 + 全方向视线 + 靠近速度；数值是可调初值，不是马术研究结论。
- 用户已确认 P2.2 单匹反应感觉不错；P3 马群手感仍待试玩。
- 仅验证 Editor Development 和 Editor -game，没有验证 Shipping 打包或其他平台。

# Next Recommended Work

试玩 P3，重点观察慢速接近时局部退让、高速切入时的传播节奏、马匹间距和复杂障碍附近的聚散。确认马群体验后再决定进入 P4 目标切出，或先调整 P3 参数。

2026-09-12 P2.1：新增 Steppe.P2.StoppingDistanceAndGaps 测试通过；两次单机场景重载验证通过。新增制动距离探测、沿途地面采样、危险制动 HUD 及 F2 重试。详情见 P2_WILD_HORSE.md 的 P2.1 节；新证据为 Validation/P21-Results.json、P21-Retry-Playground.png。上文 P2 速度和截图为历史记录。

2026-09-12 P2.2：根据用户试玩反馈加入 Yielding，慢速压力不再无限积累至逃跑；贴身逃跑边界为 3.5 m，原 9 m 边界改为慢速退让。停止/退后即使可见也会恢复。新增使用真实配置资产的 SlowApproachAndRelease 测试通过。新证据见 Validation/P22-Results.json，当前规则和试玩步骤见 P2_WILD_HORSE.md 的 P2.2 节；前文为历史里程碑记录。

2026-09-12 P3：用户确认 P2.2 试玩感觉不错并明确授权小规模马群。新增 SteppeHerdManager，默认生成 5 匹完整野马，提供中心、方向、邻居、分离引导与按距离传播的群体警报。自动测试增至 7 项全部通过；实际游戏和两次场景重试通过，追逐截图时 4/5 匹仍在逃跑。当前验收与限制见 P3_SMALL_HERD.md。

2026-09-12 P3.1：根据试玩反馈修复五匹马开局同步移动及卡住问题。HerdManager 新增根组件，马群从误落世界原点恢复到配置的 38 米外锚点；成员获得独立目标、暂停、转向偏好和反应时间。环境探测不再把其他 Pawn 当成静态封路，新增玩家坐骑动态避让、12 向探测、拥挤侧向让路及全阻挡原地转向。静止与高速追逐实际验证均为 5 个不同朝向、0 阻塞，最小间距约 4.2 米。详情见 P3_SMALL_HERD.md 的 P3.1 节。
