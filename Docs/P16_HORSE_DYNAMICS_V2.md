# P16 Horse Dynamics v2 — 实施基线

**状态：P16 第一轮已实现并通过自动化；真人手感复核待进行。**

## 当前问题

原实现的 `UHorseMovementComponent::CalcVelocity` 每帧把 `Velocity` 直接写成 `Heading.Vector() * Speed`。朝向改变时速度方向立即跟随，马体缺少短暂的横向惯性；套索也没有向运动组件施加可复用的侧拉外力入口。本轮已替换这两个行为。

## 已实现

- `TurnVelocityTowardFacing` 以指数抓地响应让水平速度方向逐步追上朝向，同时保留原有速度大小响应。低速抓地率 18/s，高速 3.5/s；马的 `ForwardSpeed`、`LateralSpeed`、`SlipAngleDegrees`、`EffectiveGripRate` 可直接检查。朝向仍受原有速度相关转向率限制。
- 草地与硬地使用现有 Physical Surface，默认抓地倍率分别为 1.0 与 1.05。CharacterMovement 的地面扫掠默认不请求物理材质，故优先读取 Hit 的 PhysMaterial，缺失时读取落脚组件第 0 材质槽的 Physical Material；未配置时回退草地。
- `SetExternalAcceleration` 提供水平外力入口，默认上限 700 cm/s²，非有限值归零；积分后的水平速度不超过马自身最高速的 110%。套索 Attached 状态根据实时张力和马到骑手/障碍弯折点的方向施加侧拉，释放、归顺、捕获及结束时清除。
- 原有套索速度棘轮、绳索断裂、骑手平衡和马群 AI 规则不变。运动组件负责最终轨迹，Lasso 只提供本帧牵引数据。

## 当前验证

UE 5.8.2 Editor 与 Win64 Shipping 构建成功；24 项 Steppe 自动化全部通过。新测试覆盖不同抓地率、帧率稳定性、速度大小、地表差异、转向侧滑/收敛、外力上限与轨迹改变、绳索施力与释放清理。带渲染的完整捕获、牵回和命名 Smoke 通过，打包后 Smoke 启动退出码 0。视觉、音频及真人手感仍需下一轮体验复核。详细结果见 `Validation/P16-Dynamics-Runs.txt`。

## 本阶段范围与顺序

1. 保留 Rider Intent、Horse Intent、速度响应、步态、体力和原有避障语义。先分离面朝方向与实际水平速度，记录 ForwardSpeed、LateralSpeed 和 SlipAngle。低速高抓地、高速急转允许短时偏航，但不做持续汽车式漂移。
2. 增加可调 `SurfaceGrip`。先只接现有 Grass/Hard 物理表面，保持现有默认速度体验；Wet Grass、Mud、Snow 等仅预留调参入口，不在本阶段建场景或材质。
3. 提供有界的外力运动入口，按 CharacterMovement 的积分步长进入速度积分。套索侧拉通过这一入口影响野马轨迹；AI 仍只提交行为意图。
4. 为落地、碰撞、坐骑、野马和不同帧率增加确定性回归。P15 投索、P14.4 控绳和完整捕获闭环必须继续通过。

## 验收

- 高速急转后 Facing 与 Velocity 有短暂可见夹角，随后按抓地参数收敛；低速转向仍可控。
- 同样的输入和初态产生稳定结果，Grass/Hard 抓地差异可测且可调。
- 有界侧向套索力能改变野马轨迹，同时不让速度、张力或 Rider Balance 失控。
- Editor 全套回归与至少一条实际渲染完整流程通过，随后再启动 P16.5 的骨骼马和骑手动画集成。

## 非目标

本阶段不接正式模型、蹄部 IK、河流、复杂地形、Chaos 软体绳、布娃娃、生态、牧场或育种。
