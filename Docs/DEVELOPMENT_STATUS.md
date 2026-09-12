# Current Phase

**P2.1 — 单匹追逐避障、危险制动与 F2 场景重试已实现。**

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

自动测试：**5 passed, 0 failed**。

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

打开 Steppe.uproject → Play。前方约 38 m 的浅色方块是野马。先轻点 W 缓慢接近，再按 F2 重试并按住 W 高速接近，对比反应。拉开距离/遮挡视线后观察平静过程。

按键继承 P1：W/S、A/D、鼠标、Shift、Ctrl、E、F1。`steppe.Debug.Movement 1` 显示方向；野马不能 E 上马。完整流程与参数见 P2_WILD_HORSE.md。

# Known Limits

- 仅单匹野马与一个显式玩家目标；无马群、套索、捕获、多人。
- 仍是灰盒占位模型，没有真实马动画、听觉、鸣叫或奖励闭环。
- 局部探测不是全局寻路，复杂地形可能停住或绕行不理想；不保证任意障碍都能绕开。
- 感知为距离 + 全方向视线 + 靠近速度；数值是可调初值，不是马术研究结论。
- 用户尚未试玩本轮 P2，不能宣称追逐已经足够有趣。
- 仅验证 Editor Development 和 Editor -game，没有验证 Shipping 打包或其他平台。

# Next Recommended Work

试玩 P2 并调整感知范围、逃离触发、恢复速度与避障。确认单匹追逐体验后再另行讨论 P3 马群；本次不自动推进 P3。

2026-09-12 P2.1：新增 Steppe.P2.StoppingDistanceAndGaps 测试通过；两次单机场景重载验证通过。新增制动距离探测、沿途地面采样、危险制动 HUD 及 F2 重试。详情见 P2_WILD_HORSE.md 的 P2.1 节；新证据为 Validation/P21-Results.json、P21-Retry-Playground.png。上文 P2 速度和截图为历史记录。
