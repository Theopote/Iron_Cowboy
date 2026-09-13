# Current Phase

**P11 — Fast、Strong、Nervous 三类可感知马匹原型已经实现并完成实际渲染验证。**

项目使用 UE 5.8.2。当前已经具备“骑乘 → 马群 → 切出 → 投索 → 控绳 → 捕获 → 安全接近 → 第一次接触 → 牵回营地 → 命名”的完整灰盒闭环，并让野马类型真实影响追逐、受惊、控绳和接近。

# Completed

- 骑手输入经 RidingIntent 驱动坐骑，具备渐进加减速、速度相关转向、步态、体力、独立观察和安全上下马。
- 五匹 WildHorse Actor 各自运行感知、状态机、局部避障和 CMC；HerdManager 汇总邻居、分离、群体方向与延迟警报。
- Q 选择目标并通过 18 米/2 秒条件切出；RMB/LMB 投索，空格稳绳，Subdued 后按 C 登记捕获。
- 捕获后必须下马、慢速接近并平静停留；冲入会触发可恢复的拒绝，ReadyForContact 时按 E 完成第一次接触。
- 第一次接触后再按 E 进入 Leading；目标通过 HorseMovement 跟随骑手，双方进入营地后显示 Horse Card 并完成命名。
- Fast 具有更高极速、加速与敏捷；Strong 具有更高体力、力量与控制抗性；Nervous 更快受惊、恢复更慢且要求更慢、更久的安全接近。
- 三类 Profile 按马群成员编号确定性分配，集中保存在 HerdManager 可编辑数组中，应用入口幂等。
- Profile 已接入 HorseAttribute、HorseBrain、LassoComponent 和 HorseTrust；Horse Card 显示实际属性，F1 调试标签显示类型，灰盒材质提供辅助识别色。
- Trial 分别记录 Secure、Contact、Deliver、Name，计时持续到命名后才结算成功；F2 重载完整重建本轮。

# Build Result

**Succeeded — UE 5.8.2 / SteppeEditor Win64 Development。**

UHT、C++、UMG 编译与链接成功。继续使用 NoPCHs / -NoUBA 本地构建设置；没有修改引擎，也没有增加 NavigationSystem、Mass、GAS、绳索物理或 SaveGame。

# Validation

自动测试：**15 passed, 0 failed**，其中 1 项包含既有的 RiderSeat 占位警告。

新增 `Steppe.P11.ArchetypeGameplayDifferences`，覆盖确定性分配、Fast 与 Strong 的运动差异、Strong 的体力/力量/压制时间、Nervous 的受惊和恢复速度、安全接近阈值、平静停留时间，以及重复应用不叠乘。P1–P10 回归继续通过。

实际渲染烟测同时生成 H1 Fast、H2 Strong、H3 Nervous，并记录三者的速度、力量、恐惧增长/衰减、安全接近速度和安抚时间。完整成功链路以 `Saran` 命名结算，目标通过 HorseMovement 移动 416.2 cm；独立超时失败路线继续通过。

证据：`Validation/P11-Results.json`、`P11-Archetypes.png`、`P11-FullLoop.png` 和 `P11-Runs.txt`。原始日志为 `Saved/Logs/P11-FinalAutomation.log`、`P11-ArchetypeRender.log`、`P11-FullLoopRegression.log` 与 `P11-FailureRegression.log`。

# Manual Steps

打开 `Steppe.uproject` → Play。按 F1 可观察 `H# [Fast/Strong/Nervous]` 标签，并在相似路线下比较追逐速度与受惊反应。在 120 秒内完成 Q 选择、18 米隔离、RMB/LMB 投掷和空格稳绳；CONTROL 100% 后按 C，减速并按 E 下马，缓慢走近并停留至 CALM 100%，按 E 接触后再按一次 E 牵行。返回 `CAMP / PEN`，在 Horse Card 输入名字并按 Enter。F2 重玩。

# Known Limits

- 三类原型已有确定性 Gameplay 差异，但尚未完成关闭 F1 后的多人盲测和最终平衡。
- 灰盒颜色只是开发期辅助；没有正式毛色材质、真实马模型、动画、听觉、鸣叫、尘土或最终美术。
- 牵行目前只支持一匹、徒步和抽象牵引锚点；没有正式牵绳模型、物理绳、骑马牵行或复杂封闭空间寻路。
- 名字只保存在当前关卡生命周期；F2 或退出游戏后清除，没有 SaveGame、马厩容量或经济系统。
- 当前 120 秒挑战尚未达到 6–10 分钟目标节奏，也未完成多人次人工试玩指标采集。
- 仅验证 Editor Development 和 Editor -game，没有验证 Shipping 打包或其他平台。

# Next Recommended Work

按照 `DEVELOPMENT_ROADMAP.md` 进入 P12：先增加可练习的摆绳/准备节奏与简化命中区域，再实现 Rider Balance、侧向拉力、落马和可主动结束的短距离拖行。开始实现前先冻结确定性阈值、恢复路线和本轮非目标。

# Milestones

- 2026-09-11 P1：用户试玩并确认骑乘基础基本可用。
- 2026-09-12 P2–P3.1：完成单匹野马、慢速退让、五匹马群、个体时序、动态避让与脱困。
- 2026-09-12 P4–P7：完成视线选马、目标切出、套索投掷、绳索对抗与捕获登记。
- 2026-09-13 P8–P8.2：完成限时任务、引导、紧迫提示、成功/失败、计分与重玩。
- 2026-09-13 P9：完成安全接近、拒绝恢复、第一次接触、最低 Trust 与延后结算；13 项测试通过。
- 2026-09-13 P10：完成牵行、营地交付、Horse Card、本轮命名和 Named 后结算；14 项测试及完整渲染链路通过。
- 2026-09-13 P11：完成三类确定性马匹原型及运动、恐惧、控绳、接近差异；15 项测试、原型同屏渲染和成功/失败回归通过。
