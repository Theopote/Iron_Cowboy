# Current Phase

**P13.3 — 马匹/骑手动画表现数据、灰盒姿态与正式结算层级已经实现并验证。**

项目使用 UE 5.8.2。完整灰盒闭环继续可玩；Sound、Niagara 和 AnimBP 所需的数据边界已经建立，灰盒马匹与骑手会实际表现步态起伏、加速俯仰、转向侧倾和受力姿态。

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
- PhysicsSettings 登记 Grass/Hard；`PM_Grass`、`PM_Hard` 已创建并绑定原型材质，地图加入使用硬地材质的 HardSurface_TestPad。
- FSteppeFeedbackAssets 提供 14 类 Sound 和两类 Niagara 插槽；Grass/Hard 具有不同步频、音色与扬尘强度。
- 高呼吸强度会产生独立 HorseBreath 节奏事件，正式循环/单次素材可直接替换程序化回退。
- Horse Card 形成 Identity、Temperament、Capability、Name 四层，并增加明确的 Confirm Name 与 Replay Round 操作。
- HorsePresentationComponent 输出 GaitPhase、StrideBlend、BodyBob/Pitch/Roll 和 FootContactPulse，供灰盒与未来 AnimBP 共用。
- RiderPresentationData 输出骑乘、稳绳、落马、拖行、Balance、侧向拉力和身体姿态，正式 Rider AnimBP 可直接读取。
- 命名成功后 Horse Card 自动收起并恢复游戏输入；最终结算显示四阶段统计、积分和 Replay。

# Build Result

**Succeeded — UE 5.8.2 / SteppeEditor Win64 Development。**

UHT、C++、UMG 编译与链接成功。没有修改引擎，也没有增加物理绳、随机散布、自动锁定、生命值、布娃娃或骨骼资源依赖。

# Validation

自动测试：**18 passed, 0 failed**。其中 3 项因覆盖正常骑乘和事故落马而记录既有 RiderSeat 灰盒回退警告。

`Steppe.P13.FeedbackSignalsAndEvents` 覆盖步态节拍、疲劳呼吸、骑乘风感、扬尘脉冲、套索事件、风险事件和禁用音频时的确定性信号。P1–P12 回归继续通过。

P13.3 侧视冒烟记录 `Sprint / Phase=0.45 / Stride=0.92 / Bob=-1.5 / HorseRoll=-6.0 / RiderRoll=-4.7 / Mounted=1`。正向完整路线仍将 `Saran` 牵回 416.2 cm，通过新版 Horse Card 命名，自动收卡并以无遮挡 Trial Success 结算结束。

证据：`Validation/P13.3-Results.json`、`P13.3-Presentation.png`、`P13.3-HorseCard.png`、`P13.3-Settlement.png` 和 `P13.3-Runs.txt`。原始日志为 `Saved/Logs/P13.3-Tests.log`、`P13.3-Presentation-Smoke.log` 与 `P13.3-FullLoop-Smoke.log`。

# Manual Steps

完成目标切出后按住 RMB，观察 OPEN 条，在绿色 THROW 窗口用 LMB 出手。调整准星高度尝试 Head、Neck、Torso。附着后按 Space 控绳，同时观察 BALANCE 与 SIDE：高 SIDE 时转向绳索方向或减速；持续高负荷会落马并显示 DRAGGED，此时 LMB 松绳。恢复后可以继续步行和重新上马。F2 重玩。

# Known Limits

- 摆绳、落马和拖行目前由状态、速度、HUD、程序化占位音和灰盒模型表达，尚无正式角色动画、布娃娃或绳圈模型。
- 草地/硬地路由已完成，但正式录音和 Niagara 资产尚未选定；当前仍由程序化音色和灰盒尘土回退。
- 命中区基于灰盒 Capsule 局部高度；换成骨骼马后应改用独立碰撞体或骨骼映射。
- Balance 周期、阈值、拖行速度和 1.25 秒上限尚未经过多人试玩平衡。
- 没有生命值或伤害；落马是可恢复的操作后果。
- 当前 120 秒挑战尚未达到 6–10 分钟目标节奏，也未完成多人次人工试玩指标采集。
- 仅验证 Editor Development 和 Editor -game，没有验证 Shipping 打包或其他平台。

# Next Recommended Work

进入 P14 原型验证准备：记录每轮选择、失误、捕获耗时、Balance 风险、失败原因与重试；先用当前可复现灰盒完成内部多轮试玩。正式声音、Niagara 与骨架资源可在许可确定后沿现有接口并行替换。

# Milestones

- 2026-09-11 P1：用户试玩并确认骑乘基础基本可用。
- 2026-09-12 P2–P3.1：完成单匹野马、慢速退让、五匹马群、个体时序、动态避让与脱困。
- 2026-09-12 P4–P7：完成视线选马、目标切出、套索投掷、绳索对抗与捕获登记。
- 2026-09-13 P8–P10：完成任务结算、捕获后安全接近、牵回营地、Horse Card 和本轮命名。
- 2026-09-13 P11：完成 Fast、Strong、Nervous 三类原型；15 项测试和成功/失败回归通过。
- 2026-09-13 P12.1：完成确定性摆绳稳定窗口、投掷参数和三类命中区域。
- 2026-09-14 P12.2：完成 Rider Balance、侧向负荷、落马、限时拖行和主动松绳恢复；17 项测试、两条 P12 渲染路线和完整闭环回归通过。
- 2026-09-14 P13.1：完成独立反馈层、程序化占位音、马蹄/风感/呼吸、灰盒扬尘和动态绳索风险表现；18 项测试、P13 渲染和完整闭环回归通过。
- 2026-09-14 P13.2：完成 Grass/Hard 实际物理材质路由、Sound/Niagara 资源插槽、呼吸事件和 Horse Card 层级；18 项测试、双地表渲染和完整闭环回归通过。
- 2026-09-14 P13.3：完成连续步态相位、马匹/骑手姿态数据、灰盒表现与无重叠结算；18 项测试、侧视表现和完整闭环回归通过。
