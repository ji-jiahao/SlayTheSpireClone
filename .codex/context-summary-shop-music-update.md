## 项目上下文摘要（商店背景与音乐替换）

生成时间：2026-09-06 12:50:42 +08:00

### 1. 相似实现分析

- **实现1**: src/app/Game.cpp
  - 模式：顶部常量声明资源路径，构造函数加载纹理和音频，场景切换时通过 playMusic() 控制循环背景音乐。
  - 可复用：playMusic()、stopMusic()、loadShopResources()、startBattle()、startEvent()。
  - 需注意：贝利亚过场还会控制心跳、蓄力和复活后的音乐。
- **实现2**: src/ui/RestView.cpp 与 src/ui/BattleView.cpp
  - 模式：视图持有外部纹理指针，绘制时按窗口比例铺满；资源失败则回退纯色背景。
  - 可复用：setBackground() 风格和背景缩放逻辑。
  - 需注意：ShopView 当前没有背景纹理接口，需要按同类视图补齐。
- **实现3**: ssets/data/events.json 与 src/ui/EventView.cpp
  - 模式：事件状态使用 sound 字段播放一次音频，ackground 字段由事件定义控制。
  - 可复用：大学事件 states[0].sound 和失败分支 states[2].sound。
  - 需注意：事件音频由 EventView::enterCurrentState() 播放，不应再由 Game 额外重复播放。

### 2. 项目约定

- **命名约定**: C++ 成员变量末尾可带 _，资源常量使用 kXxxPath。
- **文件组织**: 场景编排在 src/app/Game.*，视图绘制在 src/ui/*View.*，事件数据在 ssets/data/events.json。
- **导入顺序**: 项目头文件优先，标准库随后。
- **代码风格**: C++17、SFML 3、中文用户可见文本。

### 3. 可复用组件清单

- Game::playMusic(const std::string&, bool)：统一背景音乐播放。
- Game::startBattle()：选择普通战斗或 Boss 战音乐。
- Game::finishBelialRevival()：贝利亚复活后继续播放战斗音乐。
- ShopView::draw()：商店背景绘制入口。
- EventView::enterCurrentState()：事件状态音频播放入口。

### 4. 测试策略

- **测试框架**: CMake + CTest，本轮资源路径主要通过构建、CTest、启动冒烟验证。
- **参考文件**: 	ests/event_tests.cpp 校验大学事件存在，	ests/room_tests.cpp 校验商店规则。
- **覆盖要求**: 构建通过、CTest 全通过、资源路径可被构造流程加载、启动不崩溃。

### 5. 依赖和集成点

- **外部依赖**: SFML sf::Texture、sf::Music。
- **内部依赖**: Game 加载商店背景并注入 ShopView；vents.json 由 EventDatabase 读取。
- **集成方式**: 常量路径 + 构造期资源加载 + 场景切换调用。
- **配置来源**: ssets/images/background、ssets/sounds、ssets/data/events.json。

### 6. 技术选型理由

- 商店背景沿用 BattleView/RestView 的外部纹理指针模式，避免 ShopView 自己读文件。
- 战斗音乐轮换放在 Game::startBattle()，因为这里已经能区分普通战斗和 Boss，并掌握当前战斗次数。
- 大学事件音乐直接改 JSON，保持事件音频单一来源。

### 7. 关键风险点

- 新 MP3 文件名含中文，统一复制成英文资源名降低运行时路径风险。
- 贝利亚复活后的音乐需与 Boss 初始音乐一致，避免旧 Heavy Is the Crown 覆盖新 Boss 曲。
- 启动冒烟只能验证资源加载不导致崩溃，不能人工听辨曲目，需要通过路径配置审查补偿。
