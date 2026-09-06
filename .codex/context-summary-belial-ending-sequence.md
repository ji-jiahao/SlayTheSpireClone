## 项目上下文摘要（贝利亚胜利结算）

生成时间：2026-09-06 13:46:06 +08:00

### 1. 相似实现分析

- **实现1**: `src/app/Game.cpp` 的 `showGameOver()`、`drawGameOver()`
  - 模式：顶层场景由 `SceneType` 切换，绘制函数负责全屏覆盖层。
  - 可复用：场景切换、音乐播放、窗口标题更新。
  - 需注意：普通失败界面仍作为死亡结局入口，不应被贝利亚胜利流程复用。
- **实现2**: `src/app/Game.cpp` 的 `startBelialRevivalChoice()`、`updateBelialTransition()`、`drawBelialTransitionOverlay()`
  - 模式：使用计时器和枚举状态驱动阶段式过场。
  - 可复用：`sf::Music` 音频控制、全屏遮罩、居中文本绘制。
  - 需注意：贝利亚首次死亡复活状态和胜利结算状态必须互相清理，避免音效残留。
- **实现3**: `src/app/Game.cpp` 的 `handleBattleResult()`、`prepareBattleReward()`、`drawBattleRewardOverlay()`
  - 模式：战斗结果只处理一次，胜利后根据条件进入奖励或其他场景。
  - 可复用：`handledResult` 防重复结算、`battleRewardVisible` 奖励弹窗清理。
  - 需注意：击败贝利亚后应跳过普通卡牌奖励和 50 金币奖励。

### 2. 项目约定

- **命名约定**: 类型和函数使用 PascalCase/camelCase，成员变量中已有新增私有状态使用尾部下划线。
- **文件组织**: `Game.hpp` 声明顶层场景接口，`Game.cpp` 实现流程和绘制。
- **导入顺序**: 项目头文件在前，标准库头文件在后。
- **代码风格**: C++17，四空格缩进，中文界面文案直接写入 UTF-8 源码。

### 3. 可复用组件清单

- `Game::playMusic()`：统一播放背景音乐并设置循环。
- `Game::stopMusic()`：停止当前背景音乐。
- `Game::handleBattleResult()`：战斗胜负的唯一顶层结算入口。
- `UiHelpers::drawCenteredText()`：全屏过场居中文案绘制。
- `Game::makeText()`：使用项目字体创建 SFML 文本对象。

### 4. 测试策略

- **测试框架**: CTest，项目已有 `CardTests`、`CombatTests`、`BattleHoverTests`、`BattleCastTests`、`RelicTests`、`RoomTests`、`MapTests`、`EventTests`。
- **测试模式**: 以规则层单元测试和启动冒烟为主，UI 过场通过构建、资源复制检查和代码路径审查补偿。
- **参考文件**: `tests/combat_tests.cpp`、`tests/event_tests.cpp`。
- **覆盖要求**: 构建通过、现有测试全通过、新资源复制到运行目录、程序可启动。

### 5. 依赖和集成点

- **外部依赖**: SFML Graphics/Audio。
- **内部依赖**: `Game` 持有 `CombatSystem`、`BattleView`、`GameState` 和音频对象。
- **集成方式**: 贝利亚胜利由 `handleBattleResult()` 分流到 `startEndingSequence()`，再由 `update()` 和 `render()` 驱动结算场景。
- **配置来源**: 新音乐资源位于 `assets/sounds/ending_credits.mp3`，CMake 构建后复制整个 `assets` 目录。

### 6. 技术选型理由

- **为什么用这个方案**: 结算流程是全局场景，不属于战斗规则层；放在 `Game` 顶层能复用现有窗口、音乐和场景生命周期。
- **优势**: 改动范围小，和已有贝利亚过场/结果覆盖层一致，易于后续增加名单或跳转逻辑。
- **劣势和风险**: 当前没有自动化视觉断言，名单滚动效果主要依赖启动冒烟和代码审查。

### 7. 关键风险点

- **并发问题**: 背景音乐和贝利亚心跳/蓄力音效需要在结算开始时停止，避免重叠。
- **边界条件**: 击败贝利亚后不能继续发放普通战斗奖励；结算结束需要完整重置运行状态。
- **性能瓶颈**: 结算仅绘制少量文本和全屏矩形，不存在明显瓶颈。
- **安全考虑**: 本轮不涉及安全控制。
