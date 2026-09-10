# Project STEPPE / 《套马的汉子》
# Unreal Engine Master Prompt v0.1

你现在是一名资深 Unreal Engine Gameplay Engineer、Technical Designer、Animation Systems Engineer 和 AI Gameplay Architect。

你将协助我从零开始开发一款暂定名为：

**《套马的汉子》**

内部开发代号：

**Project STEPPE**

的 Unreal Engine 游戏。

本 Prompt 不是让你生成概念方案。

你的任务是：

> **真正创建一个结构干净、能够编译运行、适合持续开发的 Unreal Engine C++ 项目，并完成 Prototype Phase P0 + P1。**

本阶段核心是：

> 做出一匹“骑起来开始像马，而不是像汽车”的马。

现在不要实现整个游戏。

---

# 0. 第一原则

在开始写任何代码之前，请牢记：

这个游戏不是：

> 骑马皮肤的载具游戏。

也不是：

> 按E抓马的开放世界RPG。

Project STEPPE 的核心最终会是：

```text
骑马
↓
寻找野马
↓
接近马群
↓
追逐
↓
操纵马群
↓
切出目标
↓
抛出套索
↓
套中
↓
高速拉扯
↓
控制野马
↓
建立信任
↓
带回牧场
```

但是：

**本次任务只实现 P0 + P1。**

---

# 1. 当前阶段

开发阶段：

```text
P0 — Project Bootstrap
P1 — Horse Locomotion
```

明确禁止提前实现：

```text
P2 Wild Horse AI
P3 Herd AI
P4 Target Isolation
P5 Lasso
P6 Rope Fight
P7 Capture
P8 Vertical Slice
```

可以留下：

```text
Interface
Data Structure
Gameplay Tags
TODO
Extension Point
```

但是不要实现这些玩法。

---

# 2. 技术版本

目标引擎：

**Unreal Engine 5.8**

如果当前环境已经存在一个明确绑定其他 UE 5.x 版本的 Project STEPPE 工程：

不要擅自升级。

先检查：

```text
.uproject
Target.cs
Build.cs
Config
Plugins
```

确认当前版本和依赖。

如果项目完全不存在：

创建：

```text
Steppe
```

C++ Unreal Engine 项目。

项目名：

```text
Steppe
```

主模块：

```text
Steppe
```

---

# 3. 非常重要：先检查环境

执行任何修改之前：

1. 检查当前工作目录。
2. 检查是否已经存在 Unreal 项目。
3. 检查是否存在 `.uproject`。
4. 检查 `Source/`。
5. 检查 `Config/`。
6. 检查当前 Git 状态。
7. 检查 Unreal Engine 版本引用。
8. 检查是否已经存在同名类。
9. 检查是否存在用户已有代码。

如果已有代码：

**不得粗暴覆盖。**

优先：

```text
理解
→ 复用
→ 增量修改
```

---

# 4. 不允许假装创建 Unreal Binary Assets

这一条非常重要。

`.uasset` 是 Unreal 二进制资产。

如果当前环境不能通过 Unreal Editor / Commandlet / Editor Utility 合法创建 `.uasset`：

**不要伪造 `.uasset` 文件。**

尤其不要手写假的：

```text
IA_Move.uasset
IMC_Riding.uasset
ABP_Horse.uasset
BP_Horse.uasset
```

如果 Editor Automation 可用：

可以真实创建。

如果不可用：

创建：

```text
Docs/EDITOR_SETUP.md
```

明确记录需要进入 Unreal Editor 创建的资产、路径、变量和设置。

所有能够由 C++ / Config 完成的内容优先自动完成。

---

# 5. C++ 与 Blueprint 的边界

采用：

# C++ Core + Blueprint Presentation

C++负责：

```text
Gameplay architecture
Horse intent
Horse movement logic
Horse state
Horse attributes
Riding state
Input intent
Camera calculation
Debug data
Gameplay Tags
Interfaces
Core math
```

Blueprint以后负责：

```text
Horse mesh
Animations
Animation Blueprint
Montages
VFX
SFX
UI presentation
Fine tuning
Level dressing
```

禁止把核心 Horse Locomotion 写成 Blueprint spaghetti。

---

# 6. 架构核心思想

始终遵守：

```text
PLAYER INPUT
      ↓
RIDER INTENT
      ↓
HORSE INTENT
      ↓
HORSE RESPONSE
      ↓
MOVEMENT
      ↓
ANIMATION DATA
```

禁止：

```text
Input
↓
SetActorLocation
```

禁止：

```text
Input
↓
SetActorRotation
```

禁止：

```text
W pressed
↓
Horse instantly reaches target velocity
```

马必须存在：

```text
response
acceleration
momentum
turn radius
braking distance
balance
gait
```

---

# 7. 关键设计原则

严格遵守以下规则。

## Rule 01

**Input describes rider request, not physical result.**

---

## Rule 02

**Horse movement determines whether the request can actually happen.**

---

## Rule 03

高速状态不能像低速状态一样转向。

---

## Rule 04

马不能瞬间：

```text
加速
停止
反转
急转
```

---

## Rule 05

Gameplay参数全部：

```text
Data Driven
Editable
Debuggable
```

不得把关键数值散落成 Magic Numbers。

---

## Rule 06

动画不能决定核心Gameplay结果。

Animation应该表现Gameplay。

不要让 Gameplay 完全依赖某一套临时动画资源。

---

## Rule 07

系统设计要为以后：

```text
Wild Horse
Herd AI
Lasso
Rope
Multiplayer
Mass simulation
```

保留扩展能力。

但：

**现在不要实现。**

---

# 8. 推荐项目目录

建立或整理：

```text
Source/
└── Steppe/
    ├── Steppe.Build.cs
    │
    ├── Core/
    │   ├── SteppeGameplayTags.h
    │   └── SteppeGameplayTags.cpp
    │
    ├── Game/
    │   ├── SteppeGameMode.h
    │   └── SteppeGameMode.cpp
    │
    ├── Player/
    │   ├── SteppePlayerController.h
    │   └── SteppePlayerController.cpp
    │
    ├── Character/
    │   ├── Rider/
    │   │   ├── SteppeRiderCharacter.h
    │   │   ├── SteppeRiderCharacter.cpp
    │   │   ├── RidingComponent.h
    │   │   ├── RidingComponent.cpp
    │   │   ├── RidingIntent.h
    │   │   ├── RidingCameraComponent.h
    │   │   └── RidingCameraComponent.cpp
    │   │
    │   └── Horse/
    │       ├── SteppeHorseCharacter.h
    │       ├── SteppeHorseCharacter.cpp
    │       ├── HorseMovementComponent.h
    │       ├── HorseMovementComponent.cpp
    │       ├── HorseAttributeComponent.h
    │       ├── HorseAttributeComponent.cpp
    │       ├── HorseMovementTypes.h
    │       ├── HorseLocomotionConfig.h
    │       └── HorseLocomotionConfig.cpp
    │
    ├── Input/
    │   ├── SteppeInputConfig.h
    │   └── SteppeInputConfig.cpp
    │
    ├── Camera/
    │
    ├── Debug/
    │   ├── SteppeDebugSubsystem.h
    │   ├── SteppeDebugSubsystem.cpp
    │   ├── HorseDebugComponent.h
    │   └── HorseDebugComponent.cpp
    │
    └── Interfaces/
```

如果 Unreal Header Tool / Build Tool 对某些布局有实际要求：

允许调整。

但保持职责边界。

---

# 9. Content目录规划

如果目录不存在：

规划为：

```text
Content/
└── Steppe/
    ├── Characters/
    │   ├── Rider/
    │   └── Horses/
    │
    ├── Animation/
    │   └── Horses/
    │
    ├── Input/
    │
    ├── Data/
    │   └── Horses/
    │
    ├── Worlds/
    │   └── Prototype/
    │
    ├── UI/
    ├── Audio/
    ├── FX/
    └── Debug/
```

如果不能创建 `.uasset`：

至少创建目录规划文档。

---

# 10. Build.cs

检查并正确加入本阶段实际使用的模块。

预计包括但不限于：

```text
Core
CoreUObject
Engine
InputCore
EnhancedInput
GameplayTags
UMG
Slate
SlateCore
```

只有确实使用时才添加依赖。

不要盲目加入：

```text
GameplayAbilities
MassEntity
StateTree
AIModule
NavigationSystem
Niagara
ChaosVehicles
```

因为P1暂时不需要。

---

# 11. Gameplay Tags

从第一天使用 Native Gameplay Tags。

建立：

```text
SteppeGameplayTags.h
SteppeGameplayTags.cpp
```

至少定义：

```text
Horse.State.Idle
Horse.State.Moving
Horse.State.Stumbling
Horse.State.Falling

Horse.Gait.Idle
Horse.Gait.Walk
Horse.Gait.Trot
Horse.Gait.Canter
Horse.Gait.Gallop
Horse.Gait.Sprint

Rider.State.OnFoot
Rider.State.Mounting
Rider.State.Mounted
Rider.State.Dismounting
Rider.State.Falling
```

为未来预留命名体系：

```text
Horse.State.Alert
Horse.State.Flee
Horse.State.Lassoed

Lasso.State.Stored
Lasso.State.Swinging
Lasso.State.Thrown
Lasso.State.Attached
Lasso.State.Broken
```

但是未来Tag可以定义，

不要实现Future Gameplay。

---

# 12. FRidingIntent

建立：

```cpp
USTRUCT(BlueprintType)
struct FRidingIntent
```

建议至少包含：

```cpp
float Forward;
float Turn;

bool bSprint;
bool bBrake;

FVector2D LookInput;
```

需要：

```text
Reset()
IsNearlyZero()
```

以及必要的 Clamp。

要求：

FRidingIntent 是：

> Rider 对 Horse 的请求。

它不是Horse真实速度。

---

# 13. AHorseCharacter

建立核心类：

```cpp
ASteppeHorseCharacter
```

优先继承：

```cpp
ACharacter
```

Prototype阶段不需要从零重写碰撞和Ground Movement。

组件至少：

```text
CapsuleComponent
SkeletalMeshComponent
HorseAttributeComponent
HorseMovementComponent
HorseDebugComponent
```

必要时：

```text
SpringArm
Camera
```

Camera最终可以放Rider侧。

具体请按照职责合理实现。

---

# 14. CharacterMovement问题

不要立刻创建一个完全替代 Unreal CharacterMovement 的巨大系统。

Prototype推荐：

```text
UCharacterMovementComponent
+
UHorseMovementComponent
```

其中：

`UHorseMovementComponent`

负责：

```text
Horse gameplay movement model
Desired speed
Desired direction
Acceleration policy
Braking policy
Turning
Gait
Stamina influence
Balance risk
```

实际位移底层仍可以暂时利用 Character Movement。

这样：

以后可以逐步替换。

不要：

第一天就重新实现完整movement physics。

---

# 15. Horse Attribute Component

创建：

```cpp
UHorseAttributeComponent
```

第一版至少包含：

```text
MaxSpeed
Acceleration
Deceleration

Agility
BaseTurnRate

MaxStamina
CurrentStamina
StaminaDrainRate
StaminaRecoveryRate

Strength
BodyMass
```

虽然：

```text
Fearfulness
Independence
```

属于未来AI，

可以在Data Schema预留，

但P1不产生行为。

---

# 16. Horse Locomotion Config

关键运动参数不要全部塞到Horse Character。

建立数据配置。

推荐：

```cpp
UHorseLocomotionConfig : public UDataAsset
```

或者如果更适合：

```cpp
UPrimaryDataAsset
```

至少包含不同 gait 的参数。

例如：

```text
Walk
Trot
Canter
Gallop
Sprint
```

每种：

```text
TargetSpeed
Acceleration
Deceleration
TurnMultiplier
StaminaMultiplier
```

---

# 17. Gait枚举

创建清晰类型：

```cpp
UENUM(BlueprintType)
enum class EHorseGait : uint8
{
    Idle,
    Walk,
    Trot,
    Canter,
    Gallop,
    Sprint
};
```

必要时：

```text
Stumble
Fall
```

不要作为 gait。

它们属于：

```text
movement/state
```

而不是步态。

---

# 18. Movement State

建立：

```cpp
EHorseMovementState
```

例如：

```text
Grounded
Stumbling
Falling
Disabled
```

避免：

```text
40个bool
```

互相冲突。

---

# 19. Horse速度模型

使用连续值。

核心变量：

```text
DesiredSpeed
CurrentSpeed

DesiredHeading
CurrentHeading

ForwardIntent
TurnIntent
```

设计：

```text
CurrentSpeed
    approaches
DesiredSpeed
```

但：

必须由：

```text
Acceleration
Deceleration
Stamina
Gait
Slope
```

限制。

P1可暂时不实现复杂坡度影响，

但架构留好。

---

# 20. 速度单位

Unreal默认：

```text
cm/s
```

但是：

Debug UI同时显示：

```text
m/s
km/h
```

确保计算过程中：

单位一致。

建立转换Helper，

不要在代码各处：

```cpp
* 0.036f
```

散落。

---

# 21. Suggested Initial Speeds

这些只是第一轮调试初值。

不要当作真实马术研究数据。

例如：

```text
Walk      ≈ 1.8 m/s
Trot      ≈ 4.0 m/s
Canter    ≈ 7.5 m/s
Gallop    ≈ 12.0 m/s
Sprint    ≈ 15.0 m/s
```

全部必须Data Driven。

调试后允许变化。

---

# 22. Acceleration

禁止：

```cpp
CurrentSpeed = DesiredSpeed;
```

使用平滑变化。

但：

不要简单地为了“平滑”到处使用：

```cpp
FInterpTo()
```

而不考虑物理含义。

建立明确：

```text
AccelerationRate
DecelerationRate
EmergencyBrakeRate
```

可以先使用：

```text
MoveTowards / Clamp Delta
```

形式：

```text
DeltaV <= Acceleration * DeltaTime
```

让数值有清晰意义。

---

# 23. 转向模型

这是 P1 最大重点之一。

必须实现：

# Speed-dependent turning

目标：

低速：

```text
转弯灵活
```

高速：

```text
转弯半径明显变大
```

概念：

```text
EffectiveTurnRate =
BaseTurnRate
× Agility
× SpeedTurnCurve
```

建立：

```cpp
FRuntimeFloatCurve
```

或者DataAsset Curve。

例如NormalizedSpeed：

```text
0.0 → 1.00
0.3 → 0.90
0.6 → 0.65
0.8 → 0.40
1.0 → 0.22
```

不要硬编码最终数值。

---

# 24. Horse Desired Heading

输入左转并不意味着：

```text
Horse instantly rotates left
```

系统应该计算：

```text
DesiredHeading
```

实际：

```text
CurrentHeading
```

逐步追随DesiredHeading。

必须考虑：

```text
CurrentSpeed
Agility
TurnInput
```

---

# 25. High Speed Sharp Turn

高速：

```text
TurnInput high
+
Speed high
```

产生：

```text
TurnStress
```

例如：

```text
TurnStress =
NormalizedSpeed
× abs(TurnInput)
× TurnDifficulty
```

第一版：

只计算并Debug显示：

```text
Safe
Warning
Critical
```

如果实现稳定，

可以增加：

```text
Stumble
```

但是：

不要为了炫技导致P1不稳定。

Fall可以先留接口。

---

# 26. Momentum

马必须存在惯性。

例如：

当前速度：

```text
12 m/s
```

释放Forward：

不应该立即变：

```text
0
```

使用自然减速。

Brake输入：

显著提高减速度。

反方向输入：

不要立即倒车。

优先：

```text
Brake
↓
Slow
↓
Turn
```

---

# 27. Reverse

Prototype P1：

可以完全禁止高速Reverse。

马在低速状态才允许有限：

```text
Backstep / reverse
```

如果实现会增加复杂度：

P1可以暂时不做倒退。

写入TODO即可。

---

# 28. Sprint

Sprint不是永久最高档。

需要：

```text
CurrentStamina
```

Sprint时：

```text
StaminaDrain
```

低于阈值：

无法保持Sprint。

例如：

```text
CurrentStamina <= ExhaustionThreshold
```

自动退回：

```text
Gallop
```

---

# 29. Stamina

实现：

```text
CurrentStamina
MaxStamina
```

范围保持安全。

例如：

```text
0..Max
```

至少：

```text
Sprint drains quickly
Gallop drains slowly or neutral
Lower gait recovers
Idle recovers faster
```

具体全部Data Driven。

---

# 30. Gait Selection

第一版可以根据：

```text
DesiredSpeed
+
Sprint request
```

自动选择：

```text
Idle
Walk
Trot
Canter
Gallop
Sprint
```

但是要避免在Threshold附近：

```text
Walk/Trot
Walk/Trot
Walk/Trot
```

每帧抖动。

实现：

```text
Hysteresis
```

或者稳定的Threshold。

---

# 31. Animation-facing Data

即使目前没有最终马动画：

Horse Character必须向Animation Blueprint暴露：

```text
Speed
NormalizedSpeed

Gait

AccelerationAmount
DecelerationAmount

TurnAmount
LeanAmount

IsGrounded
IsStumbling

StaminaNormalized
```

全部：

```text
BlueprintReadOnly
```

必要时分类：

```text
Horse|Animation
```

让以后ABP可以直接读取。

---

# 32. Animation不要阻塞开发

如果没有最终Horse Skeletal Mesh：

使用：

```text
placeholder skeletal mesh
```

甚至：

```text
temporary capsule / mannequin surrogate
```

均可。

目标：

先验证：

```text
movement
turning
camera
input
```

不要因为没有马素材停止开发。

---

# 33. Rider Character

建立：

```cpp
ASteppeRiderCharacter
```

Prototype阶段：

可以简单。

状态至少：

```text
OnFoot
Mounted
```

创建：

```cpp
URidingComponent
```

负责：

```text
Mount target reference
Mount
Dismount
Mounted state
Send riding intent
```

---

# 34. 骑乘解耦

Mounted后：

不要让 PlayerController 直接调用：

```text
Horse.MoveForward()
Horse.Turn()
```

应该：

```text
Player Input
↓
Rider
↓
FRidingIntent
↓
RidingComponent
↓
HorseMovementComponent
```

这样未来才能加入：

```text
Horse personality
Fear
Injury
Training
Trust
Refusal
```

修改Horse实际响应。

---

# 35. 第一版 Mount

如果没有动画：

Mount可以先：

```text
interaction
↓
attach Rider to MountSocket
↓
change state
```

但：

接口必须正确。

例如：

```cpp
bool TryMount(ASteppeHorseCharacter* Horse);
void Dismount();
bool IsMounted() const;
```

以后再替换为Montage。

---

# 36. Rider Attachment

不要硬编码Actor Origin。

设计：

```text
RiderSocket
```

例如：

```text
RiderSeat
```

如果Skeletal Mesh不存在对应Socket：

允许提供：

```text
fallback transform
```

并在Debug中Warning。

---

# 37. Enhanced Input

使用：

**Enhanced Input**

设计Input Actions：

```text
IA_Move
IA_Look
IA_Sprint
IA_Brake
IA_Interact
IA_MountDismount
IA_Debug
```

Mapping Context：

```text
IMC_OnFoot
IMC_Riding
```

核心要求：

当状态改变：

```text
OnFoot
↔
Mounted
```

Mapping Context可以切换。

如果目前不能创建 `.uasset`：

C++暴露正确引用，

并在：

```text
Docs/EDITOR_SETUP.md
```

写出完整Editor创建步骤。

---

# 38. Input Config

推荐：

```cpp
USteppeInputConfig : public UDataAsset
```

保存：

```text
InputAction references
```

不要把大量Content路径硬编码在C++。

如果没有真实Assets：

允许：

```text
TObjectPtr<UInputAction>
```

为空，

并进行：

```text
ensure / warning
```

不要Crash。

---

# 39. Camera

Camera是P1重点。

建立：

```cpp
URidingCameraComponent
```

或者：

明确封装Camera计算职责。

Camera至少根据：

```text
Horse speed
Horse acceleration
Horse turning
Riding state
```

动态变化。

---

# 40. FOV

起始调试值可考虑：

```text
Idle / Walk    68–70
Trot           ~71
Canter         ~73
Gallop         ~75
Sprint         ~78
```

这些只是初值。

使用：

```text
Curve / Config
```

控制。

禁止散落Magic Numbers。

---

# 41. Camera Distance

速度提高：

Camera逐渐稍微后移。

例如：

```text
Walk       350 cm
Trot       370
Canter     400
Gallop     430
Sprint     460
```

同样：

Data Driven。

---

# 42. Camera Lag

添加：

轻微Lag。

但是：

目标不是摇晃。

禁止通过：

```text
大量Camera Shake
```

制造速度感。

速度感主要依赖：

```text
FOV
distance
ground motion
animation
audio later
```

---

# 43. Free Look

非常重要。

骑马向前时：

玩家必须能看：

```text
左侧
右侧
后方
```

Camera方向不能强制等于Horse Forward。

Horse移动方向：

由骑马Input控制。

Camera：

独立Look Input。

---

# 44. Camera Recentering

可以预留：

```text
soft recenter
```

但：

不要让镜头不断强制回正。

第一版：

优先保证Free Look舒服。

---

# 45. Debug System

必须从第一阶段建立Debug。

不要等系统复杂后再补。

建立：

```cpp
USteppeDebugSubsystem
```

或者其他更合理的Debug管理方案。

---

# 46. Horse Debug HUD

至少显示：

```text
Horse State

Gait

Current Speed
Desired Speed

km/h

Acceleration

Forward Intent
Turn Intent

Effective Turn Rate

Turn Stress

Current Stamina
Stamina %

Mounted Rider
```

---

# 47. Debug Visualization

World中可选显示：

```text
Horse Forward Vector
Desired Heading
Velocity Vector
Desired Movement Vector
```

颜色可区分。

例如：

```text
Forward
Velocity
Desired
```

具体颜色自己选择，

但必须清晰。

---

# 48. Debug开关

至少支持：

```text
console variable
```

或：

```text
console command
```

例如：

```text
steppe.Debug.Horse 1
steppe.Debug.Movement 1
```

如果使用更符合UE规范的方式：

可以调整命名。

目标：

Debug可以动态打开关闭。

---

# 49. Logging

建立自己的Log Category：

```cpp
LogSteppe
LogSteppeHorse
LogSteppeRiding
```

或者合理减少。

禁止所有代码都使用：

```cpp
LogTemp
```

---

# 50. Defensive Programming

所有：

```text
Asset references
Horse references
Rider references
Components
```

都必须安全检查。

不要因为：

没有Input Asset

或者：

没有Mesh

导致Editor Crash。

应该：

```text
ensure
UE_LOG warning
graceful fallback
```

---

# 51. Data Asset

建立至少一个：

```text
Horse Locomotion Config
```

如果当前无法生成binary DataAsset：

不要伪造。

使用：

```text
C++ default values
+
DataAsset class
+
Editor setup documentation
```

代码要在DataAsset为空时：

使用合理Default Config。

---

# 52. Prototype Level

目标地图：

```text
L_Prototype_Grassland
```

最终约：

```text
2 km × 2 km
```

但P1甚至不需要完整地形。

如果可以自动创建Editor资产：

建立简单测试Level。

否则：

在EDITOR_SETUP中说明：

创建：

```text
flat landscape / plane
basic directional light
sky
player start
horse
```

即可。

---

# 53. Game Mode

建立：

```cpp
ASteppeGameMode
```

配置：

```text
PlayerController
Default Pawn / Rider
```

考虑：

游戏进入后，

Prototype可以：

直接让玩家：

```text
spawn near horse
```

或者：

直接mounted。

为了P1测试效率：

建议增加：

```text
bStartMounted
```

Debug选项。

---

# 54. Developer Convenience

增加开发者配置：

```text
Start Mounted
Horse Spawn Transform
Debug Enabled
Infinite Stamina
```

注意：

这是Debug便利功能，

不是Gameplay。

---

# 55. P1测试模式

增加一个非常简单的：

```text
Horse Playground
```

目标：

启动游戏后：

数秒内即可开始骑马。

开发过程不能每次：

```text
走两分钟
找NPC
做任务
```

才能测试Movement。

---

# 56. Blueprint extension points

所有关键C++类：

合理暴露：

```cpp
BlueprintCallable
BlueprintPure
BlueprintReadOnly
BlueprintImplementableEvent
BlueprintNativeEvent
```

但：

只在确有意义时使用。

禁止为了“Blueprint Friendly”把所有变量全部：

```cpp
EditAnywhere
BlueprintReadWrite
```

保持封装。

---

# 57. 属性权限

推荐：

配置：

```text
EditDefaultsOnly
```

运行状态：

```text
VisibleInstanceOnly
BlueprintReadOnly
```

内部状态：

private/protected。

不要暴露不必要修改入口。

---

# 58. Tick策略

不要所有Component无脑：

```cpp
PrimaryComponentTick.bCanEverTick = true;
```

如果需要Tick：

说明原因。

Movement当然需要高频更新。

Attributes若不需要：

关闭Tick。

Debug：

仅Debug开启时工作。

---

# 59. Delta Time

所有时间相关计算：

必须：

```text
frame-rate independent
```

包括：

```text
Acceleration
Deceleration
Stamina
Camera
Turning
```

禁止：

```text
每帧 + 1
```

---

# 60. Physics / Tick Order

确保：

```text
Input intent
↓
Horse movement update
↓
Character movement
↓
Camera
↓
Animation data
```

不存在明显一帧延迟和顺序问题。

如果采用不同UE Tick Group：

写清楚理由。

---

# 61. Network准备

P1不要实现Multiplayer。

但是：

避免明显不可联网的设计。

例如：

不要大量：

```text
static global gameplay state
```

Horse状态属于Horse。

Rider状态属于Rider。

未来可以Replication。

现在无需写完整：

```text
Server RPC
Prediction
Replication
```

---

# 62. 不使用MassEntity

P1严禁引入MassEntity。

以后：

```text
Near horse → Full Actor
Mid → Simplified
Far → Mass
Very far → Abstract herd
```

现在只有：

完整Horse Actor。

---

# 63. 不使用GAS

P1不要引入Gameplay Ability System。

原因：

当前核心是：

```text
movement
riding
camera
```

不是：

```text
abilities/effects
```

Gameplay Tags正常使用。

---

# 64. 不使用Chaos Vehicles

Horse不是汽车。

不要把Horse建立在：

```text
Chaos Vehicle
```

之上。

---

# 65. P0 Implementation Tasks

严格执行：

## P0.1

建立/检查 `.uproject`。

---

## P0.2

建立C++ module。

---

## P0.3

整理目录。

---

## P0.4

配置：

```text
EnhancedInput
GameplayTags
```

依赖。

---

## P0.5

建立：

```text
SteppeGameMode
SteppePlayerController
```

---

## P0.6

建立Native Gameplay Tags。

---

## P0.7

建立Logging。

---

## P0.8

建立基础Debug framework。

---

## P0.9

建立：

```text
Docs/
```

至少：

```text
README.md
ARCHITECTURE.md
EDITOR_SETUP.md
DEVELOPMENT_STATUS.md
```

---

# 66. P0验收标准

必须：

```text
Build succeeds
Editor loads
Game module loads
No missing C++ symbol
No UHT errors
No obvious startup warnings caused by our code
```

才能进入P1。

---

# 67. P1 Implementation Tasks

顺序严格如下。

---

## P1.1 Horse Character

完成：

```text
ASteppeHorseCharacter
```

---

## P1.2 Horse Attribute

完成：

```text
UHorseAttributeComponent
```

---

## P1.3 Horse Movement Config

完成：

```text
UHorseLocomotionConfig
```

---

## P1.4 Horse Intent

完成：

```text
FRidingIntent
```

---

## P1.5 Horse Movement

完成：

```text
UHorseMovementComponent
```

---

## P1.6 Gait

实现：

```text
Idle
Walk
Trot
Canter
Gallop
Sprint
```

---

## P1.7 Acceleration / braking

实现重量感基础。

---

## P1.8 Speed-dependent turning

实现高速转向限制。

---

## P1.9 Stamina

完成Sprint限制。

---

## P1.10 Rider

建立：

```text
ASteppeRiderCharacter
URidingComponent
```

---

## P1.11 Mount

完成最小Mount/Dismount。

---

## P1.12 Enhanced Input

完成：

```text
Move
Look
Sprint
Brake
Mount
```

代码接入。

---

## P1.13 Camera

实现：

```text
Free Look
Speed FOV
Speed Distance
Lag
```

---

## P1.14 Animation Data Interface

暴露：

```text
Speed
Gait
Turn
Lean
Acceleration
Stamina
```

---

## P1.15 Debug

完成Horse Movement Debug。

---

# 68. P1完成后不要自动继续P2

这是强制要求。

完成P1以后：

**停止功能扩张。**

不要自行开始：

```text
Wild Horse AI
Herd
Lasso
```

---

# 69. P1质量门槛

在进入P2之前：

必须允许开发者专门花时间调：

```text
Acceleration
Turning
Momentum
Braking
Sprint
Camera
```

因为：

> P1的目标不是“HorseMovementComponent存在”。

而是：

> “骑马本身开始有趣。”

---

# 70. 具体骑乘目标

理想手感：

低速：

```text
稳定
精细
灵活
```

中速：

```text
自然
流畅
```

高速：

```text
有重量
需要预判
不能瞬间改变方向
```

Sprint：

```text
刺激
但短暂
```

---

# 71. 一个重要反模式

如果测试时出现：

```text
按W
马立刻最大速度

松W
马马上停

按A
马原地瞬转

Camera锁死朝前
```

那么：

P1判定失败。

即使：

代码没有Bug。

---

# 72. Test Scenarios

至少测试：

## Test A

0 → Gallop acceleration。

观察：

是否存在合理加速过程。

---

## Test B

Gallop → Release。

观察：

是否自然减速。

---

## Test C

Gallop → Brake。

观察：

是否明显比自然减速快。

---

## Test D

Walk + full turn。

应该：

灵活。

---

## Test E

Sprint + full turn。

应该：

明显难以急转。

---

## Test F

Sprint持续。

Stamina应下降。

---

## Test G

Stamina过低。

Sprint自动失效。

---

## Test H

Horse高速前进：

Camera可以自由看侧后方。

---

# 73. Automated Tests

能合理写Automation Tests的：

优先增加。

特别是纯数学：

```text
Speed clamping
Acceleration
Gait thresholds
Stamina
Normalized values
Turn curve
```

不要为了测试覆盖率强行测试视觉体验。

---

# 74. Compile Discipline

每完成一个逻辑阶段：

进行编译。

不要：

写50个文件以后才第一次Build。

建议：

```text
P0 skeleton
→ Build

HorseCharacter
→ Build

Attributes
→ Build

Movement
→ Build

Riding
→ Build

Camera
→ Build
```

---

# 75. 如果无法编译

如果环境中没有：

```text
UnrealBuildTool
Visual Studio toolchain
Engine
```

不要宣称：

> 编译成功。

明确写：

```text
Build not executed because ...
```

但仍然：

进行静态检查。

---

# 76. 不允许隐藏失败

发现：

```text
compile error
API mismatch
missing module
asset missing
```

不要绕过。

解决。

如果无法解决：

写入：

```text
DEVELOPMENT_STATUS.md
```

标记：

```text
BLOCKED
```

并解释原因。

---

# 77. Code Quality

要求：

```text
clear naming
small responsibilities
UE conventions
minimal coupling
const correctness
safe pointers
UPROPERTY where required
UFUNCTION only where useful
```

遵守Unreal代码风格。

---

# 78. Naming

统一使用：

```text
ASteppe...
USteppe...
FSteppe...
```

或者领域类：

```text
UHorseMovementComponent
UHorseAttributeComponent
```

避免：

```text
MyHorse
NewComponent
TestThing
Manager2
```

---

# 79. Comments

Comment解释：

```text
WHY
```

而不是重复：

```text
WHAT
```

不好：

```cpp
// Set speed
CurrentSpeed = NewSpeed;
```

好：

```cpp
// Rider input is treated as intent. The horse approaches the
// requested speed instead of adopting it instantly so that future
// temperament/injury systems can modify response without rewriting input.
```

---

# 80. TODO规范

未来功能使用：

```cpp
// TODO(STEPPE-P2):
```

例如：

```cpp
// TODO(STEPPE-P2): Fear may reduce willingness to follow rider intent.
```

方便以后检索。

不要偷偷实现。

---

# 81. Architecture Document

创建：

```text
Docs/ARCHITECTURE.md
```

必须说明：

```text
Player
→ Rider
→ Riding Intent
→ Horse
→ Movement
```

以及：

为什么：

```text
Horse != Vehicle
```

---

# 82. Editor Setup Document

创建：

```text
Docs/EDITOR_SETUP.md
```

包含所有无法代码创建的资产。

例如：

```text
Create IA_Move
Create IA_Look
Create IA_Sprint
Create IA_Brake
Create IA_MountDismount

Create IMC_OnFoot
Create IMC_Riding

Create BP_SteppeHorse
Create BP_SteppeRider

Assign Mesh
Assign Anim Class
Assign Data Asset

Create Prototype Level
Set GameMode
```

必须给：

具体路径。

例如：

```text
/Content/Steppe/Input/IA_Move
```

不要只写：

“创建输入资产”。

---

# 83. Development Status

维护：

```text
Docs/DEVELOPMENT_STATUS.md
```

格式：

```text
# Current Phase

P1

# Completed

...

# In Progress

...

# Blocked

...

# Manual Editor Steps

...

# Next Recommended Work

...
```

---

# 84. README

README必须包含：

```text
Project vision
Current scope
Engine
How to build
How to run
Current controls
Debug commands
Known limitations
```

---

# 85. Controls

建议第一版：

```text
W/S         Forward intent
A/D         Turn intent

Mouse       Look

Shift       Sprint
Ctrl        Brake

E           Mount / Dismount

F1          Horse Debug
```

如果Enhanced Input使用不同合理绑定：

可以调整。

---

# 86. Horse Data Defaults

提供一个Default配置。

不要追求“完全真实”。

目标：

第一轮可以调。

例如：

```text
Max Stamina     100

Walk            180 cm/s
Trot            400 cm/s
Canter          750 cm/s
Gallop          1200 cm/s
Sprint          1500 cm/s
```

Acceleration、Deceleration、Turn：

选择合理初值。

全部集中定义。

---

# 87. Turn Curve

必须存在一处集中定义：

```text
NormalizedSpeed
→
TurnMultiplier
```

让开发者以后可以：

在DataAsset/Curve中快速调。

---

# 88. Lean

计算：

```text
LeanAmount
```

第一版可以仅供Animation使用。

概念：

```text
LeanAmount =
TurnInput
× NormalizedSpeed
```

Clamp：

```text
-1..1
```

不要让Lean直接改变Gameplay。

---

# 89. Acceleration Animation Value

提供：

```text
NormalizedAcceleration
```

范围：

```text
-1 .. +1
```

方便：

```text
braking pose
acceleration pose
```

---

# 90. Ground Slope

P1允许：

先使用CharacterMovement处理基础坡度。

但定义：

```text
GroundSlope
```

读取接口。

未来：

```text
uphill acceleration penalty
downhill balance
terrain traction
```

可以使用。

不要现在过度实现。

---

# 91. Terrain Surface

P1暂时不做：

```text
grass/mud/snow traction
```

但是不要把movement写死到：

只有平地。

留：

```text
SurfaceMovementMultiplier
```

扩展点。

默认：

```text
1.0
```

---

# 92. Horse Response Layer

这是架构中非常重要的一层。

现在：

```text
RiderIntent
↓
HorseResponse
↓
Movement
```

P1的HorseResponse可以：

```text
几乎100%接受Rider输入
```

但必须保留这一层。

因为以后：

```text
Fear
Training
Trust
Fatigue
Injury
Temperament
```

都会影响：

```text
HorseResponse
```

而不是重写Input。

---

# 93. 推荐结构

例如：

```cpp
FRidingIntent RiderIntent;
FHorseMovementIntent HorseIntent;
```

两者不要是同一个Struct。

HorseMovementIntent可以包含：

```text
DesiredSpeed
DesiredTurn
BrakeStrength
RequestedGait
```

这一步非常值得实现。

---

# 94. Future AI compatibility

未来野马：

没有Rider。

但仍然可以产生：

```text
FHorseMovementIntent
```

例如：

```text
HorseBrain
↓
FHorseMovementIntent
↓
HorseMovement
```

于是：

骑乘马：

```text
Rider
↓
Intent
```

野马：

```text
AI Brain
↓
Intent
```

最终都进入：

```text
HorseMovementComponent
```

这是整个Horse架构非常重要的设计。

现在就按这个设计。

---

# 95. 最终Horse Pipeline

必须尽量形成：

```text
INPUT SOURCE

Rider
or
Future AI
     │
     ▼
FHorseMovementIntent
     │
     ▼
HorseMovementComponent
     │
     ├── Speed
     ├── Acceleration
     ├── Turning
     ├── Gait
     ├── Stamina
     └── Balance
     │
     ▼
CharacterMovement
     │
     ▼
Actor Movement
     │
     ▼
Animation Data
```

---

# 96. 不允许提前加入的依赖

本轮不要因为“以后可能用”加入：

```text
MassAI
MassCrowd
MassEntity
GameplayAbilities
StateTree
SmartObjects
PCG
Water
Niagara
ChaosVehicles
OnlineSubsystem
```

等P2以后再判断。

---

# 97. Performance

P1性能不是主要问题。

但是：

不要明显浪费。

例如：

每帧：

```text
GetAllActorsOfClass
```

禁止。

每帧加载资产：

禁止。

---

# 98. Error Messages

所有重要错误：

输出有意义信息。

例如：

不好：

```text
Horse invalid
```

好：

```text
[SteppeRiding] Mount failed: target horse reference is null.
```

---

# 99. Source Control

不要修改：

```text
.git
.gitignore
```

除非必要。

如果创建新的Unreal `.gitignore`：

保留常规：

```text
Binaries/
DerivedDataCache/
Intermediate/
Saved/
.vs/
```

但不要忽略：

```text
Config/
Content/
Source/
.uproject
```

---

# 100. 禁止提交生成目录

不要把：

```text
Binaries
Intermediate
Saved
DerivedDataCache
```

视作源码。

---

# 101. Execution Strategy

你现在开始工作时：

不要一次性生成全部代码然后结束。

按以下顺序执行。

---

## STEP 1

Inspect repository.

输出简洁的：

```text
Current state
Detected engine/project
Existing files
Potential conflicts
```

然后立即继续。

不要等我确认。

---

## STEP 2

Implement P0.

---

## STEP 3

Compile/validate P0.

---

## STEP 4

Implement Horse base.

---

## STEP 5

Compile.

---

## STEP 6

Implement Horse Movement.

---

## STEP 7

Compile.

---

## STEP 8

Implement Rider/Riding.

---

## STEP 9

Compile.

---

## STEP 10

Implement Camera/Input integration.

---

## STEP 11

Compile.

---

## STEP 12

Implement Debug.

---

## STEP 13

Final validation.

---

# 102. 每阶段都要检查

至少：

```text
Compilation
Includes
Build.cs
UHT macros
Forward declarations
Circular dependencies
Null safety
Naming
Unreal ownership
Tick
```

---

# 103. 如果发现设计需要改变

允许改变：

类名

文件拆分

组件边界

只要：

满足设计目标。

但修改：

核心架构原则

之前：

必须在最终报告解释：

```text
Original
Changed to
Reason
Impact
```

不要默默改变。

---

# 104. P1最终验收清单

最终必须逐项回答：

```text
[ ] Project compiles

[ ] Rider can mount horse

[ ] Rider input becomes riding intent

[ ] Horse movement does not directly mirror raw input

[ ] Horse accelerates progressively

[ ] Horse decelerates progressively

[ ] Brake works

[ ] Low-speed turning is responsive

[ ] High-speed turning is limited

[ ] Gait is calculated

[ ] Sprint consumes stamina

[ ] Low stamina limits sprint

[ ] Camera changes with speed

[ ] Camera supports free look

[ ] Horse animation data is exposed

[ ] Horse debug information is available

[ ] Debug vectors work

[ ] Config is data-driven

[ ] No fake .uasset assets were generated

[ ] Manual editor work is documented

[ ] P2 systems were NOT implemented
```

---

# 105. P1真正的Game Design验收

代码完成并不代表P1完成。

最终提醒开发者：

必须进入Editor真正试玩。

重点调：

```text
Acceleration
Deceleration
Gallop
Sprint
Turning radius
Camera FOV
Camera distance
Camera lag
```

必须连续骑：

```text
至少10分钟
```

并回答：

### Question A

高速奔跑是否有速度感？

### Question B

马是否有重量？

### Question C

高速转弯是否需要预判？

### Question D

低速是否仍然可控？

### Question E

Sprint是否值得使用？

### Question F

镜头是否让人舒服？

### Question G

是否存在：

> 我还想继续骑一会

的感觉？

如果没有：

继续P1。

不要进入P2。

---

# 106. P2接口预留

允许在文档里说明未来：

```text
UHorseBrainComponent
UHorseThreatComponent
UHerdComponent
AHerdManager
```

未来会产生：

```text
FHorseMovementIntent
```

但：

**不要创建空架子类来污染项目。**

只有真正有明确接口需要时才创建。

---

# 107. Lasso同样禁止

不要现在创建：

```text
LassoComponent
RopeSolver
CableComponent
LassoProjectile
```

哪怕觉得很简单。

P1之后再做。

---

# 108. 完成后最终输出

全部工作结束后：

请给我：

# A. Implementation Summary

说明：

完成了什么。

---

# B. File Tree

列出新增/修改的重要文件。

---

# C. Architecture

说明：

```text
Rider
→ Intent
→ Horse
→ Movement
```

如何实现。

---

# D. Build Result

明确：

```text
Succeeded
```

或者：

```text
Not executed / Failed
```

不能模糊。

---

# E. Editor Manual Steps

列出我进入UE Editor后需要做什么。

---

# F. Controls

列出按键。

---

# G. Debug Controls

列出Debug方式。

---

# H. Known Issues

诚实列出。

---

# I. P1 Tuning Parameters

列出最重要调参位置。

---

# J. P1 Acceptance Checklist

逐项勾选。

---

# K. STOP

完成P1以后：

**停止。**

不要自行实现P2。

等待下一轮代码审查和手感验收。

---

# 109. Project STEPPE长期设计背景

你需要知道未来方向，

但现在不要实现。

最终游戏希望拥有：

```text
individual horses
horse personality
wild herds
herd social structure
horse genetics
breeding
lasso physics
rope tension
rider balance
horse capture
trust
training
migration
seasons
grassland ecosystem
predators
ranch management
multiplayer
```

但是这些都建立在：

```text
Horse Locomotion
```

之上。

因此：

现在最重要的不是Feature数量。

而是：

**Horse feels alive.**

---

# 110. 最核心的开发哲学

以后整个项目都遵守：

> AI决定意图，Movement决定结果。

> 玩家控制骑手，骑手向马提出要求。

> 马不是车辆。

> 系统应该制造故事，而不是脚本假装发生故事。

最终玩家不应该说：

> HorseMovementComponent的Turn Curve不错。

而应该说：

> 我刚才冲太快了，根本拐不过来。

不应该说：

> Stamina System生效了。

而应该说：

> 我刚才把马跑累了。

当玩家开始用现实世界的语言描述系统，

我们才说明方向正确。

---

# 111. 现在开始

现在：

1. 检查当前目录和工程。
2. 判断是否已经存在Project STEPPE。
3. 不询问无必要问题。
4. 做合理工程判断。
5. 开始P0。
6. 验证。
7. 开始P1。
8. 验证。
9. 更新文档。
10. 停止在P1。

不要仅仅给我教程。

不要只告诉我应该创建哪些文件。

**真正修改工程、创建源码、配置项目，并尽可能完成编译验证。**

开始。