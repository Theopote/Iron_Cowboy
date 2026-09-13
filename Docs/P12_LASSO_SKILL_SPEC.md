# P12 套索技巧与失控后果规格

**状态：第一增量已实现并验证；第二增量待实施**  
**依赖：P11**  
**当前范围：摆绳准备、投掷稳定性、Head/Neck/Torso 简化命中区域**

## 目标

让玩家通过观察摆绳节奏和选择命中位置提高成功率。投掷仍要求玩家判断方向与提前量，系统不使用随机散布或自动吸附替代操作。

## 第一增量规则

### 摆绳与稳定性

- 按住 RMB 进入 Aiming 后开始累计准备时间并循环摆绳相位。
- 准备未完成时仍允许抢投，但有效环口和射程较小。
- 每个摆绳周期中部为稳定窗口；LMB 出手时锁定本次 Stability。
- Stability 只缩放连续扫掠的有效半径、速度和最大射程，不改变玩家给出的瞄准方向。
- HUD 显示 SWING、OPEN 和稳定窗口提示；松开 RMB 取消并重置准备。

### 简化命中区域

当前灰盒马使用一个 Capsule，因此区域按目标局部高度划分：

| 区域 | 规则 | 结果 |
| --- | --- | --- |
| Head | 局部高度高于 85 cm | 张力放大 1.15，压制时间 1.05；命中可用但更容易过载 |
| Neck | 局部高度 35–85 cm | 张力 1.00，压制时间 0.85；当前最稳定命中 |
| Torso | 局部高度低于 35 cm | 张力 0.90，压制时间 1.25；不易骤断但控制更慢 |

区域来自 Gameplay 命中点并保存到 LassoComponent；视觉绳和 HUD 只读取结果。未来接入骨骼马模型时可改为碰撞体/骨骼映射，不改变后续控绳接口。

## 第一增量验收

1. Aiming 时间推进准备度和周期性 Stability，稳定窗口可预测。
2. 稳定出手的有效环口、速度和射程大于抢投。
3. Throw 后锁定 Stability，不受后续 Tick 改写。
4. Head、Neck、Torso 分类确定且各自影响张力或压制时间。
5. Neck 命中保持 P5–P11 完整闭环兼容。
6. 自动化覆盖纯规则与真实 UWorld 投掷；实际渲染烟测显示摆绳与 Neck 命中结果。

## 第二增量预留

后续在第一增量稳定后加入 Rider Balance、侧向拉力、Stumble/Fall，以及可主动松手的短距离 Dragged。阈值必须确定、失败可恢复，且不由动画决定 Gameplay 结果。

## 非目标

本增量不实现物理套索绳、骨骼级精确缠绕、随机风偏、自动锁定、伤害、骑手落马、拖行、投索动画或正式音效。

## 第一增量实现与验证结果

- Aiming 状态按时间计算准备度、摆绳相位和 Stability；出手时锁定本次有效环口、速度和射程。
- 实际命中点被确定性划分为 Head、Neck、Torso，并持续显示在 HUD；区域倍率进入张力与压制时间。
- UE 5.8.2 Development 构建成功；新增 `Steppe.P12.SwingTimingAndHitZones`，P1–P12 共 **16 passed、0 failed**，保留 1 项既有 RiderSeat 占位警告。
- 实际渲染烟测记录 `Stability=1.00`、`Zone=Neck`、`Radius=80.0`、`Range=2600.0`；P10 完整捕获到命名路线继续通过。

证据位于 `Validation/P12-Results.json`、`P12-Swing.png`、`P12-NeckHit.png`、`P12-FullLoop.png` 和 `P12-Runs.txt`。原始日志为 `Saved/Logs/P12-FinalAutomation.log`、`P12-LassoSkillRender.log` 与 `P12-FullLoopRegression.log`。
