## 项目上下文摘要（愤怒移除与战斗快照修复）

生成时间：2026-09-06 12:26:15 +08:00

### 1. 相似实现分析

- **实现1**: `src/card/CardDatabase.cpp`
  - 模式：集中式卡牌定义，`createIroncladCardPool()` 返回完整卡池，`createById()` 按 ID 查找。
  - 可复用：`makeCard()`、`attack()`、`skill()`。
  - 需注意：卡池数量由 `tests/card_tests.cpp` 固定校验。
- **实现2**: `src/combat/CombatSystem.cpp`
  - 模式：`playCard()` 统一扣费、移动卡牌，再由 `resolveCardEffects()` 分发效果。
  - 可复用：`createStatusCard()`、`resolveEffect()`、`captureSafeSnapshot()`。
  - 需注意：生成状态牌和复制牌不能与正式卡牌 ID 产生语义冲突。
- **实现3**: `tests/combat_tests.cpp`
  - 模式：使用固定种子、固定牌堆和 `assert` 验证战斗规则。
  - 可复用：`fiveCopies()`、`findCard()`、`pileContains()`。
  - 需注意：测试应保持静默，只在最终输出通过信息。

### 2. 项目约定

- **命名约定**: C++ 类型使用 PascalCase，函数和变量使用 lowerCamelCase，卡牌 ID 使用 snake_case。
- **文件组织**: 卡牌定义在 `src/card`，战斗规则在 `src/combat`，视图在 `src/ui`，测试在 `tests`。
- **导入顺序**: 先引入项目头文件，再引入标准库头文件。
- **代码风格**: C++17，花括号独立成块，中文用户可见文本和注释。

### 3. 可复用组件清单

- `CardDatabase::createIroncladCardPool()`：奖励、商店和测试使用的统一卡池。
- `CombatSystem::captureSafeSnapshot()`：贝利亚一次性复活使用的战斗快照入口。
- `CombatSystem::reviveFromLastSafeSnapshot()`：恢复战斗状态并附加复活增益。
- `Deck`：维护抽牌堆、手牌、弃牌堆、消耗堆。

### 4. 测试策略

- **测试框架**: CTest 调度的 C++ 可执行测试，内部使用 `assert`。
- **测试模式**: 单元级规则测试和启动冒烟。
- **参考文件**: `tests/card_tests.cpp`、`tests/combat_tests.cpp`。
- **覆盖要求**: 卡池数量、卡牌 ID 可创建、战斗快照复活、能量回合边界、敌人特殊机制。

### 5. 依赖和集成点

- **外部依赖**: C++17 标准库、SFML 3、MSVC/CMake。
- **内部依赖**: `CombatSystem` 依赖 `CardDatabase`、`Deck`、`Player`、`Enemy`。
- **集成方式**: `Game` 调用 `CombatSystem` 进入战斗、结算战斗结果和贝利亚复活。
- **配置来源**: CMake presets 与项目资源目录。

### 6. 技术选型理由

- **为什么删除愤怒**: `愤怒` 同时作为正式卡牌和 `makeStatusCard()` 特殊生成牌存在，复制出来的牌与卡库定义效果不一致，会造成描述与实际逻辑冲突。
- **为什么快照上堆**: `CombatSystem` 原先用 `std::optional<BattleSnapshot>` 内嵌完整快照，导致栈上 `CombatSystem` 过重；改为 `std::unique_ptr<BattleSnapshot>` 后按需分配，测试中栈压力段错误消失。
- **优势**: 不改变复活接口语义，降低栈占用，移除冲突卡牌入口。
- **劣势和风险**: 卡池从 74 张降为 73 张，后续若需要恢复 `愤怒`，必须把复制牌实现改成从 `CardDatabase` 复制完整定义并补测试。

### 7. 关键风险点

- **并发问题**: 当前战斗系统单线程，无并发风险。
- **边界条件**: 旧存档若引用 `anger`，`createById("anger")` 会失败；当前项目尚无真实读档系统。
- **性能瓶颈**: 快照改堆会增加一次堆分配，但只在战斗安全点更新，规模可接受。
- **安全考虑**: 无外部输入安全边界变更。
