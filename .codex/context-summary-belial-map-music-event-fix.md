## 项目上下文摘要（贝利亚战斗、地图路线与战斗音乐修复）

生成时间：2026-09-09 23:40:00 +08:00

### 1. 相似实现分析

- **战斗回合推进**：`src/combat/CombatSystem.cpp:265-327`
  - 模式：玩家结束回合后依次结算玩家回合末效果、敌人行动、敌人回合末效果、推进敌人意图、玩家回合开始效果和抽牌。
  - 可复用：`resolveEnemyIntent()`、`resolvePlayerStartTurnEffects()`、`captureSafeSnapshot()`。
  - 需注意：致命伤害可能发生在敌人多段攻击中途，复活必须从致命伤害前的快照开启完整新回合。
- **贝利亚意图与伤害**：`src/combat/Enemy.cpp:205-240`、`src/combat/CombatSystem.cpp:940-988`
  - 模式：敌人意图存放基础伤害与段数，`Enemy::getIntentDamage()`处理贝利亚黑暗蓄能倍率，战斗系统再统一应用力量、虚弱和易伤。
  - 可复用：`Enemy::getIntentDamage()`、`CombatSystem::getEnemyIntentDamage()`。
  - 需注意：黑暗侵蚀在回合结束时作为状态牌造成 2 点不可格挡伤害，不能误判为贝利亚攻击伤害。
- **地图路线生成**：`src/map/MapGenerator.cpp:460-547`、`tests/map_tests.cpp:65-208`
  - 模式：按固定行设置商店、事件、精英、休息和 Boss，连接保留同列直连并按概率添加相邻斜连。
  - 可复用：`MapGenerator::generateMap()`、路径递归校验。
  - 需注意：可选事件层按列生成，路径应最终经过 1 或 2 个事件，精英层每条路径恰好一次。

### 2. 项目约定

- **命名约定**：类和公共类型使用 PascalCase，成员函数使用 camelCase，私有状态成员以现有项目风格命名。
- **文件组织**：战斗规则位于 `src/combat`，地图生成位于 `src/map`，场景调度位于 `src/app`，测试位于 `tests`。
- **代码风格**：C++17，`.hpp/.cpp` 分离，4 空格缩进，中文用户文案和注释，资源路径使用 `assets/...`。

### 3. 可复用组件清单

- `CombatSystem::captureSafeSnapshot()`：保存可复活的战斗状态。
- `CombatSystem::reviveFromLastSafeSnapshot()`：恢复快照、应用复活增益并可开启新回合。
- `Enemy::getIntentDamage()`：提供贝利亚蓄能倍率后的意图基础伤害。
- `Game::startBattle()`：统一选择普通战斗音乐并按索引轮换。
- `MapGenerator::generateMap()`：生成带分支连接的地图。

### 4. 测试策略

- **测试框架**：CMake 注册的独立 C++ 测试可执行文件，当前环境直接运行 `out/build/windows-x64/Debug/*.exe`，因为命令行未提供 `ctest`。
- **测试模式**：战斗状态机和地图路径属性测试。
- **参考文件**：`tests/combat_tests.cpp`、`tests/map_tests.cpp`。
- **覆盖要求**：贝利亚 200 HP、蓄能伤害、黑暗侵蚀、致命伤害恢复、新回合、精英和事件路径约束、三首音乐资源存在。

### 5. 依赖和集成点

- **外部依赖**：SFML、CMake、Visual Studio 2022。
- **内部依赖**：`Game::startBattle()`调用`CombatSystem::startBattle()`并根据地图节点选择音乐；地图节点类型由`MapGenerator`提供给`Game`。
- **配置来源**：`CMakeLists.txt`负责资源复制，`CMakePresets.json`负责 Windows Debug 构建。

### 6. 技术选型理由

- 使用现有快照机制修复复活，不新增第二套战斗状态保存逻辑。
- 使用现有地图行结构约束路线，避免在 UI 层补偿地图规则。
- 使用现有普通战斗音乐数组和索引轮换，避免重复创建音乐调度器。

### 7. 关键风险点

- **战斗边界**：致命伤害发生在多段攻击中必须只判定一次失败，并从攻击前状态恢复。
- **地图边界**：斜向连接可能使路径进入不同列，事件数量必须按真实连线递归统计。
- **性能瓶颈**：音乐按战斗切换，资源由 `sf::Music`流式读取；地图测试需控制递归路径数量。
- **资源风险**：第三首普通战斗音乐必须同时存在于源码资源目录和构建输出目录。
