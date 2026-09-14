# P5 套索原型

P5 在已完成的目标切出上加入了可玩的套索闭环。套索的命中由 `ULassoComponent` 的连续球形扫掠决定，HUD 绳线只负责表现，不参与 Gameplay 判定。

## 操作与规则

1. 用 Q 选择一匹野马，并把它切到距其余马群中心至少 10 米、保持 1 秒，直到 HUD 显示 `ISOLATED`。
2. 选中后即可按住鼠标右键进入 Aiming。目标尚未隔离时显示灰色环，超过射程时显示红色环；两项条件都满足后才出现绿色投掷窗口。
3. 保持准星对准目标，在 26 米内按鼠标左键投掷。套索速度为 3200 cm/s，命中半径 80 cm。
4. 命中后进入 Attached，目标进入 `Horse.State.Lassoed` 并通过原有 HorseMovement 紧急制动。
5. Attached 时按鼠标左键释放。脱靶、击中障碍、目标消失或绳长超过限制都会进入 1.5 秒 Recovering，让失败原因能够被看清，随后重新可用。

状态为 Stored / Aiming / Thrown / Attached / Recovering，并分别暴露 `Lasso.State.*` Gameplay Tags。投掷会被 Pawn、WorldStatic 和 WorldDynamic 阻挡；不依赖临时动画资产。

## 验证

`Steppe.P5.LassoThrowAttachAndRecovery` 覆盖未隔离时的可见瞄准和投掷锁定、投掷扫掠、目标命中、Lassoed 行为、Gameplay Tag、主动释放、脱靶和自动恢复。

```powershell
.\Scripts\RunEditor.ps1 -Tests -ExpectedTests 9 -Commands 'Automation RunTests Steppe' -LogName P5-FinalTests
.\Scripts\RunEditor.ps1 -Game -Render -Smoke -LassoSmoke -Commands 'steppe.Debug.Movement 1' -LogName P5-FinalRender
```

2026-09-12 最终结果：9 passed、0 failed；实际渲染中 H1 与其余马群中心相距 2152.5 cm，隔离完成，套索进入 Attached，目标 `bLassoed=1`。证据位于 `Validation/P5-Results.json`、`P5-Playground.png` 和 `P5-Runs.txt`。

当前附着效果是玩法原型：野马被制动，绳线随两端更新。尚未实现摆绳蓄力、绳圈骨骼动画、物理绳摆动、拖拽对抗、驯服或捕获奖励。
