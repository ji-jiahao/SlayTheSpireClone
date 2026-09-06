## 项目上下文摘要（事件背景与回合末结算）

生成时间：2026-09-05

### 1. 相似实现分析

- `src/ui/EventView.cpp`：事件背景由 `EventDefinition::backgroundPath` 加载并铺满窗口。
- `assets/data/events.json`：奶龙事件 `sacred_nailong` 和大学事件
  `university_choice` 通过 `background` 字段指定背景。
- `src/combat/CombatSystem.cpp`：`endPlayerTurn()` 统一处理玩家回合结束、虚无、
  敌人行动和下一回合开始效果。

### 2. 复用接口

- `EventDatabase::loadFromFile()`：验证事件数据和背景路径。
- `EventDatabase::getEvent()`：验证两个事件使用同一张新背景。
- `CombatSystem::resolvePlayerEndTurnEffects()`：处理金属化、燃烧等持久效果。
- `tests/combat_tests.cpp`：沿用现有战斗断言测试模式。

### 3. 本轮规则结论

- 金属化在敌人行动前提供格挡。
- 狂怒和双发属于“本回合”效果，回合结束后清零。
- 灼伤在回合结束时失去生命并消耗。
- 事件背景统一使用 `assets/images/event/torch_stone_event_background.png`。

### 4. 验证方式

- Debug 构建。
- 全量 CTest，包含新增 `EventTests`。
- `git diff --check`。
- 主程序 3 秒启动冒烟。
