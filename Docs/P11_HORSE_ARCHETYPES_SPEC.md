# P11 可感知马匹个体差异规格

**状态：已实现并验证**
**依赖：P10**
**范围：Fast、Strong、Nervous 三种灰盒原型**

## 目标

玩家应能从追逐、控绳和接近行为中描述“这匹更快”“这匹更难控制”“这匹更容易受惊”，而不是只在 Horse Card 看到不同词条。

## 数据模型

HerdManager 持有可编辑的 Archetype Profile 数组，并按成员编号确定性分配。每个 Profile 包含：

- MaxSpeed、Acceleration、Stamina、Strength、Agility 倍率。
- FearRise、FearDecay、FlightSpeed 倍率。
- LassoStruggle 与 SubdueResistance 倍率。
- SafeApproachSpeed 与 CalmHold 倍率。
- 显示名、性格词、毛色和灰盒识别色。

Profile 在 WildHorse 完成生成后应用一次，避免 Blueprint 构造覆盖运行时参数和材质。Gameplay 系统只读取应用后的参数，不依赖资产路径或 HUD 文案。

## 三种原型

| 类型 | 可感知优势 | 可感知代价 |
| --- | --- | --- |
| Fast | 更高极速、加速和转向，更快逃跑 | 耐力与力量较低，控绳时间略短 |
| Strong | 更高耐力和力量，警觉恢复较快 | 极速、加速和敏捷较低，控绳时间更长 |
| Nervous | 警觉增长快、逃跑积极、转向灵活 | 平静恢复慢，安全接近速度更低、安抚停留更久 |

## 系统接入

- HorseMovement 继续读取 HorseAttributeComponent，因而速度、加速度、耐力和敏捷直接生效。
- HorseBrain 将 Fear 倍率用于 Awareness 增长/衰减，并将 Flight/Struggle 倍率用于运动意图。
- LassoComponent 使用目标 Strength 与 Profile 的 Resistance 计算实际 SubdueSeconds。
- HorseTrust 使用 Profile 调整安全接近速度和 CalmHoldSeconds。
- Horse Card 展示 Archetype 和已经参与 Gameplay 的最终属性。
- Placeholder 动态材质使用 Profile 识别色；颜色不参与判定。

## 自动化验收

1. 五匹默认马群至少包含 Fast、Strong、Nervous。
2. 相同基础值下 Fast 的速度/加速度高于 Strong。
3. Strong 的耐力、力量和控绳时间高于 Fast。
4. Nervous 的 FearRise 高于其他类型，FearDecay 更慢。
5. Nervous 的安全接近速度更低、CalmHold 更长。
6. Profile 只应用一次，重复调用不会叠乘。
7. P1–P10 既有流程继续通过。

## 实际试玩验收

- 同屏能通过灰盒颜色区分至少三种类型。
- 调试 HUD 能显示类型与关键倍率，便于调参。
- 实际运行日志记录三种类型的速度、力量、恐惧和接近参数。
- 玩家试玩时优先询问行为感受，再展示 Horse Card 标签。

## 非目标

P11 不实现遗传、随机品质、稀有度、经济价格、永久收藏、正式毛色材质、繁育或完整性格树。

## 实现与验证结果

- 五匹默认成员按编号确定性循环分配 Fast、Strong、Nervous；配置保存在 HerdManager 的可编辑 Profile 数组中。
- 属性倍率已接入 HorseMovement、HorseBrain、LassoComponent 与 HorseTrust，Horse Card 和 F1 调试标签展示同一份运行时结果。
- UE 5.8.2 Development 构建成功；`Steppe.P11.ArchetypeGameplayDifferences` 与 P1–P10 回归共 **15 passed、0 failed**，保留 1 项既有 RiderSeat 占位警告。
- 实际渲染烟测同时生成三类马，并记录 H1 Fast、H2 Strong、H3 Nervous 的速度、力量、恐惧与接近参数；完整成功链路和超时失败路线均通过。

证据位于 `Validation/P11-Results.json`、`P11-Archetypes.png`、`P11-FullLoop.png` 和 `P11-Runs.txt`。原始日志为 `Saved/Logs/P11-FinalAutomation.log`、`P11-ArchetypeRender.log`、`P11-FullLoopRegression.log` 与 `P11-FailureRegression.log`。
