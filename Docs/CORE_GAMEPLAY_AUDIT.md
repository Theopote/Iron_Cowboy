# 核心玩法审计与阶段门

**审计日期：2026-09-14**  
**适用基线：P14.2**

## 结论

项目暂停横向功能扩展。当前闭环已经足够支撑玩法验证，但最具辨识度的两部分——马的重量感和套索的空间物理感——仍是可运行的简化模型。下一步先完成真实人工试玩，再只修核心手感，之后才分别重做套索和马匹动力学。

**2026-09-15 更新：** 项目负责人依据累计真人反馈确认 P14.3 阶段门通过，P14.4 高频问题已经修复，P15 第一轮开始实施。数据完整性说明见 `P14.3_GAMEPLAY_RESEARCH_REVIEW.md`。

## 已核实的工程事实

### 套索仍是球形扫掠投射物

`LassoComponent` 用直线推进 `LoopLocation`，并以 `EffectiveCaptureRadius` 做 Sphere Sweep。Swing Stability 目前缩放捕获半径、速度和射程；没有独立的环平面、角相位、环半径变化、重力或抛物轨迹。

命中 Actor 必须等于 Q 选中的 `Target`，否则进入 blocked recovery。因此 Q 当前既是关注目标也是唯一允许附着的对象。该实现适合稳定原型验证，不代表最终套索设计。

### 朝向与速度尚未分离

`HorseMovementComponent::CalcVelocity()` 最终把速度写成 `Heading.Vector() * Speed`。加速度、制动、步态、体力、转向限速和响应延迟已经存在，但没有持续的侧向速度、抓地衰减或绳索外力叠加。

### 开发验证代码正在挤占 GameMode

`SteppeGameMode.cpp` 当前约 34 KB，包含生成、任务、指标、命令行开关、自动摆场、计时、截图和多条 Smoke choreography。先保留以维持现有回归；P14 人工验证结束后再把开发场景迁入独立 `SteppeDeveloper` 模块或开发专用 Runner。

### HorseBrain 已接近下一次拆分点

`HorseBrainComponent.cpp` 当前约 21 KB，同时承担感知、威胁、状态、群体引导、地形/障碍探测、被套、捕获、牵行和游荡。P14 阶段不重构。只有当核心规则经人工验证稳定后，才按 Perception、Threat、Navigation、Herd、Capture 职责拆分。

## 已确认的开发顺序

### P14.3 — 真实人工试玩

- 5 名测试者，每人连续 3 轮。
- 不口头提供完整解法；保留游戏内现有按键和反馈。
- 每轮关联自动 JSON，并记录失败理解、主动重玩、四段核心体验和马匹辨识。
- 达到 15 轮前，不开始 P15/P16，不扩大世界或长期系统。

### P14.4 — Core Feel Repair

只根据 P14.3 高频证据修复：Horse inertia、Horse turning、Chase feel、Target isolation、Lasso aiming clarity、Rope feedback。每项修改必须对应至少一个可复现的人工观察；不借此加入新系统。

### P15 — Physical Lasso v2

将套索表达拆为 Hand Anchor、Swing Plane、Loop Radius、Loop Angular Phase、Loop Center Trajectory 和 Loop Orientation。命中改为目标身体是否穿过绳圈的有效区域。

Q 只表达 Desired Target 和 HUD 关注。实际附着对象由几何结果决定；错误套中另一匹马应成为有效且可解释的结果。通用 `ILassoAttachable` 只在马匹版本验证后再考虑树、围栏、牛或马车。

**当前状态：第一轮已实现并进入试玩。** 采用抛物线中心轨迹、动态环半径、随速度更新的空间平面、角相位和三点马体几何命中；详细边界见 `P15_PHYSICAL_LASSO_V2.md`。

### P16 — Horse Dynamics v2

引入 Facing/Velocity 分离、Forward/Lateral Velocity、速度相关抓地和外力入口。低速保持高抓地，高速急转允许短时侧向速度，再由地面抓地逐渐衰减。套索横向力和地形影响通过同一外力边界进入。

### P17 — 世界与生态扩展评估

只有 P14–P16 证明核心体验可学习、可解释且愿意重玩后，才评估生态、世界规模、经济、剧情或更多动物。

## 当前明确不做

- 不在 P14.3 前重写 Sphere Sweep 或 HorseMovement。
- 不为代码整洁提前拆 GameMode、HorseBrain 或 Runtime Module。
- 不用 12 秒自动成功路线证明 3–8 分钟流程好玩。
- 不增加天气、季节、育种、经济、NPC、迁徙、多人或开放世界系统。

## P14.3 退出条件

形成 15 轮有效样本和问题频次表，能回答：玩家是否主动重试、失败能否复述、第 3 轮是否较第 1 轮改善、四段核心体验哪段最弱、三种马能否靠行为辨认。随后将问题按 Blocker、Core Feel、Clarity、Polish 排序，并为 P14.4 选出最多 3 个修复目标。
