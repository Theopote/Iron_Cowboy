# Current Phase

**P10 — 牵回营地、Horse Card 与本轮命名已经实现并完成实际渲染验证。**

项目使用 UE 5.8.2。当前已经具备“骑乘 → 马群 → 切出 → 投索 → 控绳 → 捕获 → 安全接近 → 第一次接触 → 牵回营地 → 命名”的完整灰盒闭环。

# Completed

- 骑手输入经 RidingIntent 驱动坐骑，具备渐进加减速、速度相关转向、步态、体力、独立观察和安全上下马。
- 五匹 WildHorse Actor 各自运行感知、状态机、局部避障和 CMC；HerdManager 汇总邻居、分离、群体方向与延迟警报。
- Q 选择目标并通过 18 米/2 秒条件切出；RMB/LMB 投索，空格稳绳，Subdued 后按 C 登记捕获。
- 捕获后必须下马、慢速接近并平静停留；冲入会触发可恢复的拒绝，ReadyForContact 时按 E 完成第一次接触。
- 第一次接触后再按 E 进入 Leading。HorseBrain 跟随骑手后方锚点，并通过既有地面/障碍探测向 HorseMovement 提交 Walk 或 Brake 意图。
- 出生点附近生成可见的 `CAMP / PEN` 围栏；只有骑手和当前牵行目标同时进入区域才完成交付。
- 交付后显示原生 UMG Horse Card，展示编号、性别/年龄占位、毛色、性格以及速度、耐力、力量和敏捷。
- 命名框接受 1–16 个字符，支持 Enter 或按钮确认；UI 接管输入焦点，F2 可以重玩。
- Trial 分别记录 Secure、Contact、Deliver、Name，计时持续到命名后才结算成功。
- F2 重载关卡，清理骑手、坐骑、马群、牵行、交付、名字和命名界面。

# Build Result

**Succeeded — UE 5.8.2 / SteppeEditor Win64 Development。**

UHT、C++、UMG 编译与链接成功。继续使用 NoPCHs / -NoUBA 本地构建设置；没有修改引擎，也没有增加 NavigationSystem、Mass、GAS、绳索物理或 SaveGame。

# Validation

自动测试：**14 passed, 0 failed**，其中 1 项包含既有的 RiderSeat 占位警告。

新增 `Steppe.P10.LeadDeliveryAndNaming`，覆盖第一次接触前禁止牵行、有效牵行、双 Actor 营地判定、空名字拒绝、名字保存、唯一计数和命名结算。P8 任务规则更新为 Named 后成功；P1–P9 回归继续通过。

实际渲染完整链路记录 H1 通过 HorseMovement 自行移动 428.3 cm 后进入营地，显示 Horse Card，命名为 `Saran`，任务以 2080 分 Success。P9 兼容路线确认第一次接触后任务仍为 Running；独立超时失败路线继续通过。

证据：`Validation/P10-Results.json`、`P10-Lead.png`、`P10-HorseCard.png`、`P10-Named.png` 和 `P10-Runs.txt`。原始日志为 `Saved/Logs/P10-FinalAutomation.log`、`P10-FinalRender.log`、`P10-P9Regression.log` 与 `P10-FailureRegression.log`。

# Manual Steps

打开 `Steppe.uproject` → Play。在 120 秒内完成 Q 选择、18 米隔离、RMB/LMB 投掷和空格稳绳。CONTROL 100% 后按 C，减速并按 E 下马；缓慢走到目标约 2.2 米内，停留至 CALM 100%，按 E 完成第一次接触，再按 E 拿起牵绳。根据 HUD 的距离提示返回 `CAMP / PEN`，等待目标进入围栏，在 Horse Card 输入名字并按 Enter。F2 重玩。

# Known Limits

- 牵行目前只支持一匹、徒步和抽象牵引锚点；没有正式牵绳模型、物理绳、骑马牵行或复杂封闭空间寻路。
- Horse Card 的身份描述是确定性的灰盒占位；能力值读取真实属性，但 P11 尚未让个体差异充分影响全部 Gameplay。
- 名字只保存在当前关卡生命周期；F2 或退出游戏后清除，没有 SaveGame、马厩容量或经济系统。
- 仍是灰盒占位模型，没有真实马动画、听觉、鸣叫、尘土或最终美术。
- 当前 120 秒挑战尚未达到 6–10 分钟目标节奏，也未完成多人次人工试玩指标采集。
- 仅验证 Editor Development 和 Editor -game，没有验证 Shipping 打包或其他平台。

# Next Recommended Work

按照 `DEVELOPMENT_ROADMAP.md` 进入 P11：建立 Fast、Strong、Nervous 等可感知原型，让 Speed、Stamina、Fear、Strength、Agility 真实影响运动、威胁、张力和接近，并让 Horse Card 只展示玩家能在行为中验证的差异。

# Milestones

- 2026-09-11 P1：用户试玩并确认骑乘基础基本可用。
- 2026-09-12 P2–P3.1：完成单匹野马、慢速退让、五匹马群、个体时序、动态避让与脱困。
- 2026-09-12 P4–P7：完成视线选马、目标切出、套索投掷、绳索对抗与捕获登记。
- 2026-09-13 P8–P8.2：完成限时任务、引导、紧迫提示、成功/失败、计分与重玩。
- 2026-09-13 P9：完成安全接近、拒绝恢复、第一次接触、最低 Trust 与延后结算；13 项测试通过。
- 2026-09-13 P10：完成牵行、营地交付、Horse Card、本轮命名和 Named 后结算；14 项测试及完整渲染链路通过。
