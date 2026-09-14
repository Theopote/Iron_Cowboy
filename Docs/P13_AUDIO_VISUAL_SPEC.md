# P13 垂直切片音画规格

**状态：第一增量已完成并验证**
**依赖：P12**
**当前范围：反馈信号层、程序化占位音、马蹄/尘土、绳索受力与风险反馈**

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

接入可替换且许可明确的真实马蹄、呼吸、绳索与环境声音，按地表材质区分步音；把灰盒尘土迁移到 Niagara，把状态驱动接入马与骑手动画接口，并整理 Horse Card、任务提示和结算 UI 层级。所有表现仍只读取现有 Gameplay 状态。
