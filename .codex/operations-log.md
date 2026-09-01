## 编码前检查 - 奶龙事件

时间：2026-09-01 13:42:54 +08:00

□ 已查阅上下文摘要文件：`.codex/context-summary-nailong-event.md`
□ 将使用以下可复用组件：

- `GameState`: `src/core/GameState.hpp` - 统一修改生命、金币和事件访问记录
- `EventSystem`: `src/event/EventSystem.cpp` - 执行选项效果和事件完成状态
- `EventDatabase`: `src/event/EventDatabase.cpp` - 读取事件 JSON

□ 将遵循命名约定：类和结构体大驼峰，函数和变量小驼峰，常量 `k` 前缀。
□ 将遵循代码风格：C++17、`.hpp/.cpp` 分离、项目 include 顺序。
□ 确认不重复造轮子，证明：已检查 `src/app/`、`src/ui/`、`src/event/`，App/UI 目录只有 `.gitkeep`，事件执行和 JSON 解析已有底座可扩展。

## 编码后声明 - 奶龙事件

时间：2026-09-01 13:42:54 +08:00

### 1. 复用了以下既有组件

- `GameState`: 用于生命、金币、牌组和事件访问记录。
- `EventSystem`: 用于执行 `lose_health`、`gain_gold` 等事件效果。
- `EventDatabase`: 用于解析 `assets/data/events.json`。

### 2. 遵循了以下项目约定

- 命名约定：新增 `Game`、`EventView`、`EventState` 均使用大驼峰。
- 代码风格：新增实现文件已加入 `CMakeLists.txt`，使用 SFML 3.0.1 API。
- 文件组织：App 层在 `src/app/`，事件界面在 `src/ui/`，事件数据和资源在 `assets/`。

### 3. 对比了以下相似实现

- `EventSystem.cpp`: 延续效果分发模式，只增加阶段切换和访问记录。
- `EventDatabase.cpp`: 延续 JSON 字段解析模式，新增字段均有默认值。
- 旧 `main.cpp`: 复用鼠标点击思路，但拆分到 `Game` 与 `EventView`。

### 4. 未重复造轮子的证明

- 检查了 `src/app/`、`src/ui/`、`src/event/`，未发现已有事件视图或场景壳。
- 保留并扩展已有事件数据库和事件执行器，没有另写第二套事件系统。

## 验证记录

- 已执行：`D:/c++/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe --preset windows-x64`
- 已执行：`D:/c++/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe --build --preset debug`
- 已执行：启动 `out/build/windows-x64/Debug/SlayTheSpire.exe` 3 秒，进程保持运行后结束测试进程。
- 已检查：`sfml-audio-d-3.dll`、事件图片、字体、音频和 `events.json` 已复制到 exe 旁边。
