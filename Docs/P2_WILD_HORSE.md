# P2 — 单匹野马 AI

## 本轮范围

用户试玩 P1 后反馈“感觉还可以”，并明确要求按提出的单匹野马计划继续开发。因此本轮进入 P2；原始主提示词中 P1 停止点保留为历史需求，不再作为禁止本次已授权 P2 的条件。没有扩展到马群、套索、捕获或多人。

## 如何试玩

使用 UE 5.8.2 打开 Steppe.uproject，点击 Play。默认仍骑在原有坐骑上；浅色野马出生在坐骑前方约 38 m 处，随后会在附近闲逛。

1. 第一轮小幅点按 W，缓慢接近，观察 `WILD HORSE` 面板从 Roaming 进入 Alert。
2. 停止 Play 后重新开始，按住 W 冲过去，比较警觉与逃离出现的时机。
3. 追逐时用 A/D 调整方向；野马也有加速和转向限制，不会瞬间转向或变速。
4. 松开 W 或制动，拉开距离，或者利用障碍阻断视线。短暂失去视线仍保留威胁记忆，之后进入 Recovering，再平静闲逛。
5. 野马不能直接按 E 骑走。E 仍可用于自己的坐骑。

F1 切换遥测；`steppe.Debug.Movement 1` 开启向量。橙色箭头/球表示野马选择的运动方向和目标。HUD 显示状态、警觉度、威胁距离、靠近速度、是否可见及前方是否被挡住。

## 状态与运动

`显式指定的骑手目标 → 距离/相对接近速度/视线检测 → HorseBrainComponent → FHorseMovementIntent → 已有 HorseMovementComponent → CMC 位移`。

- Roaming：在小范围内选择目标，间歇停留，使用 Walk 请求。
- Alert：暂停前进并观察；保持最低停留时间，避免边界反复切换。
- Fleeing：远离最后一次看到的目标位置；仍受现有加速度、转向和体力约束。极近或高速冲近可立即进入逃离。
- Recovering：逐步减速和平静；新的明显威胁可再次触发警觉/逃离。

威胁目标使用弱引用，没有每帧全场搜索。骑手上马后其自身 Movement 不再提供真实速度，感知层读取其附着坐骑的速度。检测采用全方向视线，不模拟听觉、视野锥或真实马术生物数据。

Brain 默认每 0.1 秒决策，运动组件逐帧运行，并显式依赖 Brain Tick。Brain 不修改 Actor 位置或旋转，不创建 AIController、行为树或 NavMesh 依赖。

局部转向采样八个方向，检测碰撞和候选方向前方的地面支撑；所有方向都被挡住时请求制动。它不是全局寻路：复杂迷宫、窄沟和陡峭地形仍需后续工作，不能保证任意障碍都能绕过。

## 调参

真实行为资产：`/Game/Steppe/Data/Horses/DA_WildHorse_Default`。

| 参数 | 初值 | 作用 |
| --- | --- | --- |
| DecisionInterval | 0.1 s | 决策频率 |
| NoticeDistance | 3500 cm | 可见威胁的感知范围 |
| FlightDistance | 900 cm | 极近立即逃离 |
| FastApproachDistance | 2500 cm | 高速接近触发范围 |
| FastClosingSpeed | 700 cm/s | 相对靠近速度门槛 |
| AwarenessRiseRate / DecayRate | 0.35 / 0.18 | 警觉积累与消退 |
| AlertThreshold / FlightThreshold | 0.25 / 0.75 | 普通警觉与逃离阈值 |
| CalmThreshold | 0.1 | 恢复平静阈值，应低于 AlertThreshold |
| MinimumAlertSeconds | 0.8 s | 警觉最低持续时间（紧急危险可打断） |
| MinimumFlightSeconds | 3 s | 最短逃离时间 |
| ThreatMemorySeconds | 3 s | 失去视线后的记忆时间 |
| RecoverySeconds | 4 s | 最短恢复时间 |
| RoamRadius / RoamSpeed | 1500 cm / 180 cm/s | 闲逛范围与速度 |
| FlightSpeed | 1200 cm/s | 逃离请求速度，最终由运动层限制 |
| ProbeDistance / ProbeSeconds | 450 cm / 1 s | 障碍前瞻距离，随速度增大 |

运动参数仍来自 `/Game/Steppe/Data/Horses/DA_HorseLocomotion_Default`。状态阈值推荐保持 `Calm < Alert < Flight`；探测范围推荐 `FlightDistance < FastApproachDistance < NoticeDistance`。

BP_SteppeGameMode 新增 SpawnWildHorse、WildHorseSpawnTransform、WildHorseClass。关闭 SpawnWildHorse 可回到纯 P1 运动场。野马 Blueprint 为 `/Game/Steppe/Characters/Horses/BP_SteppeWildHorse`，继承原生 SteppeWildHorseCharacter；Brain 的 Config 可替换。

复建资产：

```powershell
.\Scripts\RunEditor.ps1 -PythonScript 'D:\development\Project STEPPE\Scripts\CreateWildHorse.py' -LogName P2-Assets
```

脚本保存真实资产，仅新增/接入野马，不重建已有地图或覆盖坐骑运动参数。

## 自动验证

```powershell
.\Scripts\Build.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.8'
.\Scripts\RunEditor.ps1 -Commands 'Automation RunTests Steppe.' -Tests -ExpectedTests 4 -LogName P2-FinalTests
.\Scripts\RunEditor.ps1 -Game -Render -Smoke -Commands 'steppe.Debug.Movement 1' -LogName P2-Render
```

已通过 4 项测试：两项 P1 回归，以及 P2 PerceptionAndState、MovementOwnershipAndObstacles。P2 覆盖慢/快接近差异、遮挡、记忆、恢复、目标销毁、真实渐进移动、意图来源分离、野马不可上马、被围堵时制动。

## 尚需人工评价

- 慢靠近与高速追逐是否有可感知的差别？
- 野马逃离是否可读，是否有合理追逐空间？
- 在绕桩场景是否出现长时间卡住或左右摇摆？
- 警觉和恢复时间是否自然？

当前仍是方块占位体，没有马动画、鸣叫、捕获或奖励闭环。本轮停在单匹野马，等待试玩反馈。

## P2.1 单匹追逐完善（2026-09-12）

F2 重载当前单机场景，复位骑手、坐骑、野马状态与计时器；控制台等价命令为 `SteppeRestartTrial`。步行和骑乘均可使用，离开场景会移除旧输入映射。自定义 InputConfig 需配置 RestartTrial 动作；现有原型使用运行时默认配置。

避障按当前速度、紧急制动率、决策间隔及安全余量扩大探测距离，当前运动方向危险时输出紧急制动意图，实际运动仍由 CMC 处理。沿途采样地面，拒绝中途缺地、不可行走坡面和过大落差。默认 GroundSampleSpacing=100 cm、MaximumGroundDrop=60 cm、BrakeSafetyDistance=150 cm，每方向最多 32 个地面样本。HUD 的 Path 区分 clear、blocked、hazard braking。

UE 5.8.2 编译通过，5 项测试通过（骑乘测试保留预期座位占位警告）。新增 StoppingDistanceAndGaps 覆盖高速时旧探测范围之外的墙、中途沟隙、危险制动意图与安全方向解除制动。实际游戏连续重载两次，每次恢复上马和零警觉，随后重新进入 Fleeing。

复现：
```powershell
.\Scripts\RunEditor.ps1 -Tests -ExpectedTests 5 -Commands 'Automation RunTests Steppe.' -LogName P21-Tests
.\Scripts\RunEditor.ps1 -Game -Render -Smoke -RetrySmoke -Commands 'steppe.Debug.Movement 1' -LogName P21-RetryRender
```

证据：Validation/P21-Results.json、Validation/P21-Retry-Playground.png；日志 Saved/Logs/P21-Tests.log 与 P21-RetryRender.log。自动重试调用与 F2 相同的方法，尚未模拟物理键盘 F2 或单独验证 PIE 重载。

试玩时按 W 接近野马，观察转向与 Path，按 F2 回起点比较慢速和高速接近。复杂障碍仍可能停住，尚无全局寻路；有限采样可能漏掉很窄的沟隙，急坡可能被保守拒绝，不能保证任意速度/地形均及时停下。本轮保持 P2。

## P2.2 慢速接近与压力解除（2026-09-12）

用户试玩反馈：慢速与快速接近看起来都会直接逃跑。本轮将反应拆为观察、缓慢退让与逃跑，以下规则替代前文 P2 的无条件 9 米逃跑及可见即累积警觉规则。

| 玩家行为 | 默认反应 |
| --- | --- |
| 低于 7 m/s 朝野马接近 | 先 Alert 停下观察，持续慢速压力的警觉上限为 60%，不会仅因等得久而触发逃跑 |
| 慢速持续逼近至约 9 m | 观察至少 0.8 秒后进入 Yielding，按 220 cm/s 的目标速度走开 |
| 7 m/s 及以上朝野马接近，距离在 25 m 内 | 立即 Fleeing，目标速度 1200 cm/s |
| 任意方式进入 3.5 m 内 | 仍会立即逃跑，保留贴身安全距离 |
| 停下、退后或不再朝野马移动 | 警觉下降；退让回到观察，逃跑在满足至少 3 秒逃跑和 2 秒压力解除后进入恢复；无需完全离开视线 |

速度为朝向野马的玩家运动分量，不是按键按下时间；侧向经过不等于直接逼近。HUD 的 approach 显示该有符号速度，负数表示退后。原 ClosingSpeed 仍保留为双方相对靠近速度；逃跑中的野马不会因自己跑得更快而误判玩家停止追赶。状态行为标签新增 Horse.State.Yielding。

兼容现有 DataAsset：原 FlightDistance 属性保留，但编辑器显示名改为 Yield Distance，现用于退让边界；新增 PanicDistance=350、ApproachDeadZone=20、PressureReleaseSeconds=2、YieldSpeed=220。未覆写已有调参或重建地图。Yielding 追加到状态枚举末尾，保持旧状态序号。

试玩：F2 回起点，轻点 W 保持 HUD approach 低于 7 m/s，观察 Alert → Yielding；松开 W 后若坐骑仍在滑行，可用 Ctrl 刹停，观察警觉下降。再 F2 重试并持续按 W 加速，比较更远处触发 Fleeing。野马仍为占位模型，观察与退让主要通过停止/走开及 HUD 辨认，没有抬头动画。

验证：UE 5.8.2 编译成功，6 项自动测试通过。新增 SlowApproachAndRelease 使用实际 DA_WildHorse_Default，覆盖长时间慢速压力、8 米退让、停止后可见恢复、20 米快速逃跑、野马跑得比玩家快时保持逃跑、玩家退后解除压力，以及通过真实 CMC 产生低速位移。原骑乘、感知遮挡、避障与沟隙测试均通过。运行命令：
```powershell
.\Scripts\RunEditor.ps1 -Tests -ExpectedTests 6 -Commands 'Automation RunTests Steppe.' -LogName P22-Tests
.\Scripts\RunEditor.ps1 -Game -Render -Smoke -RetrySmoke -Commands 'steppe.Debug.Movement 1' -LogName P22-Render
```

证据：Validation/P22-Results.json、P22-Playground.png、P22-Retry.txt。自动测试验证规则差异，实际手感仍待试玩。本轮为 P2.2，尚未实现马群。
