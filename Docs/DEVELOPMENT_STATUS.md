# Current Phase

**P13.1 — 反馈信号、程序化占位音、马蹄/尘土与绳索风险表现已经实现并验证。**

项目使用 UE 5.8.2。完整灰盒闭环继续可玩；Gameplay 之外新增统一反馈组件，把速度、体力、摆绳、张力和骑手风险转换为可替换的音画信号。

# Completed

- P1–P11 的骑乘、五匹马群、三类马匹原型、目标切出、安全接近、牵回、Horse Card 和命名闭环保持有效。
- RMB Aiming 以 1.2 秒周期输出 SwingPhase 和 SwingStability；LMB 出手锁定 Stability，并决定本次有效环口、速度与射程。
- 连续扫掠命中按目标局部高度划分 Head、Neck、Torso，区域倍率进入张力、压制时间和骑手失衡负荷。
- RiderBalanceComponent 在 PostPhysics 读取张力、绳索侧向比例、坐骑速度、目标 Strength 和命中区，低风险时自动恢复。
- Balance 超过 55% 进入 Warning；满值后事故脱离坐骑，恢复 Rider 碰撞并施加确定性 Falling 速度。
- 落马后若套索仍附着且目标在 18 米内，会进入最长 1.25 秒 Dragged；CharacterMovement 处理受限拖行速度。
- LMB 可立即松绳结束拖行；超时也会自动松绳。Recovering 结束后 Rider 保留并可移动、重新上马。
- HUD 显示 SWING、OPEN、命中区、张力、CONTROL、BALANCE、SIDE 与 DRAGGED 提示；状态同时通过 Native Gameplay Tags 暴露。
- SteppeFeedbackComponent 在 Gameplay Tick 之后读取坐骑速度/Gait/Stamina、Lasso 与 RiderBalance，输出马蹄节拍、Wind、Breath、RopeStress 和离散事件。
- Gait 驱动马蹄节拍，速度与低体力驱动风感和呼吸；马蹄脉冲生成灰盒扬尘。
- 摆绳、投掷、附着、释放/断裂、控制完成、捕获和失衡均生成独立事件；程序化 PCM 提供无外部授权依赖的临时声音。
- 绳线按有效张力和危险程度显示黄、绿、橙、红并动态增粗；临时 FEEDBACK HUD 用于调参与验证。

# Build Result

**Succeeded — UE 5.8.2 / SteppeEditor Win64 Development。**

UHT、C++、UMG 编译与链接成功。没有修改引擎，也没有增加物理绳、随机散布、自动锁定、生命值、布娃娃或骨骼资源依赖。

# Validation

自动测试：**18 passed, 0 failed**。其中 3 项因覆盖正常骑乘和事故落马而记录既有 RiderSeat 灰盒回退警告。

`Steppe.P13.FeedbackSignalsAndEvents` 覆盖步态节拍、疲劳呼吸、骑乘风感、扬尘脉冲、套索事件、风险事件和禁用音频时的确定性信号。P1–P12 回归继续通过。

实际渲染记录 `Hoofbeats=9 / LassoEvents=3 / RiskEvents=0 / Wind=0.31 / Breath=0.14 / Rope=0.00 / Last=Hoofbeat`，截图时套索保持 Neck 附着。正向完整路线仍将 `Saran` 牵回、命名并以 Trial Success 结束。

证据：`Validation/P13-Results.json`、`P13-Feedback.png`、`P13-FullLoop.png` 和 `P13-Runs.txt`。原始日志为 `Saved/Logs/P13-Tests.log`、`P13-Feedback-Smoke.log` 与 `P13-FullLoop-Smoke.log`。

# Manual Steps

完成目标切出后按住 RMB，观察 OPEN 条，在绿色 THROW 窗口用 LMB 出手。调整准星高度尝试 Head、Neck、Torso。附着后按 Space 控绳，同时观察 BALANCE 与 SIDE：高 SIDE 时转向绳索方向或减速；持续高负荷会落马并显示 DRAGGED，此时 LMB 松绳。恢复后可以继续步行和重新上马。F2 重玩。

# Known Limits

- 摆绳、落马和拖行目前由状态、速度、HUD、程序化占位音和灰盒模型表达，尚无正式角色动画、布娃娃或绳圈模型。
- 尚未按草地/硬地区分步音；尘土尚未迁移到 Niagara，风感和呼吸也未接入正式循环素材与混音。
- 命中区基于灰盒 Capsule 局部高度；换成骨骼马后应改用独立碰撞体或骨骼映射。
- Balance 周期、阈值、拖行速度和 1.25 秒上限尚未经过多人试玩平衡。
- 没有生命值或伤害；落马是可恢复的操作后果。
- 当前 120 秒挑战尚未达到 6–10 分钟目标节奏，也未完成多人次人工试玩指标采集。
- 仅验证 Editor Development 和 Editor -game，没有验证 Shipping 打包或其他平台。

# Next Recommended Work

继续 P13.2：接入许可明确、可替换的正式马蹄/呼吸/绳索/环境声音和地表分类，把扬尘迁移到 Niagara，接入基础马与骑手动画接口，并整理 Horse Card、任务提示和结算 UI 层级。

# Milestones

- 2026-09-11 P1：用户试玩并确认骑乘基础基本可用。
- 2026-09-12 P2–P3.1：完成单匹野马、慢速退让、五匹马群、个体时序、动态避让与脱困。
- 2026-09-12 P4–P7：完成视线选马、目标切出、套索投掷、绳索对抗与捕获登记。
- 2026-09-13 P8–P10：完成任务结算、捕获后安全接近、牵回营地、Horse Card 和本轮命名。
- 2026-09-13 P11：完成 Fast、Strong、Nervous 三类原型；15 项测试和成功/失败回归通过。
- 2026-09-13 P12.1：完成确定性摆绳稳定窗口、投掷参数和三类命中区域。
- 2026-09-14 P12.2：完成 Rider Balance、侧向负荷、落马、限时拖行和主动松绳恢复；17 项测试、两条 P12 渲染路线和完整闭环回归通过。
- 2026-09-14 P13.1：完成独立反馈层、程序化占位音、马蹄/风感/呼吸、灰盒扬尘和动态绳索风险表现；18 项测试、P13 渲染和完整闭环回归通过。
