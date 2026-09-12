# P8 限时捕获垂直切片

P8 将已有的骑乘、驱赶、切出、套索、控绳和捕获串成一轮可开始、可完成、可失败、可重试的灰盒任务。开局目标是在 120 秒内捕获 1 匹野马。

## 任务规则

- 玩家进入关卡后任务自动开始，HUD 左下角显示捕获进度、剩余时间和积分。
- 捕获数量达到目标时立即成功；时间归零且目标未完成时失败。
- 结算后计时与积分冻结，中央显示成功或超时结果；`F2` 重新加载场景并开始新一轮。
- 积分为 `捕获数 × 1000 + 成功时向上取整的剩余秒数 × 10`。失败得 0 分。
- 同一帧既发生捕获又到达时限时，捕获优先，任务判定成功。

## 完整试玩流程

1. 骑马接近五匹野马，以 `Q` 选择目标。
2. 将目标赶到距离其余马群中心至少 18 米的位置并保持 2 秒。
3. 按住鼠标右键瞄准，以鼠标左键投掷套索。
4. 命中后按住空格，将张力维持在 20%–85%，直到目标进入 `Subdued`。
5. 按 `C` 捕获目标，查看成功结算和积分；按 `F2` 重玩。

## 验证

`Steppe.P8.TimedMissionRules` 覆盖任务开始、倒计时、部分进度、成功、失败、结算冻结、积分公式和最后一帧捕获。既有 P1–P7 测试继续覆盖实际玩法链路。

```powershell
.\Scripts\RunEditor.ps1 -Tests -ExpectedTests 12 -Commands 'Automation RunTests Steppe' -LogName P8-Automation
.\Scripts\RunEditor.ps1 -Game -Render -Smoke -VerticalSliceSmoke -Commands 'steppe.Debug.Movement 1' -LogName P8-FinalRender
```

2026-09-13 最终结果：12 passed、0 failed。实际关卡完成投索、控绳和捕获后，任务记录 `Success`、`Captured=1/1`、剩余 114.8 秒、积分 2150；活动马群剩余 4 匹。证据位于 `Validation/P8-Results.json`、`P8-Playground.png` 和 `P8-Runs.txt`。

当前任务状态仅存在于本轮关卡，不包含跨局存档、马匹品质、奖励经济、营地交付、正式 UI/音效或多人网络。
