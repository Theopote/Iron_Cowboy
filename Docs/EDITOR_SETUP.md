# Editor 设置与试玩

## 已完成

目标 UE 5.8.2 位于 `C:\Program Files\Epic Games\UE_5.8`。C++ Editor 模块已编译，以下资产已通过 UE Editor Python 真实创建，可直接打开：

| 资产路径 | 作用 |
| --- | --- |
| `/Game/Steppe/Worlds/Prototype/L_Prototype_Grassland` | 2 km × 2 km 平地、间隔 25 m 标记、绕桩、坡道、PlayerStart、动态光照 |
| `/Game/Steppe/BP_SteppeGameMode` | 默认 Rider/Horse 类、启动已上马和调试参数 |
| `/Game/Steppe/Characters/Horses/BP_SteppeHorse` | 继承原生 Horse，引用默认运动配置 |
| `/Game/Steppe/Characters/Rider/BP_SteppeRider` | 继承原生 Rider，占位圆柱 |
| `/Game/Steppe/Data/Horses/DA_HorseLocomotion_Default` | 步态、转向、体力和镜头曲线 |
| `/Game/Steppe/Debug/M_PrototypeGrass` | 灰盒地面颜色 |
| `/Game/Steppe/Debug/M_PrototypeMarker` | 标记颜色 |
| `/Game/Steppe/Debug/M_PrototypeHorse` | 马占位体颜色 |
| `/Game/Steppe/Debug/M_PrototypeRider` | 骑手占位体颜色 |

GameDefaultMap 和 EditorStartupMap 已指向测试地图。进入 PIE 即可骑乘，无需先创建输入资产。

## 人工验收必须完成

连续试玩至少 10 分钟：

1. 按住 W，确认逐渐加速；松开 W 后保持惯性减速。
2. 比较同速度松手与 Ctrl 制动的距离。
3. 低速与 Shift 高速分别按住 A/D，比较转弯半径和遥测 TurnStress。
4. 持续冲刺，观察体力消耗与耗尽降速；降低步态恢复体力后再冲刺。
5. 向前骑行时转动鼠标看侧后方，马方向应不被镜头改变。
6. 停稳后按 E 下马，靠近再按 E 上马；高速 E 和落脚点被挡住时应拒绝下马。
7. 检查镜头 FOV、距离、Lag 是否舒适；调整参数后重复以上场景。

目标：高速有速度感、有重量且需要预判，低速可控，冲刺值得用，愿意继续骑。不能用“代码通过”代替此项验收。

## 调参位置

打开 DA_HorseLocomotion_Default：Gaits 数组固定索引为 Idle/Walk/Trot/Canter/Gallop/Sprint，请保持六项和速度递增。初始目标速度 0/180/400/750/1200/1500 cm/s。

- Acceleration / Deceleration：各步态加减速度，受 Horse Attributes 个体上限约束。
- EmergencyBrakeRate：强制动，默认 600 cm/s²。
- SpeedTurnCurve：归一化速度到转向系数，初始 1 → 0.22；与 BaseTurnRate、Agility 相乘。
- ResponseSeconds：骑手意图接受时间，默认 0.25 s。
- GaitHysteresis：步态阈值滞回，默认 20 cm/s。
- ExhaustionThreshold / SprintResumeThreshold：默认 5 / 25。
- StaminaMultiplier：正数消耗、负数恢复，与 Attributes 的消耗/恢复速率相乘。
- SpeedFOVCurve / SpeedDistanceCurve：速度用 cm/s 作横轴，FOV 用度、距离用 cm。
- CameraBlendRate / CameraLagSpeed：镜头变化和位置跟随。

BP_SteppeHorse 的 Attributes：速度上限、敏捷、BaseTurnRate、体力、个体加減速度、强度与质量。BP_SteppeGameMode：StartMounted、HorseSpawnTransform、DebugEnabled、InfiniteStamina。上下马距离、允许下马速度和侧向偏移在 Rider 的 Riding 组件。

## 临时骨骼马与正式动画

P16.5 已把 Quaternius CC0 的临时 Skeletal Mesh 接入所有 Horse，按实际步态播放 Idle/Walk/Gallop，并自动隐藏旧灰盒表现；资源位置、许可、重建与限制见 [P16.5 临时骨骼马素材](P16.5_TEMPORARY_HORSE.md)。没有 `RiderSeat` Socket 时仍使用 FallbackSeat，日志 Warning 为预期行为。角色 Capsule 保留，用于玩法碰撞。

正式阶段可在 `/Game/Steppe/Animation/Horses/ABP_Horse` 创建与模型骨架一致的 AnimBP，读取 Horse 的 AnimationData。使用 Speed、Gait、GaitPhase、StrideBlend、BodyBob/Pitch/Roll、FootContactPulse、Acceleration、Turn、Lean 和 Stamina 驱动表现，不能用动画直接替代运动计算。临时网格接入时已关闭 HorsePresentationComponent 的 `bAnimatePlaceholder`。骑手 AnimBP 读取 Rider.PresentationData 中的 Mounted、Bracing、Falling、Dragged、BalanceRisk、PullSide、BodyPitch/Roll 与 SeatOffsetZ。

## P13 表面与表现资源

- Project Settings → Physics 已登记 `SurfaceType1=Grass`、`SurfaceType2=Hard`。
- `/Game/Steppe/Feedback/PM_Grass` 与 `PM_Hard` 已绑定原型草地和硬地材质；地图中的 `HardSurface_TestPad` 可直接骑上去比较反馈。
- 在 Rider Blueprint 的 SteppeFeedback → Assets 中配置 GrassHoof、HardHoof、Breath、套索/风险 SoundBase 和 Grass/Hard HoofDust NiagaraSystem。空插槽会使用程序化/灰盒回退，不影响 Gameplay。
- 导入的正式声音和粒子必须保留来源与许可记录；动画、声音和粒子只读取现有状态接口。

## 可选：把运行时输入替换为编辑器资产

在 `/Game/Steppe/Input` 创建：

- IA_Move、IA_Look：Axis2D。
- IA_Sprint、IA_Brake、IA_Interact、IA_MountDismount、IA_Debug：Bool。
- IMC_OnFoot、IMC_Riding：Mapping Context。
- DA_SteppeInput：SteppeInputConfig 数据资产，引用以上各项，并在 BP_SteppeRider 的 InputConfig 指定。

两个 Context 都映射：D → Move +X；A → Move -X（Negate）；W → Move +Y（Swizzle YXZ）；S → Move -Y（Negate 后 Swizzle YXZ）。MouseX → Look X；MouseY → Look -Y。Shift → Sprint，Ctrl → Brake，E → MountDismount，F1 → Debug。

IA_Interact 是另一个可配置交互入口，默认不绑定键位；不要把 E 同时绑定 Interact 和 MountDismount，以免同次按键上下马两次。默认运行时映射已经处理这些规则，通常无需手工替换。

## 资产自动化脚本

`Scripts/CreatePlayground.py` 创建基础资产，已有地图不重建；`Scripts/PolishPlayground.py` 给灰盒应用颜色和动态光照。它们会保存资产，日常调参不必重新运行。复建空工程时：

```powershell
.\Scripts\RunEditor.ps1 -PythonScript 'D:\development\Project STEPPE\Scripts\CreatePlayground.py' -LogName CreatePlayground
.\Scripts\RunEditor.ps1 -PythonScript 'D:\development\Project STEPPE\Scripts\PolishPlayground.py' -LogName PolishPlayground
```

PythonScriptPlugin 仅对 Editor 启用；Runtime 不依赖 Python。没有伪造 .uasset 或 .umap。

## P2 单匹野马

默认 GameMode 现已生成一匹不可直接骑乘的野马。P1 按键不变；野马的行为资产、出生位置、状态说明与慢/快接近比较见 P2_WILD_HORSE.md。关闭 GameMode 的 SpawnWildHorse 可回到纯 P1 运动场。
