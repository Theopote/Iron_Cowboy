# 文档索引

## 建议阅读顺序

1. `GAME_DESIGN_VISION.md`：长期产品方向与设计支柱。
2. `DESIGN_BASELINE.md`：当前事实、早期设想与现状的差异、文档权威顺序。
3. `PROTOTYPE_GDD.md`：当前规则与下一版 6–10 分钟完整流程。
4. `DEVELOPMENT_ROADMAP.md`：P9–P14 的执行顺序和退出条件。
5. `CORE_GAMEPLAY_AUDIT.md`：当前套索、马匹动力学和代码边界审计，以及 P14.3–P17 阶段门。
6. `P12_LASSO_SKILL_SPEC.md`：当前套索摆绳、稳定性、命中区域、骑手平衡、落马与拖行规格。
7. `P13_AUDIO_VISUAL_SPEC.md`：当前反馈信号、程序化占位音、马蹄/尘土与绳索风险表现规格。
8. `P14_PLAYTEST_VALIDATION.md`：逐轮指标字段、人工试玩步骤、问题分类和 Go 条件。
9. `P11_HORSE_ARCHETYPES_SPEC.md`：三类野马原型及 Gameplay 接入规格。
10. `P10_LEAD_DELIVERY_NAMING_SPEC.md`：完整闭环的牵行、交付和命名规格。
11. `DESIGN_DECISIONS.md`：已确认决策、待决事项和变更记录。

## 当前开发文档

- GAME_DESIGN_VISION.md：玩家幻想、核心循环、三个支柱和长期候选方向。
- DESIGN_BASELINE.md：当前可玩基线、实现差距和近期目标。
- PROTOTYPE_GDD.md：玩法规则、完整试玩流程、范围和原型验收指标。
- DEVELOPMENT_ROADMAP.md：P9–P14 里程碑与验证门。
- P9_CAPTURE_AFTERMATH_SPEC.md：已实现的安全接近、拒绝恢复与第一次接触规则。
- P10_LEAD_DELIVERY_NAMING_SPEC.md：已实现的牵回营地、Horse Card、命名和最终结算规则。
- P11_HORSE_ARCHETYPES_SPEC.md：已实现的 Fast、Strong、Nervous 原型、参数接入和验证结果。
- P12_LASSO_SKILL_SPEC.md：已实现的摆绳/命中区、Rider Balance、落马与短距离拖行规则。
- P13_AUDIO_VISUAL_SPEC.md：已实现的 P13.1–P13.3 反馈、双地表、资源插槽、动画姿态和 UI 规格。
- P14_PLAYTEST_VALIDATION.md：已实现的逐轮 JSON、数据字典、人工验证流程和决策门槛。
- CORE_GAMEPLAY_AUDIT.md：已核实的核心体验技术债和 P14.3–P17 顺序。
- Playtests/：匿名测试者说明、15 轮记录表和访谈模板。
- P14.3_PACKAGING_AND_DISTRIBUTION.md：Win64 Shipping 一键打包、ZIP 分发和结果回收方法。
- P14.3_GAMEPLAY_RESEARCH_REVIEW.md：累计真人反馈、阶段门结论与数据限制。
- P15_PHYSICAL_LASSO_V2.md：三维绳圈、实际几何命中、动态绳线和当前限制。
- P16_HORSE_DYNAMICS_V2.md：朝向/速度分离、地面抓地与绳索外力的实施基线。
- DESIGN_DECISIONS.md：跨系统技术与产品决策。
- References/：两份早期讨论 PDF 原件及使用说明。

## 实现与历史记录

- MASTER_PROMPT_v0.1.md：原始 P0 + P1 范围，保留历史版本。
- P1_BASELINE.md：进入 P2 前的 P1 实现与验证记录。
- P2_WILD_HORSE.md：已授权 P2 单匹野马的架构、试玩、参数与限制。
- P3_SMALL_HERD.md：P3 初始五匹马群的局部聚散、报警传播与历史验证；当前 P14.4 默认规模为十二匹。
- P4_TARGET_ISOLATION.md：P4 视线选马、目标切出进度与验证。
- P5_LASSO.md：P5 套索瞄准、投掷、命中与失败回收。
- P6_ROPE_FIGHT.md：P6 套中后的张力管理、稳绳与控马完成。
- P7_CAPTURE.md：P7 捕获确认、马群移除、计数与结果状态。
- P8_VERTICAL_SLICE.md：P8–P8.2 限时任务、阶段引导、紧迫提示、成功/失败结算、积分与整轮重试。
- ARCHITECTURE.md：意图分层、组件职责、Tick 顺序。
- EDITOR_SETUP.md：资产路径、按键和人工 Editor 步骤。
- DEVELOPMENT_STATUS.md：当前状态与最新验证。
- Validation/：实际运行截图与精简结果。

当前使用本机 UE 5.8.2。P14.3 真人阶段门已由项目负责人依据累计反馈确认通过，P14.4 高频 Core Feel 修复已完成；当前进入 P15 Physical Lasso v2。
