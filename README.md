# SlayTheSpireClone

基于 C++17 和 SFML 3 的课程设计项目。

所有环境配置、五人分工、接口约定、Event 规则、Git 流程和验收标准统一见 [docs/PROJECT_GUIDE.md](docs/PROJECT_GUIDE.md)。

## 当前可玩版本

目前 `main` 提供第一版单场战斗闭环：

- 铁甲战士 80 点生命、3 点能量和原版 5/4/1 初始牌组。
- 邪教徒 40 点生命，显示 6 点攻击意图。
- 点击手牌出牌，支持伤害、格挡、能量、力量、虚弱和易伤的基础结算。
- 痛击施加易伤，易伤期间攻击造成 1.5 倍伤害。
- 点击“结束回合”后，弃掉手牌、敌人攻击并重新抽 5 张牌。
- 战斗胜利后，“燃烧之血”回复 6 点生命；按 `R` 可重新战斗。

铁甲战士 74 张卡牌的数据和升级数据已经建立，但当前可玩闭环只使用初始牌组。复杂卡牌的条件触发、能力牌、选牌、X 费、多敌人和完整消耗机制仍需后续实现。

## 最近改动速查

这部分是最近几轮改动的接口说明，重点放在地图、战斗、敌人和卡牌 UI。

### 地图

地图生成与推进仍然是两层：

- `MapGenerator::generateMap(int rowCount)`：生成整张地图的节点、行列和连线。
- `MapNode`：保存 `id`、`row`、`column`、`type`、`nextNodeIds`。
- `Game::startNewRun()`：开局时创建地图并把场景切到地图页。
- `Game::handleMapMouseClick(sf::Vector2f mousePosition)`：点击地图节点，决定是否能前进。
- `Game::isMapNodeSelectable(const MapNode& node)`：判断当前节点能否选择。
- `Game::layoutMapNodes()`：把节点转成可点击区域和绘制位置。
- `Game::showMap()`：战斗、事件、商店、休息结束后回到地图页。

最近地图层的规则是：

- 只能沿着 `nextNodeIds` 指向的路径前进。
- 初始从最底层开始。
- 节点类型仍然沿用 `Battle`、`Rest`、`Shop`、`Event`、`Boss`。
- 地图显示继续由 `Game::drawMapScene()` 负责。

### 战斗

战斗页的主入口仍然在 `BattleView`，它负责输入、绘制和视觉状态：

- `BattleView::handleMouseMove(sf::Vector2f, const CombatSystem&)`
- `BattleView::handleMouseClick(sf::Vector2f, CombatSystem&)`
- `BattleView::handleKeyPress(sf::Keyboard::Key, CombatSystem&)`
- `BattleView::update(float)`
- `BattleView::draw(sf::RenderWindow&, const CombatSystem&) const`
- `BattleView::reset()`

战斗逻辑核心仍然在 `CombatSystem`：

- `CombatSystem::startBattle(...)`
- `CombatSystem::playCard(int handIndex)`
- `CombatSystem::endPlayerTurn()`
- `CombatSystem::update()`
- `CombatSystem::getHandCards()`
- `CombatSystem::getPlayer()`
- `CombatSystem::getEnemy()`
- `CombatSystem::getResult()`

最近的战斗交互改动是：

- 手牌悬停会抬起、放大并显示卡牌信息框。
- 点击卡牌后进入目标选择状态。
- 敌人或自身目标区域悬停时显示高亮框。
- 左键直接确认出牌。
- 右键或 `Esc` 取消本次选择。
- 出牌后播放飞行弧线、命中闪光和碎片效果。

战斗中卡牌布局和命中区域由这几个辅助接口支撑：

- `BattleHover::layoutHandCards(...)`
- `BattleHover::pickHoveredCardIndex(...)`
- `BattleHover::computeTooltipPosition(...)`
- `BattleHover::easeOutCubic(...)`
- `BattleCast::requiresTargetSelection(const Card&)`
- `BattleCast::resolveTargetKind(const Card&)`
- `BattleCast::easeInOutQuad(float)`
- `BattleCast::lerp(sf::Vector2f, sf::Vector2f, float)`

### 敌人

敌人本体的数据入口不变，还是走 `Enemy` 和 `CombatSystem`：

- `Enemy::getName()`
- `Enemy::getCurrentHealth()`
- `Enemy::getMaxHealth()`
- `Enemy::getIntentDamage()`
- `Enemy::getStrength()`
- `Enemy::getWeak()`
- `Enemy::getVulnerable()`
- `Enemy::isDead()`

战斗页展示敌人信息时，直接从 `CombatSystem::getEnemy()` 读取。
敌人的血条、意图、状态文字都还是由 `Game::drawEnemyPanel()` 和 `BattleView` 里的绘制逻辑承担。

### 卡牌 UI

卡牌的单卡绘制入口仍然是 `CardView`：

- `CardView::setFont(const sf::Font&)`
- `CardView::setPosition(sf::Vector2f)`
- `CardView::setScale(float)`
- `CardView::setRotation(float)`
- `CardView::getBounds() const`
- `CardView::draw(sf::RenderTarget&, const Card&) const`
- `CardView::getCardSize()`

最近卡牌 UI 的变化：

- 优先按稀有度加载占位美术资源。
- `Starter` / `Common` 使用同一张占位图。
- `Uncommon`、`Rare` 各自有独立占位图。
- 如果资源加载失败，回退到原来的几何卡牌绘制。

战斗页会通过 `CardView` 生成：

- 普通手牌绘制
- 悬停放大卡
- 选中待确认卡
- 出牌飞行中的卡牌精灵

### 资源接口

新增资源路径如下：

- `assets/images/cards/starter_placeholder.png`
- `assets/images/cards/uncommon_placeholder.png`
- `assets/images/cards/rare_placeholder.png`
- `assets/sounds/card_select.mp3`
- `assets/sounds/card_attack.mp3`
- `assets/sounds/card_defense.mp3`

### 相关文件

如果你想继续改这块，优先看这些文件：

- [src/app/Game.cpp](src/app/Game.cpp)
- [src/ui/BattleView.cpp](src/ui/BattleView.cpp)
- [src/ui/BattleView.hpp](src/ui/BattleView.hpp)
- [src/ui/CardView.cpp](src/ui/CardView.cpp)
- [src/ui/BattleHover.cpp](src/ui/BattleHover.cpp)
- [src/ui/BattleCast.cpp](src/ui/BattleCast.cpp)
- [src/map/MapGenerator.cpp](src/map/MapGenerator.cpp)
- [src/combat/CombatSystem.cpp](src/combat/CombatSystem.cpp)

## 开发环境

- Windows 10/11 x64
- Visual Studio 2022
- Visual Studio 工作负载：使用 C++ 的桌面开发
- Git
- 可访问 GitHub 的网络（首次配置时下载 SFML 3.0.1）

不需要手动安装 SFML。CMake 会下载固定的 VS2022 x64 预编译包，并在编译后把所需 DLL 复制到 exe 旁边。

## 第一次运行

1. 克隆仓库，不要只下载或复制某个 `.cpp` 文件。
2. 在 Visual Studio 2022 中选择“打开本地文件夹”，打开仓库根目录。
3. 等待 CMake 配置结束。首次配置会下载约 37 MB 的 SFML，因此会稍慢。
4. 配置选择 `windows-x64`，启动目标选择 `SlayTheSpire.exe`。
5. 按 `Ctrl+F5` 运行，或按 `F5` 调试。

也可以在“开发人员 PowerShell”中构建：

```powershell
cmake --preset windows-x64
cmake --build --preset debug
ctest --test-dir out/build/windows-x64 -C Debug --output-on-failure
```

## 协作规则

完整协作步骤见 [docs/PROJECT_GUIDE.md](docs/PROJECT_GUIDE.md)。

不要提交 `out/`、`.vs/`、`.obj`、`.exe` 或 DLL；这些文件由每台电脑自行生成。

每项功能使用独立分支，例如：

```powershell
git switch main
git pull
git switch -c feature/card-system
```

完成并本地编译通过后：

```powershell
git add .
git commit -m "feat: add card system"
git push -u origin feature/card-system
```

然后在 GitHub 创建 Pull Request，由另一名成员检查后合并到 `main`。不要让五个人同时直接修改并推送 `main`。

新增 `.cpp` 文件后，还要把它加入 `CMakeLists.txt` 中对应目标，否则不会参与编译。

## 常见问题

- 找不到编译器：在 Visual Studio Installer 安装“使用 C++ 的桌面开发”。
- 下载 SFML 失败：检查 Git 和 GitHub 网络，然后在 Visual Studio 中删除 CMake 缓存并重新配置。
- 启动目标错误：选择 `SlayTheSpire.exe`，不要选择 `ALL_BUILD` 或 `ZERO_CHECK`。
- 图片或字体找不到：使用项目内的相对路径，例如 `assets/images/card.png`，不要写个人电脑的绝对路径。
