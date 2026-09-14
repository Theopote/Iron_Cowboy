# Current Phase

**P12 — 套索技巧、命中区域、Rider Balance、落马与短距离拖行已经实现并验证。**

项目使用 UE 5.8.2。完整灰盒闭环继续可玩；套索阶段现在同时要求出手节奏、瞄准高度、张力管理和骑行路线判断。

# Completed

- P1–P11 的骑乘、五匹马群、三类马匹原型、目标切出、安全接近、牵回、Horse Card 和命名闭环保持有效。
- RMB Aiming 以 1.2 秒周期输出 SwingPhase 和 SwingStability；LMB 出手锁定 Stability，并决定本次有效环口、速度与射程。
- 连续扫掠命中按目标局部高度划分 Head、Neck、Torso，区域倍率进入张力、压制时间和骑手失衡负荷。
- RiderBalanceComponent 在 PostPhysics 读取张力、绳索侧向比例、坐骑速度、目标 Strength 和命中区，低风险时自动恢复。
- Balance 超过 55% 进入 Warning；满值后事故脱离坐骑，恢复 Rider 碰撞并施加确定性 Falling 速度。
- 落马后若套索仍附着且目标在 18 米内，会进入最长 1.25 秒 Dragged；CharacterMovement 处理受限拖行速度。
- LMB 可立即松绳结束拖行；超时也会自动松绳。Recovering 结束后 Rider 保留并可移动、重新上马。
- HUD 显示 SWING、OPEN、命中区、张力、CONTROL、BALANCE、SIDE 与 DRAGGED 提示；状态同时通过 Native Gameplay Tags 暴露。

# Build Result

**Succeeded — UE 5.8.2 / SteppeEditor Win64 Development。**

UHT、C++、UMG 编译与链接成功。没有修改引擎，也没有增加物理绳、随机散布、自动锁定、生命值、布娃娃或骨骼资源依赖。

# Validation

自动测试：**17 passed, 0 failed**。其中 2 项因覆盖正常骑乘和事故落马而记录既有 RiderSeat 灰盒回退警告。

`Steppe.P12.SwingTimingAndHitZones` 覆盖抢投/稳定出手、参数锁定、真实 Neck 附着和三类区域倍率。`Steppe.P12.BalanceFallAndDraggedRecovery` 覆盖顺向/侧向负荷、Strong/Head 风险、Torso 缓和、强制落马、Dragged Tag、主动松绳、自动拖行上限、恢复和重新上马。P1–P11 回归继续通过。

实际渲染稳定投掷记录 `Stability=1.00 / Zone=Neck / Radius=80 / Range=2600`。失控路线记录 `State=Dragged / Mounted=0 / Lasso=Attached / Balance=1.00 / Side=1.00`，LMB 后 Rider Balance 与 Lasso 都进入 Recovering。正向完整路线仍将 `Saran` 牵回、命名并以 Trial Success 结束。

证据：`Validation/P12-Results.json`、`P12-Swing.png`、`P12-NeckHit.png`、`P12-Dragged.png`、`P12-FullLoop.png` 和 `P12-Runs.txt`。原始日志为 `Saved/Logs/P12-FinalAutomation.log`、`P12-LassoSkillRender.log`、`P12-BalanceRender.log` 与 `P12-BalanceFullLoop.log`。

# Manual Steps

完成目标切出后按住 RMB，观察 OPEN 条，在绿色 THROW 窗口用 LMB 出手。调整准星高度尝试 Head、Neck、Torso。附着后按 Space 控绳，同时观察 BALANCE 与 SIDE：高 SIDE 时转向绳索方向或减速；持续高负荷会落马并显示 DRAGGED，此时 LMB 松绳。恢复后可以继续步行和重新上马。F2 重玩。

# Known Limits

- 摆绳、落马和拖行目前由状态、速度、HUD 和占位模型表达，尚无正式角色动画、布娃娃、绳圈模型或音效。
- 命中区基于灰盒 Capsule 局部高度；换成骨骼马后应改用独立碰撞体或骨骼映射。
- Balance 周期、阈值、拖行速度和 1.25 秒上限尚未经过多人试玩平衡。
- 没有生命值或伤害；落马是可恢复的操作后果。
- 当前 120 秒挑战尚未达到 6–10 分钟目标节奏，也未完成多人次人工试玩指标采集。
- 仅验证 Editor Development 和 Editor -game，没有验证 Shipping 打包或其他平台。

# Next Recommended Work

进入 P13 垂直切片音画。先接入不改变 Gameplay 的程序化/占位音频与 VFX 事件：马蹄节奏、冲刺呼吸、套索摆动/飞行/拉紧/断裂、Balance Warning、落马和 Dragged；再增加追逐尘土、绳索受力和状态切换反馈。资源缺失时仍须保持现有状态机和测试结果。

# Milestones

- 2026-09-11 P1：用户试玩并确认骑乘基础基本可用。
- 2026-09-12 P2–P3.1：完成单匹野马、慢速退让、五匹马群、个体时序、动态避让与脱困。
- 2026-09-12 P4–P7：完成视线选马、目标切出、套索投掷、绳索对抗与捕获登记。
- 2026-09-13 P8–P10：完成任务结算、捕获后安全接近、牵回营地、Horse Card 和本轮命名。
- 2026-09-13 P11：完成 Fast、Strong、Nervous 三类原型；15 项测试和成功/失败回归通过。
- 2026-09-13 P12.1：完成确定性摆绳稳定窗口、投掷参数和三类命中区域。
- 2026-09-14 P12.2：完成 Rider Balance、侧向负荷、落马、限时拖行和主动松绳恢复；17 项测试、两条 P12 渲染路线和完整闭环回归通过。
