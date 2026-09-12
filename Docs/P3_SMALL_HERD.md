# P3 小规模马群原型

## 当前体验

原型场景默认生成 5 匹野马。每匹野马都是完整的 `ASteppeWildHorseCharacter`，各自拥有 Brain、移动、碰撞、避障和状态；`ASteppeHerdManager` 只负责成员生成、邻居摘要、群体中心与报警传播，不直接写入马匹位置或速度。

快速接近时，靠近玩家的个体先逃跑。报警按成员距离传播，附近成员先进入 Alert，达到最短观察时间后再进入 Fleeing，因此不会整群在同一帧切换状态。慢速接近仍继承 P2.2 的 Alert / Yielding / Fleeing 层次。

成员移动会混合以下局部引导：

- Separation：距离过近时相互避让。
- Cohesion：平静、观察或退让时倾向群体中心。
- Alignment：参考附近马群的平均移动方向。
- Flight alignment：逃跑时参考群体方向，但不使用中心吸引，避免逃跑时折返。

所有结果仍转换为 `FHorseMovementIntent`，由现有 `UHorseMovementComponent` 执行加速、转向、地面运动和危险制动。

## 默认参数

| 参数 | 默认值 | 作用 |
| --- | ---: | --- |
| HerdSize | 5 | 原型成员数，限制为 1–12 |
| FormationSpacing | 450 cm | 初始松散楔形间距 |
| NeighborRadius | 1800 cm | 统计邻居的范围 |
| SeparationDistance | 380 cm | 开始相互避让的距离 |
| AlarmPropagationSpeed | 900 cm/s | 报警从受惊个体传播的速度 |
| AlarmHoldSeconds | 1.5 s | 单次群体警报保持时间 |
| AlarmStrength | 0.85 | 群体警报提供的警觉强度 |

马群移动权重保存在 WildHorseConfig 属性中：CohesionWeight 0.3、AlignmentWeight 0.25、SeparationWeight 1.4、FlightAlignmentWeight 0.45。新增属性使用 C++ 默认值，不覆盖已有 P2 调参。

## 试玩方法

打开原型地图并 Play。前方的 5 个浅色占位体是野马群，按 F1 显示遥测，按 F2 重试。

1. 缓慢接近，观察靠近的一侧先进入 Alert / Yielding。
2. 停止或退后，确认成员分别恢复，不会整体同步归零。
3. 按 F2 后高速接近，观察 H1–H5 标签由黄到红分阶段变化。
4. 从马群侧面切入，观察成员避让、扩散和方向对齐。

HUD 显示成员数、直接报警源数量、焦点个体的邻居数。H1–H5 世界标签显示每匹马当前状态；紫色球体和箭头表示群体中心与平均方向，橙色标记仍属于焦点个体。

## 验证

UE 5.8.2 `SteppeEditor Win64 Development` 编译通过。7 项 Steppe 自动测试通过，0 项失败；其中骑乘测试包含 1 项带预期占位座位警告的通过结果。

`Steppe.P3.SmallHerdFormationAndAlarm` 覆盖：生成 5 匹、初始空间范围、首匹直接受惊、报警不瞬间广播、附近成员延迟警戒、最终传播至其余 4 匹、邻居与分离引导，以及成员销毁后的安全清理。

实际游戏烟雾验证连续重载两次，每次恢复已上马和零警觉。最后一次追逐的日志记录为：5 匹成员、4 匹仍在 Fleeing、4 个可见报警源、群体展开范围约 15.4 m。

```powershell
.\Scripts\Build.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.8'
.\Scripts\RunEditor.ps1 -Tests -ExpectedTests 7 -Commands 'Automation RunTests Steppe.' -LogName P3-FinalTests
.\Scripts\RunEditor.ps1 -Game -Render -Smoke -RetrySmoke -Commands 'steppe.Debug.Movement 1' -LogName P3-FinalRender
```

证据位于 `Docs/Validation/P3-Results.json`、`P3-Playground.png` 与 `P3-Retry.txt`。

## 当前限制

- 仅 5 匹近距离完整 Actor，没有远距离 LOD、Mass 或抽象马群。
- 没有领头马角色、长期社会关系、个性或成员替换。
- 局部群体引导不等于全局寻路，复杂障碍可能拆散马群或使部分成员停住。
- 所有成员使用相同占位模型与调参，主要依靠状态标签辨认个体。
- 受惊传播按距离与直接视线工作，尚无声音、风向和遮挡衰减。
- 本轮没有目标切出、套索、捕获和多人同步。
