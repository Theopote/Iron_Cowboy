# P12 套索技巧与失控后果规格

**状态：两次增量均已实现并验证**
**依赖：P11**  
**当前范围：摆绳准备、投掷稳定性、Head/Neck/Torso 简化命中区域、Rider Balance、落马与短距离拖行**

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

## 第二增量规则

### Rider Balance

- 独立 RiderBalanceComponent 读取当前套索张力、绳索相对坐骑的侧向比例、坐骑速度、目标 Strength 与命中区域。
- 低速、顺向或低张力时 Balance 恢复；持续侧向高负荷时先进入 Warning，再达到阈值触发 Fall。
- Head 放大失衡负荷，Torso 略微降低负荷；所有阈值和速率可编辑。
- HUD 持续显示 Balance、侧向比例和 SAFE/WARNING/FALL 状态。

### Fall 与 Dragged

- Fall 使用 RidingComponent 的事故脱离入口，一次性解除挂接、恢复 Rider 碰撞并施加确定性侧向/向上速度。
- 若套索仍附着且目标在最大拖行距离内，Rider 进入限时 Dragged；CharacterMovement 按目标方向设置受限速度。
- LMB 主动松绳立即结束 Dragged 并进入短暂 Recovering。P14.4 已将超时自动松绳改为站起后继续进入 Pulled 徒步控绳。
- Recovering 结束后回到 Stable，可以继续步行和后续玩法；不销毁 Rider、坐骑或目标。

## 第二增量验收

1. 同一张力和速度下，侧向拉力比顺向拉力产生更高 Balance Load。
2. 更高目标 Strength 和 Head 命中提高风险，Torso 降低风险。
3. 达到阈值后安全解除 Mounted 关系，Rider 进入 Falling 或 Dragged。
4. Dragged 有严格最大时间，LMB Release 可提前结束。
5. 状态恢复后角色仍存在并可移动；完整 P1–P12.1 闭环继续通过。

## 非目标

P12 不实现物理套索绳、骨骼级精确缠绕、随机风偏、自动锁定、生命值/伤害、布娃娃、长距离拖行、投索/落马正式动画或正式音效。

## 第一增量实现与验证结果

- Aiming 状态按时间计算准备度、摆绳相位和 Stability；出手时锁定本次有效环口、速度和射程。
- 实际命中点被确定性划分为 Head、Neck、Torso，并持续显示在 HUD；区域倍率进入张力与压制时间。
- UE 5.8.2 Development 构建成功；新增 `Steppe.P12.SwingTimingAndHitZones`，当时 P1–P12.1 共 **16 passed、0 failed**。
- 实际渲染烟测记录 `Stability=1.00`、`Zone=Neck`、`Radius=80.0`、`Range=2600.0`；P10 完整捕获到命名路线继续通过。

## 第二增量实现与验证结果

- 新增 RiderBalanceComponent，在 PostPhysics 读取已计算的套索张力，并结合侧向比例、坐骑速度、目标 Strength 和命中区计算负荷。
- Balance 达到 WarningThreshold 时给出红色预警；达到 FallThreshold 后通过 RidingComponent 事故入口解除挂接、恢复碰撞并进入 Falling/Dragged。
- 此处记录的是 P12 初始行为；P14.4 将 Dragged 调整为 2 秒，并在结束后保留绳索进入 Pulled，详见 `P14.4_ROPE_PERSISTENCE.md`。
- 新增 `Steppe.P12.BalanceFallAndDraggedRecovery`；最终 P1–P12 共 **17 passed、0 failed**，2 项因测试触发 RiderSeat 灰盒回退而带既有警告。
- 实际渲染记录 `Dragged / Mounted=0 / Lasso=Attached / Balance=1.00 / Side=1.00`，随后主动 Release 进入双 Recovering；完整命名闭环继续成功。

证据位于 `Validation/P12-Results.json`、`P12-Swing.png`、`P12-NeckHit.png`、`P12-Dragged.png`、`P12-FullLoop.png` 和 `P12-Runs.txt`。原始日志为 `Saved/Logs/P12-FinalAutomation.log`、`P12-LassoSkillRender.log`、`P12-BalanceRender.log` 与 `P12-BalanceFullLoop.log`。
