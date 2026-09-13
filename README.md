# 东南苦行塔

基于 C++17、SFML 3.0.1 和 CMake 的《杀戮尖塔》第一幕铁甲战士简化复刻。

当前工作树基于 `main` 的最新已拉取提交 `b2204f7`。这个版本已经把队友的地图、背景音乐、篝火和商店代码合入，但仍然是“可运行的垂直切片”，不是完整第一幕。请先阅读 [实现现状与差距](docs/实现现状与差距.md)，再参考 [统一协作指南](docs/PROJECT_GUIDE.md) 和 [制作计划](docs/制作计划.md)。

## 当前可玩闭环

已接入的运行流程：

```text
主菜单（开始界面背景/音乐）
→ 塔底（fufu 动画、地图配乐、三选一祝福）
→ 9 层地图（每条路线 4 到 6 场非 Boss 战斗、至少 1 个商店，最后为 Boss）
→ 普通战斗、事件、篝火或商店
→ 返回地图
```

当前真实行为：

- 战斗当前仍是单敌人，但已支持通过 `EncounterDefinition.enemyId` 选择邪教徒、颚虫、酸液史莱姆、真菌兽、乐加维林和史莱姆老大等敌人原型；胜利后会获得 50 金币，并从三张候选卡牌中最多选择两张加入牌组，也可以只选一张或跳过。
- 每局生成新的随机种子，普通战斗将邪教徒、颚虫、酸液史莱姆和真菌兽洗牌后依次抽取，四种用完再洗牌，避免相邻重复；只有普通战斗胜利才推进序列。同一种子与进度可复现遭遇，精英节点只出现乐加维林，Boss 节点固定为 200 点生命的贝利亚，半血后不再提升攻击力。普通战斗少于四场的路线只遇到该随机序列的前几种怪。
- 铁甲战士初始状态为 80 HP、3 能量、5 张打击/4 张防御/1 张痛击；普通战斗胜利额外恢复 5 点生命，随后仍触发燃烧之血回血。
- `CardDatabase` 中有 73 张可获得卡牌定义和升级数据，但运行时牌组默认只使用初始牌组；`assets/data/cards.json` 尚未被加载。
- 地图节点实际包含普通战斗、精英、事件、篝火、商店和 Boss；没有宝箱场景，每条路线固定经过一次乐加维林精英战。
- 篝火支持一次休息（回复最大生命值的 30%），商店支持购买卡牌和删牌，不再出售遗物。
- 背景音乐、菜单/地图/篝火/商店资源和事件图片/音频已接入；进入贝利亚 Boss 战时会先从地图短暂淡入地球背景+贝利亚静止立绘的出场画面，出场音效结束或玩家点击/按键后进入可操作战斗；战斗 Boss 贝利亚使用静止立绘，其他敌人使用预生成 PNG 帧动画，卡牌使用第三版 `pixel_v2` 卡面资源。
- 击败贝利亚后不再进入普通战斗奖励，而是播放黑屏淡出、感谢游玩、制作人名单和最终致辞结算流程，结束后自动返回主菜单；结算期间右下角显示 `可按ESC退出`。
- 主菜单的“读档”按钮目前会重新开始新游戏，不是真正读档。

功能完成度和接口/资源缺口见 [docs/实现现状与差距.md](docs/实现现状与差距.md)。

## 最近改动速查

这部分是最近几轮改动的接口说明，重点放在地图、战斗、敌人和卡牌 UI。

### 地图

地图生成与推进仍然是两层：

- `MapGenerator::generateMap(int rowCount)`：生成整张地图的节点、行列和连线。
- `MapNode`：保存 `id`、`row`、`column`、`type`、`nextNodeIds`。
- `Game::startNewRun()`：开局时创建地图并进入塔底，选择祝福后进入地图。
- `Game::handleMapMouseClick(sf::Vector2f mousePosition)`：点击地图节点，决定是否能前进。
- `Game::isMapNodeSelectable(const MapNode& node)`：判断当前节点能否选择。
- `Game::layoutMapNodes()`：把节点转成可点击区域和绘制位置。
- `Game::showMap()`：战斗、事件、商店、休息结束后回到地图页。

最近地图层的规则是：

- 只能沿着 `nextNodeIds` 指向的路径前进。
- 初始从最底层开始。
- 节点类型仍然沿用 `Battle`、`Elite`、`Rest`、`Shop`、`Event`、`Boss`。
- 地图显示继续由 `Game::drawMapScene()` 负责，地图高度超过窗口时通过鼠标滚轮上下滚动查看。
- 当前开局生成 9 层地图：第 1、2 层保留战斗入口，第 3 层固定为商店，第 4 层为必经随机事件，第 5 层按分支随机为事件或普通战斗，第 6 层固定为精英乐加维林，第 8 层休息，顶层为 Boss；因此每条路线至少经过一个商店、恰好一个精英，并经过 1 到 2 个随机事件。
- 地图连接由 `Game::drawMapScene()` 绘制为弯曲虚线；节点点击使用 `mapScrollOffset_` 进行同一坐标换算，滚动后视觉位置与点击区域保持一致。

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
- `CombatSystem::getEnemyIntentDamage()`
- `CombatSystem::getResult()`

最近的战斗交互改动是：

- 手牌悬停会抬起、放大并显示卡牌信息框。
- 点击卡牌后进入目标选择状态。
- 敌人或自身目标区域悬停时显示高亮框。
- 左键直接确认出牌。
- 右键或 `Esc` 取消本次选择。
- 出牌后播放飞行弧线、命中闪光和碎片效果。
- 结束回合按钮沿用开始界面的悬停高亮方式，点击时播放 `assets/sounds/end_turn.mp3`。
- 普通战斗会在 `assets/sounds/battle_normal_2.mp3`、`assets/sounds/battle_normal_3.mp3` 和 `assets/sounds/battle_trance.mp3` 三首曲目之间轮流播放；贝利亚 Boss 首次被击倒前播放 Clark Aboud 的《The Heart》（`assets/sounds/the_heart.mp3`），相信光复活后继续播放 `assets/sounds/heavy_is_the_crown.mp3`。
- 进入贝利亚 Boss 战时先从地图画面淡入 `assets/images/background/belial_intro_earth.jpg` 和 `assets/images/enemies/belial.png`，同时播放 `assets/sounds/belial_intro.mp3` 与 Boss 战配乐；音效结束或玩家输入后才进入战斗。
- 贝利亚 Boss 胜利后进入制作名单结算流程，淡黑时开始播放 `assets/sounds/ending_credits.mp3`，流程结束后自动重置运行状态并返回主菜单。
- 战斗胜利后显示卡牌奖励窗口，可点击三张候选卡牌中的一张或两张，再点击“确认领取”加入牌组，也可以不选卡直接点击“跳过”或按 `Esc` 返回地图；金币奖励只在本场胜利结算一次。

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
- `Enemy::getIntent()`
- `Enemy::getBlock()`
- `Enemy::getArchetype()`
- `Enemy::getPowerDescription()`
- `Enemy::getStrength()`
- `Enemy::getWeak()`
- `Enemy::getVulnerable()`
- `Enemy::isDead()`

战斗页展示敌人信息时，直接从 `CombatSystem::getEnemy()` 和
`CombatSystem::getEnemyIntentDamage()` 读取。敌人的血条、意图名称、意图伤害、
格挡和状态文字由 `BattleView::drawEnemyPanel()` 绘制。

`EnemyIntent` 由规则层维护，`CombatSystem::endPlayerTurn()` 负责依次结算当前意图、
应用状态效果并推进下一回合意图。特殊行为包括仪式、沉睡唤醒、金属化、分裂、
死亡后的易伤以及向弃牌堆加入黏液。

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

- 优先按 `Card.id` 加载独立卡面资源。
- 资源路径格式为 `assets/images/cards/pixel_v2/<Card.id>.png`。
- 当前 73 个可获得卡牌 ID 均已有对应卡面。
- 重复中文名的卡牌按 ID 区分，不会因为名称相同而串图。
- 如果独立卡面或占位资源加载失败，回退到原来的几何卡牌绘制。

战斗页会通过 `CardView` 生成：

- 普通手牌绘制
- 悬停放大卡
- 选中待确认卡
- 出牌飞行中的卡牌精灵

商店卡牌也复用同一个 `CardView`，鼠标悬停未售卡牌时显示与战斗页一致的名称、类型、费用和描述提示框。商店使用 `assets/images/background/shop_background.jpg` 作为背景，不再出售遗物，卡牌商品扩展为 8 张并放大展示；卡牌价格按普通/非凡/稀有为 30/50/70，删牌费用为 50。

### 卡牌规则一致性

卡牌规则由 `CardEffectType + CardEffect::parameter` 描述，并统一从
`CombatSystem::playCard()` 进入 `resolveEffect()` 结算。当前已补齐：

- `Card::upgrade()`：同步升级后的名称、费用、描述、伤害、格挡和效果；`灼热打击`支持重复升级。
- `Deck::upgradeHandCard()` / `Deck::upgradeAllHandCards()`：供`武装`升级手牌使用。
- `CardEffectType::Heal`：实际恢复生命，且不会超过最大生命。
- `虚无`：未打出的牌在回合结束时消耗，打出后正常进入弃牌堆。
- `头槌`和`发掘`：取回牌时跳过当前正在结算的自身，避免把自己错误取回。
- 多段伤害、X费用和敌人死亡后的卡牌后续副作用：均与卡牌描述保持一致；`愤怒` 因复制牌路径与当前牌库表现冲突，已从可获得卡池移除。

卡牌需要选择具体手牌但当前没有独立选牌窗口时，规则层会按牌堆顺序选择第一张合法牌；
后续增加选牌 UI 时，只需替换对应选择入口，不需要改动基础效果结算。

### 战斗奖励

战斗结算由 `Game` 顶层场景编排，主要接口如下：

- `Game::handleBattleResult()`：监听战斗状态变化，结算普通战斗 5 点回血、燃烧之血、50 金币和奖励候选。
- `Game::prepareBattleReward()`：从 `CardDatabase::createIroncladCardPool()` 使用新的随机种子取三张不同卡牌。
- `Game::handleBattleRewardClick(sf::Vector2f)`：处理卡牌选中/取消、最多两张、确认领取和跳过，选中的卡牌通过 `GameState::addCard()` 加入牌组。
- `Game::drawBattleRewardOverlay()`：绘制胜利奖励窗口、卡牌和跳过按钮。

胜利奖励窗口显示在战斗视觉动画结束后；玩家可以从三张候选卡中选择一张、两张或直接跳过，选择后点击确认领取；失败战斗仍沿用原有结果覆盖层和返回地图流程。

### 资源接口

新增资源路径如下：

- `assets/images/cards/pixel_v2/<card_id>.png`：第三版按卡牌 ID 一一对应的正式卡面；第三版原始文件名为编号拼音，已按 `CardDatabase` 的可获得卡牌定义顺序转换为项目 ID 文件名
- `assets/images/enemies/belial.png`：贝利亚 Boss 静止立绘
- `assets/images/background/belial_intro_earth.jpg`：贝利亚 Boss 出场画面背景
- `assets/images/enemies/<enemy_id>/frame_000.png`：普通敌人逐帧 PNG；原始 GIF 已在资源接入时转换并去除洋红色背景
- `assets/sounds/belial_intro.mp3`：贝利亚 Boss 出场画面播放一次的开场语音
- `assets/sounds/battle_normal_2.mp3`、`assets/sounds/battle_normal_3.mp3`、`assets/sounds/battle_trance.mp3`：普通战斗轮换播放的三首战斗曲
- `assets/sounds/the_heart.mp3`：贝利亚 Boss 第一次死亡前的战斗曲（Clark Aboud — The Heart）
- `assets/sounds/heavy_is_the_crown.mp3`：贝利亚相信光复活后的战斗曲
- `assets/sounds/ending_credits.mp3`：击败贝利亚后的结算和制作名单音乐
- `assets/sounds/university_event.mp3`：大学事件进入初始状态时播放的背景曲
- `assets/sounds/laoda_theme.ogg`：失败后触发的音乐
- `assets/images/cards/starter_placeholder.png`
- `assets/images/cards/uncommon_placeholder.png`
- `assets/images/cards/rare_placeholder.png`
- `assets/sounds/card_select.mp3`
- `assets/sounds/card_attack.mp3`
- `assets/sounds/card_defense.mp3`
- `assets/sounds/end_turn.mp3`
- `assets/images/background/battle_background.png`
- `assets/images/background/map_background.png`
- `assets/images/background/shop_background.jpg`
- `assets/images/event/torch_stone_event_background.png`

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
4. 配置选择 `windows-x64`，启动目标选择 `东南苦行塔.exe`（CMake 目标名为 `SlayTheSpire`）。
5. 按 `Ctrl+F5` 运行，或按 `F5` 调试。

也可以在“开发人员 PowerShell”中构建：

```powershell
cmake --preset windows-x64
cmake --build --preset debug
ctest --test-dir out/build/windows-x64 -C Debug --output-on-failure
```

如果普通 PowerShell 找不到 `cmake` 或 `ctest`，请从 Visual Studio 2022 的“开发人员 PowerShell”运行以上命令。

## 协作规则

完整协作步骤见 [docs/PROJECT_GUIDE.md](docs/PROJECT_GUIDE.md)。代码事实和当前接口以 [docs/实现现状与差距.md](docs/实现现状与差距.md) 及源码为准，旧的目标接口不能直接当作已实现功能使用。

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
- 启动目标错误：选择 `东南苦行塔.exe`，不要选择 `ALL_BUILD` 或 `ZERO_CHECK`。
- 图片或字体找不到：使用项目内的相对路径，例如 `assets/images/card.png`，不要写个人电脑的绝对路径。
