# P7 捕获结果原型

P7 为 P4–P6 的追逐、切出、套索和控马链路增加明确结果。目标进入 Subdued 后按 `C` 完成捕获；系统记录捕获、将目标从活动马群移除，并保留它在场景中显示 `Captured` 结果。

## 规则

- 只有当前套索目标处于 `Lasso.State.Subdued` 时，C 才能成功。
- 成功后套索进入 `Lasso.State.Captured`，野马进入 `Horse.State.Captured`。
- HerdManager 将目标从 `Members` 移到 `CapturedHorses`，清除旧焦点并增加 `CapturedCount`；目标不再参与群体中心、邻居、分离或报警传播。
- Captured 野马通过原 HorseMovement 保持制动，Actor 暂时留在场景中作为结果反馈。
- HUD 显示活动数量、捕获数量、`CAPTURED H#` 和 `CAPTURE COMPLETE`。
- 捕获后按鼠标左键收回套索不会撤销捕获；套索恢复后可选择剩余马匹。F2 重试会重建整轮状态。

## 完整试玩流程

1. Q 选择一匹野马，将其切到距离马群中心 18 米外并保持 2 秒。
2. 按住 RMB 瞄准，LMB 投掷套索。
3. 命中后按住空格，让张力保持在 20%–85%，直到 CONTROL 达到 100%。
4. 出现 Subdued 后按 C，确认 HUD 显示 `CAPTURE COMPLETE`，活动马群减少一匹。
5. 按 LMB 收回套索，再用 Q 选择剩余目标；或按 F2 重置整轮。

## 验证

`Steppe.P7.CaptureSubduedHorse` 覆盖过早捕获拒绝、完整 Subdued 前置、捕获登记、Lasso/Horse Gameplay Tags、活动成员移除、捕获计数、结果对象保留、焦点清理、套索收回及捕获结果持久性。

```powershell
.\Scripts\RunEditor.ps1 -Tests -ExpectedTests 11 -Commands 'Automation RunTests Steppe' -LogName P7-Automation
.\Scripts\RunEditor.ps1 -Game -Render -Smoke -CaptureSmoke -Commands 'steppe.Debug.Movement 1' -LogName P7-FinalRender
```

2026-09-12 最终结果：11 passed、0 failed。五匹实际关卡中成功捕获 H1，记录为 `Captured=1`、`Active=4`、`TargetState=Captured`。证据位于 `Validation/P7-Results.json`、`P7-Playground.png` 和 `P7-Runs.txt`。

当前结果只记录本轮捕获，不包含持久存档、马匹品质、奖励结算、营地交付、任务系统或经济系统。这些属于 P8 纵向切片和后续产品设计。
