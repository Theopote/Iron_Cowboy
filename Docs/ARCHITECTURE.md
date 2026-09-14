# Project STEPPE 架构

本文描述已经实现的工程分层。产品范围以 `DESIGN_BASELINE.md` 和 `PROTOTYPE_GDD.md` 为准，跨系统决策以 `DESIGN_DECISIONS.md` 为准。

## 意图与运动

`Enhanced Input → ASteppeRiderCharacter → FRidingIntent → URidingComponent → UHorseMovementComponent::UpdateResponse → FHorseMovementIntent → CalcVelocity → CharacterMovement 碰撞/地面位移 → FHorseAnimationData → 镜头/未来动画`。

PlayerController 不直接控制马。Rider 始终是玩家持有的 Pawn，上马后停止自己的移动、关闭胶囊碰撞，附着到 Horse 的 RiderSeat Socket；无 Socket 时使用可编辑 FallbackSeat 并记录警告。下马检测速度、地面坡度和胶囊落脚空间，两侧都被阻挡时拒绝下马。销毁马或骑手时清理引用和 Tick 依赖。

FRidingIntent 是骑手请求，含前进、转向、冲刺、制动、观察；FHorseMovementIntent 是独立类型，含期望速度、转向、制动强度、请求步态。HorseMovement 对骑手输入作有时间单位的渐进响应，再应用体力、表面乘数与个体上限。SetHorseIntent 是未来非骑手来源的入口，没有创建 AI 类。

## CharacterMovement 的定制边界

原建议为 UCharacterMovementComponent + 单独策略组件；实际使用一个轻量的 `UHorseMovementComponent : UCharacterMovementComponent`，在 ACharacter 构造中替换默认移动子对象。

原因：在引擎的地面子步里定制 CalcVelocity，可避免两个组件争用速度、重复制动和顺序错位。改变范围仅为策略的承载位置，意图→响应→运动原则不变；地面检测、碰撞、斜坡、滑动和落地仍由引擎负责。未重新实现完整物理。

地面速度按 `Δv ≤ rate × Δt` 趋近目标；低速和高速 yaw rate 由归一化速度曲线控制，角度渐进逼近期望朝向。转向压力只报告 Safe/Warning/Critical，不触发跌倒。没有高速倒车。空中沿用引擎基础运动并保持马朝向，不模拟马跳跃。

步态按实际速度加滞回选取，动画不决定游戏结果。速度统一 cm/s，集中 helper 转换 m/s 和 km/h。GroundSlope 提供坡度读数，SurfaceMovementMultiplier 默认为 1，为以后地形影响预留。

## Tick 和所有权

- HorseMovement：PrePhysics，在 Super::TickComponent 前计算响应，在 CMC 地面子步内积分速度/朝向，Super 后记录真实速度、步态、体力和动画数据。
- RidingComponent：PrePhysics，传递持有意图；上马时 HorseMovement 显式依赖该组件，下马时移除。客户端 Pawn 重启时 RidingComponent 依赖 Controller Tick，确保先处理输入。
- RidingCamera：PostPhysics，读取本帧运动数据；SpringArm 显式依赖 RidingCamera 后计算镜头。
- Attributes 和 DebugSubsystem 不 Tick。HUD 只在 DrawHUD 时查询缓存的 PlaygroundHorse；遥测和向量仅在开关开启时计算/绘制。
- 不逐帧扫描所有马；E 交互时才寻找附近可骑目标。
- UObject 引用由 UPROPERTY 持有；骑手与马互相引用采用弱引用。调试控制台变量不是游戏状态。

## 数据与表现

HorseLocomotionConfig 数据资产包含每种 gait 的 TargetSpeed、Acceleration、Deceleration、TurnMultiplier、StaminaMultiplier；体力乘数正数消耗、负数恢复。耗尽门槛与恢复冲刺门槛分离，避免冲刺反复跳档。空配置使用 C++ 默认对象。

FHorseAnimationData 只读暴露 Speed、NormalizedSpeed、Gait、NormalizedAcceleration、AccelerationAmount、DecelerationAmount、TurnAmount、LeanAmount、IsGrounded、IsStumbling、StaminaNormalized。Horse/Rider 通过只读方法返回 Native Gameplay Tags。

SteppeInputConfig 可引用外部 Action/Context。默认没有配置资产时，使用真实内存 UInputAction/UInputMappingContext 自动建立键位，不写假的 .uasset。部分配置缺失时跳过对应绑定并警告。

独立观察使用 Controller ControlRotation，马不跟随观察方向；FOV、距离、加速偏移、转弯偏移平滑变化，没有强制回正或相机震动。

## 文件职责

- Core：Native Tags；Steppe.h/cpp：主模块及日志。
- Character/Horse：角色、运动策略、参数、属性、纯数学、动画数据。
- Character/Rider：输入接收、上下马、意图传递、镜头。
- Input：可配置 Enhanced Input 引用和运行时默认映射。
- Game：Playground 启动与显式命令行烟测模式。
- Debug：World Subsystem、HUD 遥测与向量；不额外创建冗余 HorseDebugComponent。
- Tests：纯数学/体力与真实 UWorld 的运动、上下马、镜头集成验证。

P1 基线保留了非骑手意图入口。本轮 P2 使用该入口接入 HorseBrainComponent，未引入 Mass、GAS、绳索或网络实现。

## P2：单匹野马

SteppeWildHorseCharacter 继承原有 Horse，添加 HorseBrainComponent 并默认禁止骑乘。GameMode 显式指定玩家 Rider 为威胁来源，无逐帧全局扫描。Brain 以配置频率感知距离、接近速度与视线，在 Roaming/Alert/Fleeing/Recovering 之间转换，向原有 Movement 提交 FHorseMovementIntent。

Movement 的物理和体力实现保持共用。Brain 只选择目标与意图，不修改位置/旋转；移动 Tick 依赖 Brain Tick。局部探测避开障碍并检查候选方向落脚面，围堵时请求制动，不提供全局寻路。Mounted Rider 的速度从其附着的坐骑读取。

行为参数来自 WildHorseConfig 数据资产，Native Behavior Tags 与已有 Movement/Gait 状态区分。P2 完整范围、限制和验证见 P2_WILD_HORSE.md。

## P3：小规模马群

`ASteppeHerdManager` 集中生成和登记 1–12 匹完整 WildHorse Actor，以 5 Hz 汇总中心、平均速度、邻居和分离向量，并按空间距离传播警报。Brain 将群体摘要与自己的目标方向混合后继续输出 `FHorseMovementIntent`。Manager 不直接移动成员，也不替代每匹马的感知、状态机、避障或 CMC。当前原型规模为 5，详细范围见 P3_SMALL_HERD.md。

## P4：目标切出

Q 输入经 Rider 转发到 PlayerController，再由 GameMode 的 HerdManager 从相机方向选择成员。Manager 持有当前目标，计算目标与其余成员中心的二维距离，并按持续时间推进隔离进度。选中目标的 Brain 关闭凝聚与群体方向对齐，继续应用自身逃跑方向、成员分离、玩家坐骑动态避让和环境探测。完成状态锁存在本次选择中；取消、目标销毁或关卡重试会清理状态。详细范围见 P4_TARGET_ISOLATION.md。

## P5：套索

`ULassoComponent` 属于 Rider，接收 RMB/LMB 输入并维护 Stored、Aiming、Thrown、Attached、Recovering。投掷每帧从上一位置到下一位置作连续球形扫掠，命中只接受 P4 当前隔离目标；HUD 绳线与命中圈不决定结果。Attached 通过 HorseBrain 的 Lassoed 状态向共用 HorseMovement 提交紧急制动意图。主动释放、脱靶、障碍、超长或目标销毁都汇入恢复流程。状态同时通过 Native Gameplay Tags 暴露，便于后续动画、声音和网络表现读取。详细范围见 P5_LASSO.md。

## P6：绳索对抗

Attached 后，LassoComponent 根据绳距和两端沿绳方向的相对速度计算张力。Rider 的空格输入设置 Bracing；Brain 接收锚点、张力与稳绳状态，并继续通过 HorseMovement 产生向外挣扎或受控制动。LassoComponent 在有效张力区间累计 ControlProgress，过载则断绳，完成后进入 Subdued。该分层让未来的绳索网格、动画或物理表现读取同一状态，而不接管判定。P6 不生成捕获奖励或移除野马，详细范围见 P6_ROPE_FIGHT.md。

## P7：捕获结果

C 输入由 Rider 转给 LassoComponent。只有 Subdued 状态可提交捕获；HerdManager 是活动/捕获成员登记的权威，负责从 Members 移除目标、加入 CapturedHorses、清除焦点并更新计数。HorseBrain 持有 Captured 行为状态并继续通过 HorseMovement 制动。Actor 保留用于本轮结果显示，套索可独立收回。该边界为未来存档或任务系统提供明确的“捕获已发生”事件点，当前没有实现这些上层系统。详见 P7_CAPTURE.md。

## P8：限时垂直切片

GameMode 持有轻量的 `FSteppeTrialProgress`，在玩家与马群完成生成后启动 120 秒任务，并从 HerdManager 的权威捕获计数更新进度。规则结构负责成功/失败、剩余时间冻结和积分计算；HUD 只读取结果并绘制任务条与结算层。F2 仍通过关卡重载重建全部本轮状态。任务层不反向修改骑乘、马群或套索逻辑，因此 P1–P7 玩法可独立测试和继续调参。详见 P8_VERTICAL_SLICE.md。

## P9：捕获后安全接近

`UHorseTrustComponent` 属于 WildHorse，捕获登记时由 HerdManager 注入当前 Rider，并开始评估二维距离、有符号接近速度和骑乘状态。组件持有 Secured、CalmApproach、Rejected、ReadyForContact 与 FirstContact；高速冲入通过 HorseBrain 提交短时后退意图，Movement 仍负责真实移动。HerdManager 记录首次接触的权威集合与计数。Rider 的 E 输入在徒步且靠近已捕获目标时优先交给接触流程，否则保持原上马行为。详见 P9_CAPTURE_AFTERMATH_SPEC.md。

## P10：牵回营地与命名

第一次接触后，HorseTrust 进入 Leading，并让 HorseBrain 持有弱引用 LeadTarget。Brain 计算骑手后方锚点、跟随距离和环境安全方向，只向 HorseMovement 提交 Walk/Brake 意图。`ASteppeDeliveryZone` 提供可见围栏和空间判定；HerdManager 要求骑手与牵行目标同时在区域内才进入 Delivered。PlayerController 随后显示原生 UMG `UHorseNamingWidget`，UI 接管键盘焦点并通过 HerdManager 提交名字。Trial 读取 DeliveredCount 与 NamedCount，仅在命名达到目标后成功。详见 P10_LEAD_DELIVERY_NAMING_SPEC.md。

## P11：可感知马匹原型

`FWildHorseArchetypeProfile` 汇总身份文案、灰盒识别色，以及速度、体力、力量、敏捷、恐惧、挣扎和接近规则倍率。HerdManager 按成员编号确定性选择可编辑 Profile，并在 Actor 完成生成后调用 WildHorse 的幂等应用入口。WildHorse 将最终值分别写入 HorseAttribute、HorseBrain 和 HorseTrust；LassoComponent 查询目标的控制抗性计算实际压制时间。运动、AI、套索和信任组件仍只负责各自规则，HUD 与 Horse Card 读取结果但不参与判定。详见 P11_HORSE_ARCHETYPES_SPEC.md。

## P12 第一增量：摆绳与命中区域

LassoComponent 在 Aiming 内累计时间，并用可编辑周期计算 SwingPhase 与 SwingStability。Throw 锁定 Stability，再得到本次 EffectiveCaptureRadius、EffectiveThrowSpeed 和 EffectiveMaximumRange；连续 Sweep 仍是唯一命中判定，稳定性不修改瞄准方向。命中目标后按 Actor 局部高度记录 Head、Neck 或 Torso，区域倍率进入张力和实际 SubdueSeconds。HUD 只读取稳定性、命中区与既有状态。未来替换为骨骼碰撞体时可改写区域分类，不需要修改绳索对抗接口。详见 P12_LASSO_SKILL_SPEC.md。

## P12 第二增量：骑手平衡、落马与拖行

RiderBalanceComponent 在 PostPhysics 且晚于 LassoComponent 更新，读取本帧张力、绳索相对坐骑的侧向比例、坐骑速度、目标 Strength 和命中区，累计可恢复的 Balance。达到阈值后调用 RidingComponent 的 ForceDismount，一次性清理 MountedHorse 双向关系、Tick 前置和输入，恢复 Rider 碰撞并设置 Falling 速度。短距离 Dragged 仍由 CharacterMovement 的速度处理；组件只在限定时间内提交朝向目标的受限速度，并在 LMB Release 或超时时让 Lasso 进入既有 Recovery。Rider Gameplay Tags 和 HUD 读取 Warning、Dragged、Recovering 状态，不决定结果。
