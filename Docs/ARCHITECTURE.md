# 当前实现：P0

Steppe 是 Runtime 主模块；SteppeTarget 与 SteppeEditorTarget 分别提供游戏和编辑器构建目标。DefaultEngine.ini 指定原生 SteppeGameMode，后者指定 SteppePlayerController。目前保留引擎默认 Pawn，尚未接入 Rider。

Native Gameplay Tags 集中在 Core/SteppeGameplayTags；日志类别为 LogSteppe、LogSteppeHorse、LogSteppeRiding。USteppeDebugSubsystem 提供世界级查询接口，两个控制台变量仅控制开发调试显示，不承载游戏状态。Subsystem 不 Tick，当前没有调试绘制开销。

只引入 Core、CoreUObject、Engine、InputCore、EnhancedInput、GameplayTags；其中输入依赖为需求明确要求的 P0 接入准备。没有引入 GAS、Mass、ChaosVehicles 或未来玩法依赖。

# P1 设计，尚未实现

Player Input → Rider → FRidingIntent → RidingComponent → Horse Response → FHorseMovementIntent → HorseMovementComponent → CharacterMovement → Animation Data。

FRidingIntent 表达骑手请求；独立的 FHorseMovementIntent 表达马接受后的运动意图。速度、朝向、体力与转向限制由运动层决定，输入不得直接设置 Actor 位移或旋转。底层利用 CharacterMovement 处理碰撞、落地与基础坡度。

马不是车辆：需要渐进响应、加减速度、速度相关转向半径与有限冲刺能力。动画表现运动结果；没有最终模型也应可验证运动。低速保持可控，高速需要预判。

预计每帧顺序为 Rider 意图 → Horse 运动策略 → CharacterMovement → 实际运动数据 → Camera/动画；实现时必须通过 Tick prerequisite 明确依赖并验证。属性组件不单独 Tick；调试绘制仅在开启时工作。

未来 AI 可以产生同一 FHorseMovementIntent，但 P1 不创建 AI 或套索空架子。参数集中在 HorseLocomotionConfig，运行状态只读暴露给 Blueprint。当前无 P1 类，避免将设计描述误认作已实现代码。
