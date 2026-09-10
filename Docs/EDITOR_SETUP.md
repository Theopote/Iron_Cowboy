# P0 必须先完成的操作

1. 安装 UE 5.8，并准备该版本要求的 Visual Studio C++ 工具链。不要用 5.7 打开并重新关联工程。
2. 执行根目录 README 的 Build.ps1，传入实际 UE 5.8 根目录。
3. 用 UE 5.8 打开 Steppe.uproject。验证 UHT、C++ 编译、Steppe 模块加载均成功。
4. 新建基础关卡并保存为 `/Game/Steppe/Worlds/Prototype/L_Prototype_Grassland`，磁盘路径为 `Content/Steppe/Worlds/Prototype/L_Prototype_Grassland.umap`。
5. 放置地面、Directional Light、Sky Light、Player Start。确认默认 GameMode 为 SteppeGameMode。执行 PIE，检查 Output Log 无本模块导致的启动警告。
6. 执行 `steppe.Debug.Horse 1`、`steppe.Debug.Movement 1`、`SteppeToggleDebug` 验证控制台注册；P0 尚不显示马遥测。
7. 记录实际构建和加载结果到 DEVELOPMENT_STATUS.md，通过后才能开始 P1。

# P1 资产规划，代码到位后再创建

以下是预留资产清单，不代表现有资产或当前已能配置的功能。

| 路径 | 类型及设置 |
| --- | --- |
| `/Game/Steppe/Input/IA_Move` | Input Action，Axis2D；X 转向、Y 前进 |
| `/Game/Steppe/Input/IA_Look` | Input Action，Axis2D；鼠标观察 |
| `/Game/Steppe/Input/IA_Sprint` | Input Action，Bool |
| `/Game/Steppe/Input/IA_Brake` | Input Action，Bool |
| `/Game/Steppe/Input/IA_Interact` | Input Action，Bool，预留独立交互 |
| `/Game/Steppe/Input/IA_MountDismount` | Input Action，Bool，E |
| `/Game/Steppe/Input/IA_Debug` | Input Action，Bool，F1 |
| `/Game/Steppe/Input/IMC_OnFoot` | Mapping Context，步行与交互 |
| `/Game/Steppe/Input/IMC_Riding` | Mapping Context，W/S、A/D、鼠标、Shift、Ctrl、E、F1 |
| `/Game/Steppe/Input/DA_SteppeInput` | 未来 SteppeInputConfig 数据资产，引用上述 Action 与 Context |
| `/Game/Steppe/Data/Horses/DA_HorseLocomotion_Default` | 未来 HorseLocomotionConfig 数据资产 |
| `/Game/Steppe/Characters/Horses/BP_SteppeHorse` | 未来 SteppeHorseCharacter 子类 |
| `/Game/Steppe/Characters/Rider/BP_SteppeRider` | 未来 SteppeRiderCharacter 子类 |
| `/Game/Steppe/Animation/Horses/ABP_Horse` | 有真实骨架后再建 Animation Blueprint |

输入映射计划：D 为 +X，A 使用 Negate；W 使用 Swizzle 转为 +Y，S 使用 Negate + Swizzle 转为 -Y。避免 E 同时触发两个上下马回调。组件实现后补充最终字段名和 Context 切换规则。

Horse Mesh 的 RiderSeat Socket 用于骑手附着；没有 Socket 时由后续代码提供可调 fallback transform 并输出警告。模型与 Anim Class 可暂缺，不能阻塞 P1 的胶囊体运动测试。

Content 其他预留目录：Steppe/UI、Audio、FX、Debug。未生成假的二进制资产。P1 试玩至少连续 10 分钟，检查加速、制动、转弯、体力、自由观察与镜头舒适度；未通过时继续 P1。
