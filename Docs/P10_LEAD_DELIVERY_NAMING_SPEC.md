# P10 牵回营地与命名规格

**状态：已实现并验证**
**依赖：P9**
**范围：从第一次接触到本轮命名结算**

## 玩家流程

1. 徒步安全接近后按一次 `E` 完成第一次接触并进入 `Leading`；徒步稳绳完成 `SURRENDER` 时自动进入 `Leading`。
2. 玩家徒步或骑自己的马慢速返回开局附近的 `CAMP / PEN`。目标通过 HorseBrain 向 HorseMovement 提交跟随意图，不直接改写位置。
3. 骑手和目标都进入交付区域后，目标进入 `Delivered` 并停止。
4. Horse Card 显示本轮身份、毛色、性别/年龄占位、性格以及现有 Gameplay 属性。
5. 玩家输入 1–16 个字符的名字并按 Enter 或点击确认，目标进入 `Named`，任务成功结算。
6. `F2` 开始新一轮，并清理牵行、交付、名字和命名 UI。

## 牵行规则

- 常规捕获后首次接触需下马完成；开始牵行后允许再上马，骑乘不取消 `Leading`。
- 目标跟在玩家或其坐骑后方约 2.8 m；距离大于 1.2 m 时以适应牵行者速度的步伐跟随。
- 骑乘牵行限制坐骑前进输入并禁用冲刺；野马落后超过约 12.6 m 时，坐骑停下等候。距离超过 18 m 时野马仍向牵行者移动，不传送。
- HorseBrain 继续使用既有地面与障碍方向探测；HorseMovement 是位移权威。
- 目标保持 Captured 登记，不重新加入活动马群。

## 营地与交付

- 营地位于出生点附近，使用可见灰盒围栏和 `CAMP / PEN` 世界标记。
- HUD 在牵行阶段显示到营地的距离。
- 只有当前牵行目标和交互骑手同时进入区域才交付，避免马或玩家单独越线完成。

## Horse Card 与命名

- 显示临时编号、性别、年龄、毛色与性格词。
- 能力值读取现有 HorseAttributeComponent：速度、耐力、力量与敏捷。
- 命名 UI 获得键盘焦点，屏蔽骑乘输入；确认或 F2 重玩后恢复 Gameplay 输入。
- 名字去除首尾空白，限制为 1–16 个字符；空名字不确认。
- P10 只保存当前 World 生命周期内的结果，不创建 SaveGame。

## 任务规则

- 计时持续到 `Named`，FirstContact 和 Delivered 都不是最终成功。
- 计分沿用 `捕获数 × 1000 + 向上取整的剩余秒数 × 10`。
- 超时仍以 0 分失败；已经交付但没有命名也会超时。

## 自动化验收

1. FirstContact 前不能开始牵行。
2. 玩家徒步开始牵行后可上马，野马保持跟随；坐骑冲刺被限制。
3. 牵行开始后 HorseBrain 持有有效跟随目标。
4. 距离过近时制动，距离适中时提交步行意图。
5. 骑手与马必须同时位于营地才交付。
6. 空名字被拒绝；有效名字只计数一次。
7. Trial 仅在 NamedCount 达标后成功。
8. 重载关卡清理本轮状态和 UI。

## 实际烟测验收

- 自动走完捕获、下马、第一次接触、建立牵行、目标自行移动、营地交付、Horse Card 和命名结算。
- 保存一张牵行过程截图和一张 Horse Card/命名完成截图。
- 日志记录 Lead、Delivered、Named、移动距离和 Trial Success。

## 非目标

P10 原阶段不实现跨启动存档、马厩容量、出售、奖励经济、正式角色动画、牵绳物理或多个目标同时交付。骑马牵行在后续反馈修复中补入。

## 实现结果

2026-09-13 已完成：

- `UHorseTrustComponent` 增加 Leading、Delivered、Named 与本轮身份记录。
- HorseBrain 在 Captured 分支中使用牵引锚点、距离阈值和既有环境探测提交步行/制动意图。
- `ASteppeDeliveryZone` 提供出生点附近的可见围栏和双 Actor 交付判定。
- 原生 UMG Horse Card 支持键盘输入、Enter/按钮确认与 F2 重玩，并在显示时接管输入焦点。
- Trial 记录 Secure、Contact、Deliver、Name 四段进度，仅在命名后结算。

验证：14 passed、0 failed；完整渲染烟测记录目标通过 Movement 移动 428.3 cm 后交付，命名 `Saran`，Trial Success。证据位于 `Validation/P10-Results.json`、`P10-Lead.png`、`P10-HorseCard.png`、`P10-Named.png` 和 `P10-Runs.txt`。

2026-09-20 反馈修订：首次 `E` 自动接起牵绳；牵行后上马不会丢失目标，E 键可继续作用于自己的坐骑。HorseBrain 跟随坐骑位置，RidingComponent 限速并等候落后的野马。24 项 Steppe 自动化测试及安静马群、捕获后接触、完整交付烟测通过。
