## 项目上下文摘要（贝利亚出场动画）

生成时间：2026-09-06 13:56:08 +08:00

### 1. 相似实现分析

- **实现1**: `src/app/Game.cpp` 的 `startBattle()`、`handleBattleResult()`
  - 模式：由 `Game` 根据地图节点生成遭遇、启动战斗、播放对应音乐。
  - 可复用：`EncounterDefinition.enemyId`、`battleIsBelial_`、`playMusic()`。
  - 需注意：Boss 音乐应在出场阶段就开始播放，战斗规则可以先初始化但不能立即进入可操作战斗。
- **实现2**: `src/app/Game.cpp` 的 `drawBelialTransitionOverlay()`、`updateBelialTransition()`
  - 模式：贝利亚专属过场由顶层状态驱动，绘制全屏遮罩和角色相关文案。
  - 可复用：`SceneType`/计时与输入拦截思路、贝利亚音效对象。
  - 需注意：出场动画是进入 Boss 战前的阻塞场景，不应混入首次死亡复活过场。
- **实现3**: `src/app/Game.cpp` 的 `drawEndingSequence()`、`drawRestScene()` 背景绘制模式
  - 模式：场景绘制函数铺满背景纹理，再绘制少量前景内容。
  - 可复用：按窗口比例缩放背景图、居中静态图、全屏遮罩淡入的 SFML 绘制方式。
  - 需注意：用户要求最简版，因此不加粒子、着色器、额外动画。

### 2. 项目约定

- **命名约定**: 场景枚举使用 PascalCase，私有成员沿用已有字段风格，函数使用 camelCase。
- **文件组织**: 顶层流程和资源加载位于 `src/app/Game.*`，资源位于 `assets/images` 和 `assets/sounds`。
- **导入顺序**: 项目头文件在前，标准库头文件在后。
- **代码风格**: C++17、四空格缩进、中文界面文案和 UTF-8 源码。

### 3. 可复用组件清单

- `Game::playMusic()`：播放并循环 Boss 战配乐。
- `Game::startBattle()`：初始化 `CombatSystem` 和地图遭遇。
- `Game::render()` / `Game::update()`：按 `SceneType` 分派场景更新与绘制。
- `sf::Music::getStatus()`：判断出场音效是否播放完毕。
- `BattleView::reset()`：进入战斗前清理战斗 UI 状态。

### 4. 测试策略

- **测试框架**: CTest。
- **测试模式**: 规则测试全量回归，资源复制和启动冒烟验证运行时可用性。
- **参考文件**: `tests/combat_tests.cpp`、`tests/map_tests.cpp`、`tests/event_tests.cpp`。
- **覆盖要求**: 构建通过、CTest 全通过、出场背景资源复制到运行目录、主程序启动不崩溃。

### 5. 依赖和集成点

- **外部依赖**: SFML Graphics/Audio。
- **内部依赖**: `Game` 持有窗口、纹理、音乐、`BattleView` 和 `CombatSystem`。
- **集成方式**: 贝利亚战斗先进入 `SceneType::BelialIntro`，先执行地图到出场画面的短淡入，音效结束或玩家输入后调用 `finishBelialIntro()` 切到 `SceneType::Battle`。
- **配置来源**: `assets/images/background/belial_intro_earth.jpg`、`assets/images/enemies/belial.png`、`assets/sounds/belial_intro.mp3`、`assets/sounds/final_battle.mp3`。

### 6. 技术选型理由

- **为什么用这个方案**: 出场阶段是战斗前 UI 阻塞流程，顶层场景能最清楚地控制输入、绘制和音乐。
- **优势**: 复用现有场景系统，改动集中，不影响战斗规则层。
- **劣势和风险**: 没有视觉自动化截图断言，画面构图依赖代码审查和人工试玩。

### 7. 关键风险点

- **并发问题**: 出场音效与 Boss 配乐同时播放，跳过时只停止出场音效，保留 Boss 配乐。
- **边界条件**: 出场音效加载失败时应自动切入战斗，不阻塞流程。
- **性能瓶颈**: 入场画面只绘制两张纹理和一层遮罩，没有明显性能风险。
- **安全考虑**: 本轮不涉及安全控制。
