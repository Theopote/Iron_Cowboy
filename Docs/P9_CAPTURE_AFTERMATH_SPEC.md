# P9 捕获后安全接近规格

**状态：已实现并验证**
**依赖：P8.2**  
**范围：从捕获登记到第一次安全接触**

## 目标

P8.2 曾在按 C 后立即完成任务。P9 已将“绳索上控制住”与“动物愿意接受第一次接触”分开：玩家捕获后必须安全下马、观察并缓慢靠近。过快接近会被拒绝，但不会抹掉已经完成的绳索对抗。

P9 结束于第一次接触。牵回营地、Horse Card 和命名属于 P10。

## 玩家流程

1. 目标达到 Subdued 后按 C，进入 `Secured`。
2. HUD 提示玩家减速并下马；骑乘状态下不能推进接触。
3. 玩家步行靠近。目标根据玩家距离和接近速度增加或释放压力。
4. 玩家冲得太快时，目标退开并显示可解释反馈；玩家停下后可以恢复。
5. 玩家在安全距离内保持平静，目标进入 `ReadyForContact`。
6. 玩家使用上下文交互完成第一次接触，进入 `FirstContact`，P9 目标完成。

## 状态模型

新增独立的捕获后状态，避免改变 HorseBrain 已有的 `Captured` 行为含义：

```text
Inactive
  → Secured
  → CalmApproach
  → ReadyForContact
  → FirstContact
```

`Rejected` 作为短暂反馈或事件，不应成为无法恢复的终止状态。玩家退开或停顿后回到 `CalmApproach`。

建议由实际参与计算的新组件持有状态，例如 `UHorseTrustComponent`；不要预先创建 P10 以后才使用的空类。HorseBrain 继续负责目标身体运动，组件只计算接近压力、接触准备度和最小 Trust 结果，再提交行为请求。

## 初始调参

以下数值是灰盒起点，必须集中到数据资产或组件可编辑参数中：

| 参数 | 初值 | 含义 |
| --- | ---: | --- |
| EvaluationInterval | 0.10 s | 接近规则更新频率 |
| AwarenessRadius | 800 cm | 开始明显读取玩家接近 |
| SafeApproachSpeed | 120 cm/s | 可持续降低压力的最大接近速度 |
| RushApproachSpeed | 300 cm/s | 触发拒绝的接近速度 |
| ContactDistance | 220 cm | 可建立接触的水平距离 |
| CalmHoldSeconds | 2.5 s | 安全距离内所需停留时间 |
| RejectionCooldown | 1.5 s | 被拒后最短恢复时间 |
| FirstContactTrust | 10 | 第一次接触建立的最小信任值 |

距离使用二维距离；接近速度必须是朝向目标的有符号速度，远离目标不应被误判为冲入。

## 行为规则

- 玩家骑乘、下落或高速移动时，`CalmHold` 不增长。
- 玩家在 AwarenessRadius 内且 ApproachSpeed 超过 RushApproachSpeed 时触发拒绝。
- 拒绝时目标通过 Horse Intent 后退或侧移，不直接设置 Actor 位置。
- 玩家停止或后退时压力逐步下降，不要求离开整个区域重来。
- 只有 `ReadyForContact` 且玩家未骑乘、处于 ContactDistance 内时，上下文交互才成功。
- 第一次接触后目标保持 Captured 登记，不重新加入 HerdManager 的活动成员。
- F2 重试清理全部 P9 状态。

## 输入与 HUD

- 沿用 E 作为上下文交互：骑乘时负责下马，徒步且满足接触条件时执行第一次接触。
- P9 不新增独立“安抚”按键；安抚来自慢速、停顿与距离管理。
- HUD 使用行为语言：`Slow down and dismount`、`Approach slowly`、`Give the horse space`、`Hold still`、`Press E for first contact`。
- 可显示粗粒度状态或环形进度，但不显示精确 Trust 数值。
- F1 调试信息增加距离、ApproachSpeed、Pressure、CalmHold 和 PostCaptureState。

## 失败与恢复

P9 不增加任务永久失败。冲入、重新上马或离开接触区只延长完成时间。超时仍由 Trial 规则处理，并采用以下已确认规则：

- 计时持续到 `FirstContact`，因为 P9 的目标是验证完整接近过程。
- 捕获数量继续在按 C 时登记，但 Trial 的完成条件改为 FirstContactCount，以避免提前结算。

实现已同步更新 P8 任务测试、成功烟测、HUD 文案和计分时点。

## 架构接口

- `ASteppeHerdManager` 继续拥有活动成员与捕获登记，不负责信任计算。
- `ULassoComponent` 只提交 Secured/Captured 事件，不承担接近感知。
- 捕获后组件读取当前玩家 Pawn 或由 GameMode 显式设置交互者，不逐帧全局搜索。
- HorseBrain 接收后退/停步意图；HorseMovement 仍是实际位移的唯一权威。
- Trial 读取“第一次接触已完成”的权威计数或事件，不从 HUD 推断。

## 自动化验收

至少覆盖：

1. 捕获前组件保持 Inactive。
2. 骑乘玩家不能累计 CalmHold。
3. 缓慢接近并停留可达到 ReadyForContact。
4. 高速正向接近触发拒绝并清除当前 CalmHold。
5. 远离目标的高速运动不触发冲入拒绝。
6. 拒绝冷却后可恢复并再次推进。
7. 距离不足或状态错误时交互失败。
8. 第一次接触只计数一次并保留捕获登记。
9. 目标销毁、玩家销毁或 F2 重试安全清理引用。

## 实际试玩验收

- 玩家不看调试 HUD 能理解为什么目标拒绝接近。
- 首次尝试可在 30–90 秒内学会接近规则。
- 失败后玩家会调整速度或停顿，而不是反复按交互键。
- 接近过程与刚结束的高速控绳形成明显节奏变化。
- 完成第一次接触后再显示成功结算，整段流程无控制台干预。

## 非目标

P9 不实现牵行、营地交付、命名、跨局存档、喂食、刷物品增加好感、复杂情绪树、踢击伤害或正式马匹动画。

## 实现结果

2026-09-13 已完成：

- `UHorseTrustComponent` 持有捕获后状态、接近压力、平静进度、最低 Trust 和第一次接触结果。
- 骑乘状态不推进安抚；高速正向接近进入 Rejected，目标通过 HorseBrain 请求步行后退；停止或后退后可以恢复。
- E 在捕获目标附近优先处理第一次接触，避免误触发无关上马。
- Trial 记录 Secure 与 Contact 两个计数，并在 FirstContact 后才成功结算。
- HUD 显示上下文提示和平静进度，第一次接触后显示 `FIRST CONTACT`。

验证：13 passed、0 failed；实际烟测完成捕获、下马、56% 过程截图、100% 第一次接触和任务结算。证据位于 `Validation/P9-Results.json`、`P9-Approach.png`、`P9-FirstContact.png` 和 `P9-Runs.txt`。
