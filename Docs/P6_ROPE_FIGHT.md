# P6 绳索对抗原型

P6 把 P5 的套索附着扩展为短时控马对抗。目标套中后会向绳索锚点反方向挣扎；玩家需要用坐骑位置控制绳距，并按住空格稳绳。该阶段结束于 `SUBDUED`，不执行 P7 捕获。

## 规则

- 套索刚附着时记录当前绳长，并保留约 120 cm 的预张力。
- 张力由绳距超出初始长度的程度和目标相对远离速度共同计算。
- 按住空格时，目标降低挣扎速度。张力处于 20%–85% 时累计控制进度；过松、过紧或松开空格都会使进度衰减。
- 有效张力保持约 3 秒后进入 `Lasso.State.Subdued`，HUD 显示 `SUBDUED - ready for capture in P7`。
- 张力超过 100% 持续 0.35 秒，或两端距离超过最大射程的 110%，绳索断开并进入 Recovering。
- Subdued 目标通过 HorseBrain 向现有 HorseMovement 提交制动意图；左键仍可主动释放。

HUD 右上角显示当前套索状态、张力百分比、STEADY/ADJUST、控制百分比与进度条。张力和完成状态属于 Gameplay 数据，调试绳线只负责显示。

## 试玩

1. 完成 P4 隔离并用 P5 套中目标。
2. 套中后立即按住空格。让张力保持绿色 STEADY；目标靠近造成过松时适当拉开，张力上升时向目标靠近。
3. 保持约 3 秒，直到右上角显示 `LASSO Subdued` 和 `CONTROL 100%`。
4. 可再次按左键释放；也可故意快速远离，验证高张力断绳和自动回收。

## 验证

`Steppe.P6.RopeFightTensionAndSubdue` 覆盖附着起点、空格稳绳、有效张力完成控制、Subdued Gameplay Tag、目标保持受控、释放后再次使用，以及绳距过长断绳并释放目标。

```powershell
.\Scripts\RunEditor.ps1 -Tests -ExpectedTests 10 -Commands 'Automation RunTests Steppe' -LogName P6-Automation
.\Scripts\RunEditor.ps1 -Game -Render -Smoke -RopeFightSmoke -Commands 'steppe.Debug.Movement 1' -LogName P6-FinalRender
```

2026-09-12 最终结果：10 passed、0 failed。实际渲染中 H1 隔离距离 2152.5 cm，套索张力 33%，控制进度 100%，状态为 Subdued，目标仍处于 Lassoed。证据位于 `Validation/P6-Results.json`、`P6-Playground.png` 和 `P6-Runs.txt`。

当前对抗仍是灰盒手感原型。没有手部动画、绳索物理波动、骑手受力、复杂耐力博弈或捕获结果；这些应分别作为表现完善和 P7 工作处理。
