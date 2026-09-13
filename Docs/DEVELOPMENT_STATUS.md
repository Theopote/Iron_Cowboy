# Current Phase

**P12 第一增量 — 摆绳稳定窗口与 Head/Neck/Torso 命中区域已经实现并完成实际渲染验证。**

项目使用 UE 5.8.2。完整灰盒闭环继续可玩；套索投掷现在要求玩家在 RMB 摆绳期间观察稳定窗口，再用 LMB 决定出手时机和瞄准高度。

# Completed

- P1–P11 的骑乘、五匹马群、三类马匹原型、目标切出、套索对抗、安全接近、牵回、Horse Card 和命名闭环保持有效。
- Aiming 持续累计准备时间，并以 1.2 秒周期输出 SwingPhase 和 SwingStability。
- Stability 在出手时锁定；抢投保留 45% 环口、75% 速度和 80% 射程，稳定出手恢复完整参数。
- Stability 不修改玩家瞄准方向，不增加随机散布，也不自动吸附目标。
- 连续扫掠命中后按目标局部高度划分 Head、Neck、Torso。
- Neck 的压制时间倍率为 0.85；Head 的张力/压制倍率为 1.15/1.05；Torso 为 0.90/1.25。
- HUD 在 Aiming 时显示 SWING、OPEN、稳定窗口与绿色准星；附着后持续显示命中区域、张力和控制进度。
- 专用 `-LassoSkillSmoke` 固定展示目标，并验证满稳定度 Neck 投掷，不改变普通 Gameplay。

# Build Result

**Succeeded — UE 5.8.2 / SteppeEditor Win64 Development。**

UHT、C++、UMG 编译与链接成功。没有修改引擎，也没有增加物理绳、自动锁定、随机散布或骨骼资源依赖。

# Validation

自动测试：**16 passed, 0 failed**，其中 1 项包含既有 RiderSeat 占位警告。

新增 `Steppe.P12.SwingTimingAndHitZones`，覆盖抢投与稳定出手的 Stability、环口和射程差异，稳定性锁定，真实 UWorld Neck 附着，三类区域边界，以及区域对张力和压制时间的影响。P1–P11 回归继续通过。

实际渲染烟测显示 `SWING 38% | OPEN 81% | THROW` 和绿色准星；稳定出手记录 `Stability=1.00`、`Zone=Neck`、`Radius=80.0`、`Range=2600.0`，命中后 HUD 持续显示 NECK。完整捕获到命名路线继续以 Trial Success 结束。

证据：`Validation/P12-Results.json`、`P12-Swing.png`、`P12-NeckHit.png`、`P12-FullLoop.png` 和 `P12-Runs.txt`。原始日志为 `Saved/Logs/P12-FinalAutomation.log`、`P12-LassoSkillRender.log` 与 `P12-FullLoopRegression.log`。

# Manual Steps

打开 `Steppe.uproject` → Play。完成目标切出后按住 RMB，观察右上角 OPEN 条：未变绿时 LMB 抢投的环口和射程较小；变绿并提示 THROW 时出手获得完整参数。用准星高度尝试 Head、Neck、Torso，命中后从右上角区域标签和张力/控制速度比较结果。其余流程保持 Space 稳绳、C 捕获、E 下马/接触/牵行、营地命名，F2 重玩。

# Known Limits

- 当前摆绳用 HUD 节奏表达，尚无手臂、绳圈动画、正式绳索模型和音效。
- 命中区基于灰盒 Capsule 的局部高度；换成骨骼马后应改用独立碰撞体或骨骼映射。
- Stability 改变有效环口、速度和射程，但尚未经过大量人工试玩确定最终周期和倍率。
- P12 第二增量的 Rider Balance、侧向拉力、Stumble/Fall 和短距离 Dragged 尚未实现。
- 当前 120 秒挑战尚未达到 6–10 分钟目标节奏，也未完成多人次人工试玩指标采集。
- 仅验证 Editor Development 和 Editor -game，没有验证 Shipping 打包或其他平台。

# Next Recommended Work

继续 P12 第二增量。先建立 Rider Balance 的确定性输入：侧向绳力、速度、命中区与马匹 Strength 共同累积失衡；进入危险区后提供明确预警，超过阈值触发可恢复的 Stumble/Fall。随后增加有严格时长和主动松手出口的 Dragged 状态。

# Milestones

- 2026-09-11 P1：用户试玩并确认骑乘基础基本可用。
- 2026-09-12 P2–P3.1：完成单匹野马、慢速退让、五匹马群、个体时序、动态避让与脱困。
- 2026-09-12 P4–P7：完成视线选马、目标切出、套索投掷、绳索对抗与捕获登记。
- 2026-09-13 P8–P8.2：完成限时任务、引导、紧迫提示、成功/失败、计分与重玩。
- 2026-09-13 P9–P10：完成捕获后安全接近、牵回营地、Horse Card、本轮命名和完整结算。
- 2026-09-13 P11：完成 Fast、Strong、Nervous 三类原型；15 项测试和成功/失败回归通过。
- 2026-09-13 P12.1：完成确定性摆绳稳定窗口、投掷参数和三类命中区域；16 项测试、双画面烟测和完整闭环回归通过。
