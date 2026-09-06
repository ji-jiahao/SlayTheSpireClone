## 项目上下文摘要（combat 分支功能融入 main 与结束回合音效）

生成时间：2026-09-05 13:20:00 +08:00

### 1. 相似实现分析

- **实现1**：`src/combat/CombatSystem.cpp`
  - 模式：规则层集中处理出牌、回合结束、伤害和牌堆变化。
  - 可复用：现有 `resolveEffect()`、`calculatePlayerDamage()`、`calculateEnemyDamage()` 和 `startBattle()`。
  - 需注意：主线当前是单敌人 UI，因此只迁移单敌人意图与特殊行为，不迁移分支 README 中未实际落地的多敌人接口。
- **实现2**：`src/combat/Enemy.cpp`
  - 模式：敌人自身维护状态和意图，`CombatSystem` 只负责按意图结算。
  - 可复用：`EnemyIntent`、敌人原型枚举、意图推进、格挡、沉睡/分裂/黏液等行为。
  - 需注意：默认遭遇需要增加 `enemyId`，旧的通用敌人仍保留固定伤害意图。
- **实现3**：`src/ui/BattleView.cpp`
  - 模式：视图层处理鼠标输入、动画和音效，规则层通过公开接口更新战斗状态。
  - 可复用：`loadCardSounds()`、`playCardSound()`、`getEndTurnButtonBounds()`、按钮绘制路径。
  - 需注意：音效不能放在绘制函数中，只在按钮点击确认时播放；结束回合按钮继续使用固定逻辑坐标。
- **实现4**：`src/ui/MainMenuView.cpp` 与 `src/ui/UiHelpers.cpp`
  - 模式：鼠标移动记录按钮高亮状态，绘制时按高亮状态切换按钮颜色。
  - 可复用：`UiHelpers::contains()` 和按钮高亮的状态管理方式，用于结束回合按钮的交互反馈。

### 2. 项目约定

- **命名约定**：类型使用大驼峰，函数和成员变量使用小驼峰，常量使用 `k` 前缀。
- **文件组织**：规则位于 `src/combat`，牌堆位于 `src/card`，界面位于 `src/ui`，测试位于 `tests`。
- **代码风格**：C++17、`.hpp/.cpp` 分离、SFML 3.0.1，中文界面文案，MSVC 使用 UTF-8 编译选项。
- **资源约定**：运行时从 `assets/...` 读取，CMake 在构建后复制整个 `assets` 目录。

### 3. 可复用组件清单

- `src/combat/Enemy.hpp/.cpp`：敌人意图和特殊行为。
- `src/combat/Player.hpp/.cpp`：脆弱、敏捷以及受脆弱影响的卡牌格挡。
- `src/combat/CombatSystem.hpp/.cpp`：敌人意图结算和战斗流程。
- `src/card/Deck.hpp/.cpp`：把状态牌注入弃牌堆。
- `src/ui/BattleView.hpp/.cpp`：战斗按钮输入、音效加载和敌人意图显示。
- `src/ui/UiHelpers.hpp/.cpp`：按钮区域命中检测。

### 4. 测试策略

- **测试框架**：CMake/CTest 下的独立 C++ 可执行测试，主要使用 `assert`，部分 UI 数学测试使用异常包装的 `require()`。
- **参考文件**：`tests/combat_tests.cpp`、`tests/battle_hover_tests.cpp`、`tests/battle_cast_tests.cpp`。
- **覆盖要求**：保留主线基础战斗测试，并增加敌人意图、回合推进、玩家脆弱/敏捷、敌人格挡、黏液、沉睡、分裂和死亡效果测试。
- **验证方式**：本地 CMake 配置、Debug 构建、CTest 全量测试和短时启动冒烟。

### 5. 依赖和集成点

- **外部依赖**：SFML 3.0.1 Graphics、Audio。
- **内部依赖**：`combat_system` 依赖 `card_system`；`ui` 依赖 `combat_system` 和 SFML Audio。
- **集成方式**：`Game` 把鼠标事件和帧更新转发给 `BattleView`；`BattleView` 调用 `CombatSystem` 公开接口。
- **配置来源**：资源路径固定为 `assets/sounds/end_turn.mp3`，CMake 已复制整个资源目录。

### 6. 技术选型理由

- **为什么用这个方案**：逐文件迁移分支的真实功能差异，保留 main 已有地图、事件、商店和战斗 UI，降低交叉合并范围。
- **优势**：规则与 UI 边界清晰，旧的单敌人调用方式继续可用，新增敌人只需通过 `EncounterDefinition.enemyId` 选择。
- **劣势和风险**：当前主线仍是单敌人展示，分裂以单个共享血量敌人表示；多敌人需后续单独设计 UI 和规则接口。

### 7. 关键风险点

- **状态边界**：敌人意图必须在回合结束结算后推进，沉睡、分裂等特殊意图不能重复推进。
- **数值边界**：伤害、格挡、状态层数和血量均需保持非负；卡牌格挡经过敏捷和脆弱修正后仍需截断。
- **性能瓶颈**：敌人意图为轻量值对象；结束回合音效只加载一次，不在每帧创建或加载资源。
- **资源风险**：外部 MP3 复制到 `assets/sounds/end_turn.mp3` 后由 CMake 统一复制到运行目录。

### 8. 编码前充分性检查

- 已能定位至少 3 个相似实现：`src/combat/CombatSystem.cpp`、`src/combat/Enemy.cpp`、`src/ui/BattleView.cpp`、`src/ui/MainMenuView.cpp`。
- 已理解项目模式：规则层结算、视图层输入和音效、辅助模块提供纯逻辑。
- 已确认复用组件：`BattleView::loadCardSounds()`、`getEndTurnButtonBounds()`、`UiHelpers::contains()`、`Deck` 牌堆接口。
- 已确认测试入口：CMake/CTest 下的 `CombatTests` 及全量测试。
- 未发现需要新增第三方依赖；本会话没有可直接调用的 sequential-thinking、shrimp-task-manager、desktop-commander、context7 和 github.search_code 入口，已改用仓库内代码、分支差异和本地 CMake 流程完成等价检索并留痕。
