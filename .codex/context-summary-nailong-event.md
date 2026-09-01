## 项目上下文摘要（奶龙事件）

生成时间：2026-09-01 13:42:54 +08:00

### 1. 相似实现分析

- **实现1**: `src/event/EventSystem.cpp`
  - 模式：事件效果由 `EventEffectType` 分发，统一修改 `GameState`。
  - 可复用：`GameState::loseHealth`、`GameState::gainGold`。
  - 需注意：UI 不直接修改生命或金币。
- **实现2**: `src/event/EventDatabase.cpp`
  - 模式：事件 JSON 统一解析为 `EventDefinition`、`EventOption` 和 `EventEffect`。
  - 可复用：`optionalString`、`optionalInt`、`parseEffect`。
  - 需注意：新增数据字段要有默认值，避免破坏已有 6 个事件。
- **实现3**: `src/main.cpp`
  - 模式：旧版事件沙盒已有 SFML 3 鼠标点击、悬停和窗口循环。
  - 可复用：鼠标坐标转换、按钮命中检测。
  - 需注意：正式结构要求 `main.cpp` 只创建并运行 `Game`。

### 2. 项目约定

- **命名约定**：类和结构体使用大驼峰，函数和变量使用小驼峰，常量使用 `k` 前缀。
- **文件组织**：App 层放 `src/app/`，事件逻辑放 `src/event/`，SFML 事件界面放 `src/ui/`，资源放 `assets/`。
- **导入顺序**：本模块头文件、项目内头文件、第三方头文件、标准库头文件。
- **代码风格**：C++17，头文件 `.hpp`，实现 `.cpp`，MSVC 使用 `/utf-8`。

### 3. 可复用组件清单

- `src/core/GameState.hpp`：跨房间真实状态，负责生命、金币、牌组和事件访问记录。
- `src/event/EventSystem.cpp`：执行事件选项和效果。
- `src/event/EventDatabase.cpp`：从 `assets/data/events.json` 读取事件定义。
- `src/ui/EventView.cpp`：本次新增的事件绘制、鼠标输入、音频和反馈层。

### 4. 测试策略

- **测试框架**：当前仓库暂无单元测试框架。
- **测试模式**：使用 CMake Debug 构建、短启动冒烟测试、资源复制检查。
- **参考文件**：无测试文件；构建脚本为 `CMakeLists.txt` 和 `CMakePresets.json`。
- **覆盖要求**：验证事件 JSON 可加载、图片/字体/音频可复制到 exe 旁边、新增 `.cpp` 参与编译。

### 5. 依赖和集成点

- **外部依赖**：SFML 3.0.1，使用 Graphics 和 Audio 组件。
- **内部依赖**：`Game` 组合 `EventDatabase`、`EventSystem`、`EventView` 和 `GameState`。
- **集成方式**：当前事件结束后进入地图占位页；以后 A 组可把该回调替换为正式 `MapScene`。
- **配置来源**：事件定义来自 `assets/data/events.json`，资源路径相对运行目录。

### 6. 技术选型理由

- **为什么用这个方案**：事件阶段放进数据，按钮点击仍由 `EventSystem` 执行业务效果，符合现有职责边界。
- **优势**：后续随机事件可复用 `state`、`next_state`、`closes_event`、`states` 写多阶段对话。
- **劣势和风险**：正式资源管理器尚未由 A 组完成，当前 `EventView` 直接加载事件资源。

### 7. 关键风险点

- **并发问题**：无并发逻辑。
- **边界条件**：HP 扣到 0 后进入游戏结束占位；事件完成后记录访问，运行期不重复触发。
- **性能瓶颈**：事件开始时加载图片，当前资源很少，可接受。
- **安全考虑**：本地课程项目，不涉及认证、网络输入或敏感数据。
