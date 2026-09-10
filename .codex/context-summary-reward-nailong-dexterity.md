## 项目上下文摘要（战斗奖励、奶龙事件音乐与敏捷状态）

生成时间：2026-09-10 00:20:00 +08:00

### 1. 相似实现分析

- **战斗奖励窗口**：`src/app/Game.cpp:1307-1407`、`src/app/Game.cpp:1697-1773`
  - 模式：`prepareBattleReward()`生成三张不同卡牌，`handleBattleRewardClick()`处理卡牌、跳过和重试，`drawBattleRewardOverlay()`负责绘制。
  - 可复用：现有三卡布局、悬停索引、`state.addCard()`、`battleRewardVisible`。
  - 需注意：从单选改为双选后，必须保留取消选择、最多两张和确认入口，不能点击第一张就关闭窗口。
- **事件音频**：`src/event/Event.hpp:35-65`、`src/event/EventDatabase.cpp:581-597`、`src/ui/EventView.cpp:171-176,574-595`
  - 模式：事件状态携带 `soundPath`，进入状态时打开音频，状态切换或事件结束时停止。
  - 可复用：`EventState`、`parseState()`、`playStateSound()`、`stopSound()`。
  - 需注意：奶龙背景音乐需要循环，因此为状态增加可选循环标记，旧事件默认保持不循环。
- **人物状态图标**：`src/ui/BattleIcons.hpp:6-9`、`src/ui/BattleIcons.cpp:33-72`、`src/ui/BattleHud.cpp:161-170`
  - 模式：`BattleIcons::Kind`选择几何图标，`BattleHud::drawStatuses()`按固定槽位绘制图标和数字。
  - 可复用：现有图标坐标系统、状态数字和悬停说明。
  - 需注意：敏捷要加入玩家和敌人状态槽位；敌人当前没有持久敏捷值，敌人槽位传 0。

### 2. 项目约定

- **命名约定**：C++ 类和枚举使用 PascalCase，成员函数使用 camelCase，私有成员沿用现有后缀 `_` 或项目既有命名。
- **文件组织**：场景流程位于 `src/app`，事件数据位于 `assets/data`，事件模型位于 `src/event`，战斗 HUD 位于 `src/ui`，测试位于 `tests`。
- **代码风格**：C++17，4 空格缩进，UTF-8 无 BOM，用户文案和新增注释使用简体中文，资源路径使用 `assets/...`。

### 3. 可复用组件清单

- `Game::prepareBattleReward()`：随机生成三张候选卡牌。
- `Game::handleBattleRewardClick()`：统一处理奖励窗口点击。
- `Game::battleRewardCardBounds()`：复用三张卡牌的碰撞区域。
- `EventView::playStateSound()`：统一加载和播放事件状态音频。
- `BattleIcons::draw()`：统一绘制人物上方状态图标。
- `BattleHud::drawStatuses()`：统一绘制状态数字和提示说明。
- `CombatSystem::reviveFromLastSafeSnapshot()`：贝利亚复活增益入口。

### 4. 测试策略

- **测试框架**：CMake 注册的独立 C++ 测试，主要覆盖战斗、事件和 UI 辅助逻辑。
- **参考文件**：`tests/event_tests.cpp`、`tests/combat_tests.cpp`、`tests/battle_hover_tests.cpp`。
- **覆盖要求**：奖励逻辑通过代码路径检查，事件测试验证奶龙音乐路径和循环标记，战斗测试验证 5 力量和 5 敏捷复活参数，构建测试验证新增图标枚举和接口编译。

### 5. 依赖和集成点

- **外部依赖**：SFML 3.0.1、CMake、Visual Studio 2022。
- **内部依赖**：`Game`驱动场景和奖励，`EventDatabase`读取 JSON，`EventView`播放事件音频，`BattleView`调用`BattleHud::drawStatuses()`。
- **配置来源**：`assets/data/events.json`定义奶龙状态音频，`CMakeLists.txt`复制整个 `assets` 目录。

### 6. 技术选型理由

- 奖励双选使用窗口内状态，不引入新的场景或弹窗类型。
- 音频循环使用 `sf::Music::setLooping()` 的既有播放对象，通过数据字段控制，旧数据保持兼容。
- 敏捷图标使用现有几何绘制体系，避免引入纹理加载和新的资源依赖。

### 7. 关键风险点

- **奖励边界**：重复点击已选卡应取消选择，未满两张时确认按钮必须禁用。
- **事件边界**：奶龙切换到第二状态时必须停止第一状态的循环音乐。
- **布局边界**：第四个状态图标不能遮挡敌人意图区域，状态提示框宽度需保持在窗口内。
- **复活边界**：贝利亚复活后的力量和敏捷都应为 5，不能残留旧的 10/10 调用。
