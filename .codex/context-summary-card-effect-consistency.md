## 项目上下文摘要（卡牌实际效果与描述一致性）

生成时间：2026-09-05 16:20:00 +08:00

### 1. 相似实现分析

- **实现1**：`src/card/CardDatabase.cpp`
  - 卡牌定义已经通过 `CardEffectType` 和 `parameter` 描述基础效果、触发时机和特殊规则。
  - 关键参数包括 `player_block`、`hits_N`、`end_turn`、`on_exhaust`、`on_status_draw`、`x_cost`、`strike_count` 等。
  - 当前缺口是参数没有统一解释器，而不是卡牌数据缺失。
- **实现2**：`src/combat/CombatSystem.cpp`
  - `playCard()` 负责能量、牌堆移动和逐效果结算，适合作为唯一卡牌执行入口。
  - `resolveEffect()` 目前只实现少量 `CardEffectType`，其余走 `default` 丢弃。
  - `endPlayerTurn()` 已是回合边界，适合接入 `end_turn` 和 `start_turn` 触发。
- **实现3**：`src/card/Deck.cpp`
  - 已有抽牌、弃牌、消耗、弃手牌和向弃牌堆加入卡牌接口。
  - 需要补充“牌堆顶移动”和“消耗堆取回”等小接口，避免在战斗系统直接修改私有容器。
- **实现4**：`src/combat/Player.cpp` 与 `src/combat/Enemy.cpp`
  - 玩家和敌人分别持有血量、格挡、力量、虚弱、易伤等规则状态。
  - `takeDamage()` 当前返回 `void`，无法支持收割的未被格挡伤害治疗、火焰屏障反击和生命损失触发；可改为返回实际未被格挡/实际损失值，现有调用无需改变。

### 2. 项目约定

- 规则逻辑集中在 `src/combat` 和 `src/card`，UI 只通过 `CombatSystem` 公共接口读写。
- 类型使用大驼峰，函数和变量使用小驼峰，常量使用 `k` 前缀。
- C++17、SFML 3.0.1、`.hpp/.cpp` 分离，测试为 CMake/CTest 下的独立断言程序。
- 卡牌 X 费用使用 `cost == -1`，不可打出的状态牌使用更小的负值，避免把 X 费用误判为不可打出。

### 3. 可复用组件清单

- `CardEffectType` / `CardEffect::parameter`：现有卡牌效果协议。
- `Deck::drawCards()`、`discardCard()`、`exhaustCard()`：现有牌堆生命周期。
- `Player` / `Enemy`：现有战斗状态和伤害修正。
- `CardDatabase::createIroncladCardPool()`：所有卡牌定义与升级效果来源。
- `CombatTests`：现有基础出牌、状态、敌人意图和牌堆注入测试。

### 4. 测试策略

- 保留现有 CTest 全量测试。
- 在 `tests/combat_tests.cpp` 增加基础效果、特殊参数、回合触发、消耗触发、X 费用和多次命中测试。
- 覆盖正常流程、能量不足、不可打出的状态牌、空牌堆/空消耗堆和敌人死亡边界。
- UI 不直接验证规则，使用 `CombatSystem::getPlayableCardCost()` 等公共接口确认交互层可正确识别 X 费用。

### 5. 依赖和集成点

- `CombatSystem` 依赖 `Deck`、`Player`、`Enemy`、`CardDatabase`。
- `BattleView` 依赖 `CombatSystem::playCard()` 和玩家能量；需要同步 X 费用判定。
- `Game` 和地图/奖励流程不需要改变。
- 不新增外部依赖，不改变 CMake 目标结构。

### 6. 技术选型理由

- 继续使用已有 `CardEffectType + parameter` 协议，在 `CombatSystem` 内集中解释，避免为每张卡牌新增独立分支类。
- 用少量持久状态表示能力牌和回合触发，保持规则与视觉分离。
- 牌堆新增最小操作接口，由 `Deck` 负责容器不变量，战斗系统只组合这些接口。
- 选择类效果暂按当前项目无目标选择 UI 的约束自动选取第一张合法牌，并确保实际结果与描述的数量、区域和时机一致。

### 7. 关键风险点

- 多次命中与双发可能重复结算消耗/增伤，需要区分“卡牌移入牌堆一次”和“效果执行多次”。
- 消耗触发可能连锁抽牌，需通过统一抽牌/消耗辅助函数避免遗漏。
- 敌人死亡后不能继续执行同一张牌的后续攻击，但应保留已经发生的前置效果。
- `CardView` 当前将所有负费用显示为 X，UI 需要只把 `-1` 作为 X。

### 8. 编码前检查

□ 已查阅上下文摘要文件：`.codex/context-summary-card-effect-consistency.md`
□ 将使用既有组件：`CardEffect` 协议、`Deck` 牌堆接口、`Player`/`Enemy` 状态、`CombatSystem` 统一结算入口、`CombatTests` 测试模式。
□ 将遵循命名约定：类型大驼峰、函数和变量小驼峰、常量 `k` 前缀。
□ 将遵循代码风格：C++17、SFML 3.0.1、中文注释和测试输出。
□ 确认不重复造轮子：已检查 `CardDatabase`、`CombatSystem`、`Deck`、`Player`、`Enemy`，本轮扩展现有效果协议，不另建卡牌规则系统。
□ 工具限制：本会话没有 sequential-thinking、desktop-commander、context7、shrimp-task-manager 和 github.search_code 可调用入口，使用本地代码检索和 CTest 替代并记录。

### 9. 本轮完成后的结论

- `Card::upgrade()` 已成为卡牌升级的统一入口，普通升级会同步所有展示和结算字段；
  `repeatable_upgrade` 卡牌会累计升级等级，目前支持`灼热打击`每次增加 4 点伤害。
- `CombatSystem::resolveEffect()` 已覆盖 `Heal` 和 `UpgradeCard`，并继续由已有
  `CardEffect` 协议解释卡牌规则。
- `Deck` 已提供按索引升级手牌、升级全部手牌以及指定弃牌回收接口。
- `头槌`、`发掘`、`愤怒`和`虚无`的自引用/遗漏副作用已修正。
- `CardView` 只把 `cost == -1` 显示为 `X`，多段伤害牌会显示首段伤害值。
- 规则验证已扩展到升级、治疗、虚无、弃牌回收、X费用、第二风、肌肉强化和碰撞等场景。
