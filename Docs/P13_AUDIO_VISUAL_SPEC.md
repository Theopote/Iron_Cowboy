# P13 垂直切片音画规格

**状态：前两次增量已完成并验证**
**依赖：P12**
**当前范围：反馈信号层、双地表路由、可替换 Sound/Niagara 插槽、程序化回退、马蹄/尘土、绳索风险与 Horse Card 层级**

## 目标

让速度、体力、套索阶段、张力和失衡风险在不查看底层调试数值时仍有可感知反馈。表现资源不得反向决定 Gameplay 结果，缺失或替换音画资源时现有测试结果保持不变。

## 反馈信号层

Rider 持有 SteppeFeedbackComponent，在 Gameplay 组件之后读取：

- 坐骑实际速度、Gait 和 Stamina，派生 HoofbeatInterval、WindIntensity、BreathIntensity。
- Lasso State、SwingPhase 和 Tension，派生 Swing、Throw、Attach、Release/Break、Control、Capture 与 RopeDanger 事件。
- RiderBalance State，派生 BalanceWarning、Fall 和 Dragged 事件。

组件只输出归一化信号、事件计数和最后事件，不修改 HorseMovement、Lasso、RiderBalance、Trial 或 Trust。

## 第一增量表现

- Gait 决定马蹄节奏：Walk 慢，Trot/Canter/Gallop/Sprint 逐步加快。
- 实际速度驱动 Wind；低 Stamina 与 Sprint 提高 Breath。
- 马蹄脉冲在坐骑后方生成短寿命灰盒尘土标记。
- Gameplay 绳线根据张力显示黄、绿、橙、红，并随风险增粗。
- 关键事件播放短程序化 PCM 占位音；没有外部音频资产依赖。
- HUD 暂时显示 FEEDBACK 信号与最近事件，供第一轮调参与自动烟测；最终表现成熟后移除数值。

## 验收

1. 同一坐骑速度下，Sprint/Gallop 的马蹄间隔短于 Walk。
2. Wind 随实际速度增长，Breath 随体力下降或 Sprint 增长。
3. Aiming 稳定窗口、Throw、Attach、Break/Release 和高张力产生独立事件。
4. Warning、Fall、Dragged 产生独立事件。
5. 关闭音频或在测试 World 禁止音频时，信号与事件仍确定性工作。
6. 实际渲染能同时看到运动反馈、尘土与动态绳索颜色；完整 P1–P12 闭环继续通过。

## 非目标

第一增量不引入第三方音频、正式音乐、语音、真实马动画、骨骼 IK、Niagara 生产粒子、地表材质分类或混音母线。草地/硬地差异、正式资源和 Horse Card/结算 UI 层级在后续增量处理。

## 验证结果

- UE 5.8.2 Development 构建成功；新增 `Steppe.P13.FeedbackSignalsAndEvents`，P1–P13.1 共 **18 passed、0 failed**，3 项因覆盖骑乘路径而记录既有 RiderSeat 灰盒回退警告。
- 渲染冒烟记录 `Hoofbeats=9 / LassoEvents=3 / RiskEvents=0 / Wind=0.31 / Breath=0.14 / Rope=0.00 / Last=Hoofbeat`；截图时套索保持 Neck 附着，画面同步显示绳线、FEEDBACK 行与运动脉冲。
- P10 完整闭环在反馈层接入后再次通过：目标捕获、第一次接触、牵回、Horse Card、命名和 Trial Success 均保持有效。
- 证据位于 `Validation/P13-Results.json`、`P13-Feedback.png`、`P13-FullLoop.png` 和 `P13-Runs.txt`；原始日志位于 `Saved/Logs/P13-Tests.log`、`P13-Feedback-Smoke.log` 与 `P13-FullLoop-Smoke.log`。

## P13.2 后续范围

P13.2 已完成表现资源接入基础：

- `SurfaceType1=Grass`、`SurfaceType2=Hard`；反馈组件以 5 Hz 向坐骑脚下探测物理材质。
- Grass 与 Hard 使用不同步频、程序化音色和扬尘强度；原型地图包含 `PM_Grass`、`PM_Hard` 与使用对应材质的可骑乘 `HardSurface_TestPad`。
- `FSteppeFeedbackAssets` 暴露草地/硬地马蹄、呼吸、套索、控制、捕获、Balance、落马声音及两类 HoofDust Niagara 插槽。配置正式资源时优先播放资源，缺失时使用现有程序化声音或灰盒尘土。
- BreathIntensity 超过阈值后按疲劳强度产生独立呼吸节奏事件。
- Horse Card 调整为 Identity、Temperament、Capability、Name 四层，并提供 Confirm 与 Replay 明确操作。

P13.2 验证保持 **18 passed、0 failed**。实际冒烟先记录并显示 `Surface=Grass`，随后进入绑定 `PM_Hard` 的测试垫并记录 `Surface=Hard`；完整牵回、Horse Card、命名和 Trial Success 再次通过。证据位于 `Validation/P13.2-Results.json`、`P13.2-Grass.png`、`P13.2-Hard.png`、`P13.2-HorseCard.png`、`P13.2-FullLoop.png` 和 `P13.2-Runs.txt`。

## P13.3 后续范围

选择许可明确的正式声音和 Niagara 资源填入现有插槽；接入与真实骨架匹配的马匹/骑手 AnimBP，并整理任务提示和结算 UI。所有表现仍只读取现有 Gameplay 状态。
