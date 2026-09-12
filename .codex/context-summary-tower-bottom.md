# 塔底场景上下文与验收

时间：2026-09-13

目标：Start 后播放金色“塔底”渐变，事件背景上展示用户 GIF 和右侧三选一祝福；选择后进入地图。

研究：已阅读 src/ui/EventView.cpp 的背景等比填充、人物与右侧选项布局；src/ui/UiHelpers.cpp 的字体、按钮与换行；src/app/Game.cpp 的场景过渡、startNewRun、startBattle、showMap 和胜利结算；src/core/GameState.hpp 的重置、生命上限与牌组；tests/room_tests.cpp 的本地断言测试及 CMakeLists.txt 的资源复制。

依赖关系：Start → 塔底视图（SFML、UiHelpers、GIF 离线帧图）→ 祝福系统（GameState、CardDatabase）→ 地图 → 房间完成回血 / 战斗开始敏捷。随机卡复用正式铁甲卡池，排除初始、状态、诅咒。

约定：C++17，四空格，现有命名，UTF-8 无 BOM；沿用 room_system、ui 和 CTest。不新增运行时依赖。

验收：三选一不可重复；新局重置；随机牌真实加入牌组；完成房间回血 8、上限封顶、死亡不回血、同节点不重复；每场战斗敏捷 2；动画期间禁止选择；人物帧和文字不重叠；Debug 构建与本地测试、隐藏窗口截图验证。

工具补偿：sequential-thinking、shrimp-task-manager、desktop-commander、context7、github.search_code 均未提供。按研究→计划→实施→验证顺序执行，使用项目代码及随附 SFML 头文件；本任务无需新外部算法。计划依次实施祝福数据、视图和资源、场景接线、自动验证。
