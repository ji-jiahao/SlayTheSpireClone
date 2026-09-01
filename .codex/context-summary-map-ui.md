# 项目上下文摘要（地图图标与路线限制）

生成时间：2026-09-01 21:20:00

## 1. 相似实现分析

- **实现1**: `src/app/Game.cpp`
  - 模式：`Game` 负责场景切换、地图点击入口和跨房间状态。
  - 可复用：`state.currentNodeId`、`mapNodes`、`layoutMapNodes()`。
  - 需注意：事件和战斗结束后都应回到地图，不能重置路线。
- **实现2**: `src/ui/EventView.cpp`
  - 模式：资源通过相对路径加载，绘制时按窗口尺寸缩放。
  - 可复用：`sf::Texture` 缓存、`sf::Sprite` 缩放和透明度控制。
  - 需注意：资源必须位于 `assets/`，由 CMake 复制到 exe 目录。
- **实现3**: `src/map/MapGenerator.cpp`
  - 模式：生成 `MapNode` 列表，每个节点通过 `nextNodeIds` 表示可前进路线。
  - 可复用：`MapNodeType`、`MapNode::row`、`MapNode::nextNodeIds`。
  - 需注意：地图应从 `row=0` 起点向更高 row 前进，最后一层是 Boss。

## 2. 项目约定

- **命名约定**: 类型大驼峰，函数与变量小驼峰，常量使用 `k` 前缀。
- **文件组织**: 源码在 `src/`，运行资源在 `assets/`，地图图标放入 `assets/images/map/`。
- **导入顺序**: 本模块头文件、项目内头文件、第三方头文件、标准库头文件。
- **代码风格**: C++17、SFML 3.0.1、头实现分离。

## 3. 可复用组件清单

- `src/app/Game.cpp`: 地图绘制、场景切换、点击入口。
- `src/map/MapNode.hpp`: 地图节点类型和连线数据。
- `src/map/MapGenerator.cpp`: 地图节点与连线生成。
- `src/core/GameState.hpp`: 保存当前节点 `currentNodeId`。

## 4. 测试策略

- **测试框架**: 现有 CMake 测试可执行文件。
- **参考文件**: `tests/card_tests.cpp`、`tests/combat_tests.cpp`、`tests/relic_tests.cpp`。
- **覆盖要求**: Debug 构建、三组测试、exe 短启动。

## 5. 依赖和集成点

- **外部依赖**: SFML Graphics 用于图标绘制。
- **内部依赖**: `Game` 读取 `MapGenerator` 输出，按 `MapNodeType` 切换事件或战斗。
- **集成方式**: 点击可选节点后更新 `GameState::currentNodeId`，之后只允许点击当前节点的 `nextNodeIds`。
- **配置来源**: CMake 已复制整个 `assets/` 到 exe 目录。

## 6. 技术选型理由

- **为什么用这个方案**: 直接复用现有 `Game` 和 `MapNode` 数据结构即可表达路线锁定，不新增场景框架。
- **优势**: 改动小，和之后 D 组正式地图系统的 `nextNodeIds` 可以自然衔接。
- **劣势和风险**: `MapNodeType::Elite` 枚举为兼容旧接口暂时保留，但地图生成器不再生成精英节点。

## 7. 关键风险点

- **边界条件**: 初始 `currentNodeId=-1` 时只能点底层节点；节点无连线时不能继续前进。
- **性能瓶颈**: 当前节点数量很少，线性查找足够；后续大地图可引入 id 索引表。
- **资源风险**: 原始 zip 不作为运行资源，实际使用 `assets/images/map/*.png`，Boss 图标来自原精英图标资源。
