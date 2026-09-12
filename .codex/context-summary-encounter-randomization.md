# 怪物遭遇修复上下文

时间：2026-09-13

目标：普通节点不出现精英，精英只有乐加维林，全部普通怪可出现且次序随机。
根因：Game.cpp encounterForMapNode 的 laterEncounters 含 lagavulin；GameState.seed 默认固定且 startNewRun 未重新赋值；按节点哈希取模加上早期池限制不能保证普通怪覆盖。
参考实现：Game.cpp 的节点类型分派和战斗胜利；MapGenerator.cpp 的显式种子重载与 std::mt19937；ShopSystem.cpp 的 std::shuffle；Enemy.cpp 的怪物原型；tests/map_tests.cpp 的运行时 require；README.md 的四种普通怪与单一精英定义。slime_boss 是 Boss，slime_pair 是其分裂形态，不属于普通怪池。
方案：抽出 MapEncounter 模块，普通怪四个一组按局种子洗牌，按已完成普通战斗数量取项；下一组重新洗牌并避免组间连续重复。精英固定 lagavulin、Boss 沿用 belial，非战斗节点拒绝创建遭遇。新局随机种子同时传给地图和遭遇；胜利才推进普通怪计数，重进同场不重抽。
依赖：GameState（种子/计数）→ Game → MapEncounter（MapNode、EncounterDefinition、标准随机库）；不增加第三方依赖。沿用 C++17、四空格、UTF-8 无 BOM、CMake 与 CTest。
验收：多种子每组普通战四怪覆盖且无精英/Boss；不同种子顺序变化，相同种子可复现；精英仅乐加维林；非法节点拒绝；新局计数重置；全套 Debug/Release 本地回归。
工具补偿：指定 sequential-thinking、shrimp-task-manager、desktop-commander、context7、github.search_code 不可用，按分析→计划→执行→审查顺序采用本地源码与标准库接口验证，不以猜测代替项目证据。
