# 《套马的汉子》 / Project STEPPE

目标是让骑手向马提出意图，由马的响应和运动系统决定结果。当前工作范围仅 P0 工程初始化与 P1 骑乘运动；不实现野马 AI、马群、套索等后续玩法。

目标引擎：**Unreal Engine 5.8**。2026-09-10 用户确认保留此版本，不改用本机 UE 5.7.4。

当前：P0 源码骨架已建立，**构建与 Editor 验收阻塞**。P1 尚未开始，当前工程还不能骑马。

## 构建与运行

安装 UE 5.8 及其要求的 Visual Studio C++ 工具链后，在项目根目录执行：

```powershell
.\Scripts\Build.ps1 -EngineRoot 'D:\Program Files\Epic Games\UE_5.8'
```

路径为示例，应替换为真实引擎目录。脚本检查引擎版本，拒绝使用其他版本。构建成功后用 UE 5.8 打开 `Steppe.uproject`，检查 Output Log 中模块加载情况与错误。完整 Editor 步骤见 `Docs/EDITOR_SETUP.md`。

## 当前操作与调试

P0 尚无骑乘按键。控制台可用 `steppe.Debug.Horse 1`、`steppe.Debug.Movement 1` 设置调试开关，`SteppeToggleDebug` 切换 Horse 开关；遥测与向量绘制将在 P1 实现。

P1 计划按键：W/S 前进意图/减速，A/D 转向，鼠标自由观察，Shift 冲刺，Ctrl 制动，E 上下马，F1 调试。这些绑定尚未实现。

## 文件

- `Source/Steppe/`：主模块、GameMode、PlayerController、Native Tags、日志、调试 Subsystem。
- `Config/`：默认 GameMode、Enhanced Input 类及项目描述。
- `Scripts/Build.ps1`：版本检查与 Editor 编译入口。
- `Docs/`：架构、Editor 设置、进度、验收清单、原始需求副本。

没有创建任何 `.uasset` 或 `.umap`，也没有初始化或修改 Git 数据库。当前不提供可玩关卡、马模型或动画。
