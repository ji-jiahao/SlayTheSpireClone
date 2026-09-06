## 项目上下文摘要（战斗奖励、商店卡牌提示框与场景背景）

生成时间：2026-09-05 15:20:00 +08:00

### 1. 相似实现分析

- **实现1**: `src/ui/BattleView.cpp`
  - 模式：视图层维护交互状态，使用预创建的 `sf::RenderTexture`、`sf::RectangleShape` 和 `optional<sf::Text>` 绘制卡牌悬浮提示框。
  - 可复用：`BattleHover::computeTooltipPosition()`、`UiHelpers::wrapText()`、`CardView::draw()`。
  - 需注意：战斗采用固定 1280x720 逻辑坐标，提示框必须避开屏幕边缘。
- **实现2**: `src/ui/ShopView.cpp`
  - 模式：`handleMouseMove()` 复用点击命中结果维护 `hoveredAction_`，`draw()` 按商品类型绘制卡牌、价格和高亮。
  - 可复用：`cardBounds()`、`hoveredAction_`、`CardView` 和 `UiHelpers`。
  - 需注意：已售商品仍可命中，新增卡牌提示框必须跳过已售商品，且不能改变购买命中路径。
- **实现3**: `src/ui/MainMenuView.cpp` 与 `src/ui/UiHelpers.cpp`
  - 模式：固定按钮热区 + 鼠标悬停状态 + 统一按钮绘制。
  - 可复用：`UiHelpers::drawButton()`、`UiHelpers::contains()`、开始界面同风格的描边和阴影。
  - 需注意：奖励窗口的“跳过”按钮应沿用相同的命中与高亮方式。
- **实现4**: `src/app/Game.cpp`
  - 模式：`SceneType` 顶层场景路由，`handleWindowEvent()` 分发输入，`render()` 负责场景组合绘制。
  - 可复用：`handleBattleResult()`、`drawResultOverlay()`、背景纹理加载和 `showMap()`。
  - 需注意：战斗视觉锁定期间不能提前切地图；胜利奖励应只结算一次。

### 2. 项目约定

- **命名约定**：类型和方法使用项目既有大小写，私有成员使用末尾下划线，常量使用 `k` 前缀。
- **文件组织**：规则在 `src/card`、`src/combat`、`src/room`、`src/core`；界面在 `src/ui`；场景编排在 `src/app`；资源在 `assets`；测试在 `tests`。
- **代码风格**：C++17、SFML 3.0.1、`.hpp/.cpp` 分离、中文界面文字和注释、相对资源路径。

### 3. 可复用组件清单

- `src/ui/CardView.cpp`：统一卡牌牌面绘制和 `pixel_v2` 资源加载。
- `src/ui/BattleHover.cpp`：卡牌布局、重叠优先命中和提示框避边计算。
- `src/ui/UiHelpers.cpp`：文本换行、居中绘制、按钮绘制和矩形命中。
- `src/core/GameState.hpp`：`addCard()`、`gainGold()`、牌组和金币状态。
- `src/card/CardDatabase.cpp`：`createIroncladCardPool()`、`createFromInstance()`。

### 4. 测试策略

- **测试框架**：CMake/CTest 下的独立 C++ 断言程序。
- **参考文件**：`tests/room_tests.cpp`、`tests/battle_hover_tests.cpp`、`tests/battle_cast_tests.cpp`。
- **覆盖要求**：价格边界、购买/删牌扣款、卡牌池奖励候选数量和唯一性、现有全量测试、资源复制和主程序短启动。
- **限制**：当前没有可调用的 desktop-commander、sequential-thinking、context7、shrimp-task-manager 或 github.search_code 入口，改用本地 PowerShell 检索、Git 分支差异和 CTest 验证，并记录在操作日志。

### 5. 依赖和集成点

- **外部依赖**：SFML 3.0.1 Graphics/Audio，现有 CMake 资源复制流程。
- **内部依赖**：`Game` 持有 `BattleView`、`ShopView`、`CombatSystem`、`GameState`；`ShopView` 依赖 `ShopSystem`、`CardView` 和 UI helpers。
- **集成方式**：`Game::handleWindowEvent()` 分发鼠标，`Game::update()` 处理战斗结果，`Game::render()` 叠加背景和奖励窗口。
- **配置来源**：资源使用 `assets/images/background/battle_background.png`、`assets/images/background/map_background.png`；卡面继续使用 `assets/images/cards/pixel_v2/`。

### 6. 技术选型理由

- **战斗奖励**：沿用 `Game` 现有结果覆盖层，不新增场景类型，降低对地图和战斗状态机的侵入。
- **候选卡牌**：从 `CardDatabase::createIroncladCardPool()` 确定性洗牌取 3 张，保证每场战斗可复现且不重复。
- **商店提示框**：复用战斗提示框的布局算法和缓存文本对象，保持两处卡牌交互一致。
- **背景**：在 `Game` 加载纹理并传给 `BattleView`，地图由 `Game::drawMapScene()` 绘制背景和遮罩。

### 7. 关键风险点

- **状态边界**：胜利奖励必须只发放一次；奖励窗口完成前不能返回地图。
- **输入优先级**：战斗结果状态下先处理奖励窗口，不能继续把点击传给正常出牌逻辑。
- **资源路径**：背景图片要复制到 `assets`，并确认构建输出目录同步存在。
- **测试覆盖**：奖励窗口主要是渲染和鼠标交互，自动化覆盖有限，需要补充编译、资源检查和短启动验证。
- **UI 分支风险**：`origin/feature/ui` 基于旧主线，整体合并会删除当前地图、事件、商店资源和测试，本轮只保留当前主线与 `pixel_v2` 卡面。
