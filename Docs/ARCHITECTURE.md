# P0 + P1 架构

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

仅保留 P2 意图入口。未引入 Mass、GAS、AI、绳索或网络实现。
