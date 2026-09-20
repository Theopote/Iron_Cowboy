# P16 Horse Dynamics v2 — 实施基线

**状态：下一阶段，尚未修改运动代码。**

## 当前问题

`UHorseMovementComponent::CalcVelocity` 在地面每帧把 `Velocity` 直接写成 `Heading.Vector() * Speed`。朝向改变时速度方向立即跟随，马体缺少短暂的横向惯性；套索也没有向运动组件施加可复用的侧拉外力入口。

## 本阶段范围与顺序

1. 保留 Rider Intent、Horse Intent、速度响应、步态、体力和原有避障语义。先分离面朝方向与实际水平速度，记录 ForwardSpeed、LateralSpeed 和 SlipAngle。低速高抓地、高速急转允许短时偏航，但不做持续汽车式漂移。
2. 增加可调 `SurfaceGrip`。先只接现有 Grass/Hard 物理表面，保持现有默认速度体验；Wet Grass、Mud、Snow 等仅预留调参入口，不在本阶段建场景或材质。
3. 提供有界的 `AddExternalForce` 或等价运动入口，按固定时间步进入速度积分。套索侧拉通过这一入口影响野马轨迹；AI 仍只提交行为意图。
4. 为落地、碰撞、坐骑、野马和不同帧率增加确定性回归。P15 投索、P14.4 控绳和完整捕获闭环必须继续通过。

## 验收

- 高速急转后 Facing 与 Velocity 有短暂可见夹角，随后按抓地参数收敛；低速转向仍可控。
- 同样的输入和初态产生稳定结果，Grass/Hard 抓地差异可测且可调。
- 有界侧向套索力能改变野马轨迹，同时不让速度、张力或 Rider Balance 失控。
- Editor 全套回归与至少一条实际渲染完整流程通过，随后再启动 P16.5 的骨骼马和骑手动画集成。

## 非目标

本阶段不接正式模型、蹄部 IK、河流、复杂地形、Chaos 软体绳、布娃娃、生态、牧场或育种。
