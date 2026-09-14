# P4 目标切出原型

本轮建立了“从马群中选定一匹并把它切离群体”的最小玩法闭环。P14.4 调整后，玩家朝向一匹野马按 `Q`，系统从前方 35 米、视线夹角范围内选择最合适的成员；再次对同一目标按 `Q` 可取消，目标离玩家超过 42 米时自动失去关注。

被选中的马继续使用自己的感知、状态机、障碍探测、动态避让和 CMC 运动，但暂时不接受群体中心与群体平均方向的牵引，仍保留成员间分离力。这使玩家可以用坐骑从侧后方施压，把目标逐步切出，同时避免目标穿过其他马匹。

## 完成条件与反馈

- 默认要求目标距其余马群中心至少 10 米。
- 在距离外持续 1 秒完成隔离；回到距离内时进度以双倍速度衰减。
- HUD 显示目标编号、分离距离、隔离进度和 `ISOLATED` 状态。
- 世界标签用青色 `TARGET H#` 标记目标；开启 `steppe.Debug.Movement 1` 后显示目标圈和目标到其余马群中心的连线。
- F2 重试会随关卡一起清除目标与进度。

## 试玩

1. Play 后用鼠标看向一匹野马并按 `Q`，确认其标签变成青色 `TARGET H#`。
2. 从目标靠近群体的一侧或侧后方施压，使它与其余四匹马分开。
3. 观察 HUD 的 separation；超过 10.0 m 后保持 1 秒，直到显示 `ISOLATED`。
4. 让目标回群，确认完成前进度会下降；再次朝目标按 `Q`，确认取消锁定。

## 自动验证

`Steppe.P4.TargetSelectionAndIsolation` 覆盖视线选马、Brain 隔离标记、距离外进度、回群衰减、完成锁存、重复按键取消，以及目标销毁后的安全释放。全套命令：

```powershell
.\Scripts\RunEditor.ps1 -Tests -ExpectedTests 8 -Commands 'Automation RunTests Steppe' -LogName P4-Automation
.\Scripts\RunEditor.ps1 -Game -Render -Smoke -IsolationSmoke -Commands 'steppe.Debug.Movement 1' -LogName P4-FinalRender
```

2026-09-12 实际结果：8 passed、0 failed；实际渲染成功选中 H3。截图时 `Distance=1012.0 cm`、`Progress=0.00`、`Isolated=0`，符合尚未越过 18 米阈值的状态。证据位于 `Validation/P4-Results.json`、`P4-Playground.png` 和 `P4-Runs.txt`。

当前只完成目标切出。尚未实现套索投掷、绳索物理、套中判定、驯服或捕获奖励。
