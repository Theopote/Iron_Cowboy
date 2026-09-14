# Current Phase

**P14.4 — 真人试玩已开始；第一项 Core Feel Repair 聚焦套索可见性、Q 目标选择与操作提示层级。**

首批真人试玩发现玩家看不到准备中的套索、不清楚何时按 Q，且原有底部提示容易被忽略。当前已完成针对性修复：准星中的候选马显示黄色 `Q SELECT` 框，选中目标后显示青色持续标记；屏幕中央新增按阶段变化的大型操作卡；按住右键时绘制清晰的套索环，稳定窗口由橙色变为绿色，并明确提示此时按左键投掷。目标选择、隔离、Sphere Sweep 和套索动力学规则未改变。

第二批反馈进一步发现目标框不会随距离失效、18 米切群门槛与 26 米套索射程容易错位、未隔离时完全没有瞄准反馈，以及套中后的断绳/失衡/拖拽/平静过程不直观。当前修复将 Q 选择/丢失距离改为 35/42 米，将切群门槛改为 10 米、保持 1 秒；未隔离时允许显示灰色瞄准环但禁止投掷，超射程显示红环；套中后用中央双进度条和状态卡解释张力与平静过程，拖拽窗口延长到 2 秒，失败结果显示 1.5 秒。

第三批反馈指出普通高张力仍过于容易断绳，且落马或主动下马后缺少持续牵引。当前已把断绳改为高速分离过程中玩家端突然急减速所产生的冲击风险；普通持续拉力会先传给骑手。野马会在被套后继续向外挣扎，并随控绳进度逐渐降速；骑乘时冲击和侧拉可将玩家拉下马，倒地拖行结束后玩家会站起并继续握绳，徒步跟跑、被拉行和按 Space 控绳均可持续，只有玩家按 LMB、极端距离保护或真正的急停冲击会释放绳索。

本轮验证已完成：Editor 与 Shipping 编译成功，19 项自动化测试全部通过；套索控制、拖拽警告和完整捕获到命名路线均通过渲染 Smoke，新版分发包完成启动检查。

验证状态：UE 5.8.2 Editor 编译成功；19 项 Steppe 自动化测试通过，0 项失败；渲染 Smoke 验证稳定窗口套索环、目标标记和中央操作卡均可见。下一批真人样本重点记录首轮 Q 选择时间、首次投掷时间和是否仍需口头帮助。

项目使用 UE 5.8.2。完整灰盒闭环继续可玩；Sound、Niagara 和 AnimBP 所需的数据边界已经建立，灰盒马匹与骑手会实际表现步态起伏、加速俯仰、转向侧倾和受力姿态。

# Completed

- P1–P11 的骑乘、五匹马群、三类马匹原型、目标切出、安全接近、牵回、Horse Card 和命名闭环保持有效。
- RMB Aiming 以 1.2 秒周期输出 SwingPhase 和 SwingStability；LMB 出手锁定 Stability，并决定本次有效环口、速度与射程。
- 连续扫掠命中按目标局部高度划分 Head、Neck、Torso，区域倍率进入张力、压制时间和骑手失衡负荷。
- RiderBalanceComponent 在 PostPhysics 读取张力、绳索侧向比例、坐骑速度、目标 Strength 和命中区，低风险时自动恢复。
- Balance 超过 55% 进入 Warning；满值后事故脱离坐骑，恢复 Rider 碰撞并施加确定性 Falling 速度。
- 落马后若套索仍附着且目标在 18 米内，会进入最长 2 秒 Dragged；结束后切换到 Pulled，玩家站起、继续握绳并可徒步跟跑。
- 普通持续高张力不再自动断绳；野马拉力会传给坐骑或徒步玩家。高速分离时玩家端突然急减速才会累积断绳冲击，极端距离仅作为安全保护。
- LMB 可随时主动松绳；Dragged 和 Pulled 期间按住 Space 仍会推进平静度，直至控制野马或玩家主动放弃。
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
- PlaytestMetrics 按轮记录目标类型、阶段耗时、投索/命中/脱靶/断绳、危险张力、Balance 事故、结果、积分与重试。
- 成功、超时、F2 重试和世界结束都会把 UTF-8 JSON 写入 `Saved/Playtests/`；状态跃迁计数避免按帧重复。
- 原长方体马已改为 16 个低成本基础几何部件：躯干、胸部、斜颈、头、口鼻、双耳、尾巴、四腿与四蹄。
- 全马共用不缩放的表现根节点，继续响应已有 BodyBob/Pitch/Roll；四腿使用肩/胯枢轴按对角步态交替摆动，蹄部随腿运动。
- 野马全部部件继承原 Blueprint 躯干材质；视觉组件无碰撞，原 Capsule、移动、AI、套索命中和骑乘规则保持不变。
- 已核实当前套索是直线 Sphere Sweep 且只附着 Q 目标，马匹速度每帧对齐 Heading；两项分别冻结到 P15 和 P16，避免在人工基线前改变规则。
- `Docs/Playtests/` 提供 15 轮匿名记录表、访谈模板和现场说明；`SummarizePlaytests.ps1` 汇总客观 JSON。
- 试玩 JSON 升级为 schema 1.1 并记录 `isAutomated`；真人汇总默认排除自动 Smoke 与旧版无来源记录。
- `.idea/`、`.vscode/` 已加入忽略；6 个 `.idea` 文件停止 Git 跟踪，本机副本保留。
- P14.3 玩家教程覆盖启动、完整操作、三轮流程、卡住恢复和数据说明；组织者指南规定教程协助标记、逐轮口述和 JSON 对应方法。
- `PackageWindows.ps1` 提供 UE 5.8.2 Win64 Shipping 的 Build/Cook/Stage/Pak/Archive 与可选 ZIP；分发包自动附带 PlaytestKit 和本地结果收集脚本。
- Win64 Shipping 已实际完成打包和启动验证：Cook 506 个包、0 error/0 warning；最新 ZIP 66 项、314,325,561 bytes，Shipping 进程退出码 0。

# Build Result

**Succeeded — UE 5.8.2 / SteppeEditor Win64 Development。**

UHT、C++、UMG 编译与链接成功。没有修改引擎，也没有增加物理绳、随机散布、自动锁定、生命值、布娃娃或骨骼资源依赖。

# Validation

自动测试：**19 passed, 0 failed**。其中 3 项因覆盖正常骑乘和事故落马而记录既有 RiderSeat 灰盒回退警告。

新增 `Steppe.P14.PlaytestMetricTransitions`，覆盖投索去重、附着区域、断绳分类和 Balance 事故跃迁。P1–P13 回归继续通过。

P14.2 在 P1 世界测试中增加 16 部件/4 腿结构断言；完整 19 项回归继续通过。专用侧视路线并排渲染 Fast、Strong、Nervous，确认头尾方向、四足落地和完整轮廓。

P14 成功路线记录 12.0 秒、1 次投索、1 次 Neck 附着、0 次脱靶/断绳和 2080 分；六个阶段时间均已写入。失败路线在 3.0 秒超时并记录 0 分。两条路线都生成可解析 JSON 和结算截图。

证据：`Validation/P14-Results.json`、`P14-Success-Metrics.json`、`P14-Failure-Metrics.json`、`P14-Success.png`、`P14-Failure.png` 和 `P14-Runs.txt`。原始日志为 `Saved/Logs/P14-Automation.log`、`P14-Success.log` 与 `P14-Failure.log`。

P14.2 模型证据：`Validation/P14.2-HorseModel.png`、`P14.2-Results.json` 和 `P14.2-Runs.txt`；原始日志为 `Saved/Logs/P14.2-Automation.log` 与 `P14.2-HorseModel.log`。

P14.3 准备证据：`Validation/P14.3-Audit-Results.json`、`P14.3-Automated-Metrics.json` 和 `P14.3-Audit-Runs.txt`；UE 5.8.2 构建成功，19 passed、0 failed。

# Manual Steps

完成目标切出后按住 RMB，观察 OPEN 条，在绿色 THROW 窗口用 LMB 出手。调整准星高度尝试 Head、Neck、Torso。附着后按 Space 控绳，同时观察 BALANCE 与 SIDE：保持与野马同向移动，避免在双方高速分离时突然刹停。高 SIDE 或强冲击可能把玩家拉下马；DRAGGED 结束后继续用移动键跟跑并按 Space 控绳，LMB 仅在决定放弃时松绳。F2 重玩。

# Known Limits

- 摆绳、落马和拖行目前由状态、速度、HUD、程序化占位音和灰盒模型表达，尚无正式角色动画、布娃娃或绳圈模型。
- 草地/硬地路由已完成，但正式录音和 Niagara 资产尚未选定；当前仍由程序化音色和灰盒尘土回退。
- 命中区基于灰盒 Capsule 局部高度；换成骨骼马后应改用独立碰撞体或骨骼映射。
- Balance、急停冲击阈值、2 秒倒地拖行和徒步牵引速度仍需更多真人样本平衡。
- 没有生命值或伤害；落马是可恢复的操作后果。
- 当前 120 秒挑战尚未达到 6–10 分钟目标节奏；自动指标已接入，但 5 人 × 3 轮人工样本尚未采集。
- 已验证 Windows Shipping；其他平台尚未构建或验证。

# Next Recommended Work

执行 P14.3 人工验证：按 `CORE_GAMEPLAY_AUDIT.md` 和 `Playtests/P14.3-README.md` 完成至少 5 人 × 3 轮，结合自动 JSON 和简短访谈分类 Blocker、Core Feel、Clarity、Polish。随后 P14.4 只修最多 3 个高频核心手感问题；P15/P16 前不扩展功能。

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
- 2026-09-14 P14.1：完成逐轮试玩指标、成功/失败 JSON、重试/退出收尾和人工验证规范；19 项测试及两条渲染路线通过。
- 2026-09-14 P14.2：将长方体马替换为带头颈、双耳、尾巴、四腿四蹄的组合式灰盒马，并接入对角腿部摆动；19 项测试及专用侧视渲染通过。
- 2026-09-14 P14.3 准备：完成核心玩法审计、P14.3–P17 阶段门、真人试玩执行包、来源隔离汇总与 IDE 跟踪清理；等待 15 轮人工样本。
- 2026-09-14 P14.4 绳索持续牵引：普通张力改为力传递，高速急停才产生断绳冲击；补齐被套野马持续挣扎、骑手落马、倒地拖行后站起及徒步握绳跟跑。
