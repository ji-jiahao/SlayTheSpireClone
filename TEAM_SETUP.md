# 队员下载、运行和协作指南

仓库地址：<https://github.com/ji-jiahao/SlayTheSpireClone>

## 方案一：自己操作

### 1. 准备软件

Windows 电脑需要：

- Visual Studio 2022
- Visual Studio Installer 中的“使用 C++ 的桌面开发”工作负载
- Git for Windows
- 能访问 GitHub 的网络

不需要单独下载或配置 SFML。项目首次配置时，CMake 会自动下载官方 SFML 3.0.1 VS2022 x64 包。

### 2. 克隆仓库

选择一个准备存放项目的目录，在该目录打开 PowerShell，然后执行：

```powershell
git clone https://github.com/ji-jiahao/SlayTheSpireClone.git
cd SlayTheSpireClone
```

不要只下载或复制 `main.cpp`。项目还需要 `CMakeLists.txt`、`CMakePresets.json` 等构建文件。

也可以在 GitHub 页面点击 `Code`，然后选择 `Download ZIP`。但参与协作时推荐使用 `git clone`，否则不方便获取更新和提交代码。

### 3. 用 Visual Studio 2022 打开

1. 启动 Visual Studio 2022。
2. 选择“打开本地文件夹”。
3. 选择刚刚克隆的 `SlayTheSpireClone` 文件夹。
4. 等待 CMake 配置完成。第一次会下载约 37 MB 的 SFML。
5. 选择 `windows-x64` 配置和 `SlayTheSpire.exe` 启动目标。
6. 按 `Ctrl+F5` 运行，或按 `F5` 调试。

正常结果是出现一个窗口，其中有卡牌和敌人的矩形占位图。

### 4. 获取组内最新代码

开始工作前执行：

```powershell
git switch main
git pull
```

### 5. 创建自己的功能分支

不要直接在 `main` 分支开发。例如负责卡牌系统：

```powershell
git switch -c feature/card-system
```

建议的分支名：

- A（组长、核心与卡牌）：`feature/game-core`、`feature/card-system`
- B（战斗）：`feature/combat-system`
- C（UI）：`feature/ui`
- D（地图）：`feature/map-system`
- E（事件、资源和数据）：`feature/event-data`

具体接口、目录边界和跨模块调用规则，以 [docs/INTERFACE_AND_NAMING.md](docs/INTERFACE_AND_NAMING.md) 为准。

完成代码并确认能够编译后执行：

```powershell
git add .
git commit -m "feat: add card system"
git push -u origin feature/card-system
```

然后在 GitHub 仓库页面创建 Pull Request，不要直接覆盖其他成员的代码。

## 方案二：交给 AI 配置

把下面整段提示词发给能够操作本地终端和文件的 AI，并把最后一行的目标目录改成自己希望存放项目的位置：

```text
请帮我在 Windows 上下载并验证这个 C++ 课程项目：
https://github.com/ji-jiahao/SlayTheSpireClone

请严格执行以下步骤：
1. 先只读检查 Git、Visual Studio 2022、MSVC x64、Windows SDK 和 CMake 是否存在，不要猜测路径。
2. 如果缺少“使用 C++ 的桌面开发”等组件，只告诉我应在 Visual Studio Installer 中安装什么，不要擅自安装大型软件。
3. 确认目标目录不存在同名项目或重要文件后，用 git clone 克隆完整仓库。不要只下载 main.cpp，也不要覆盖已有目录。
4. 完整阅读仓库中的 README.md、TEAM_SETUP.md、CMakePresets.json 和 CMakeLists.txt。
5. 不要修改 SFML 下载地址、版本或项目架构，也不要写入个人 SFML 绝对路径。该项目会由 CMake 自动下载 SFML 3.0.1。
6. 使用 windows-x64 预设进行 CMake 配置，再执行 Debug 构建。
7. 验证 SlayTheSpire.exe 已生成，并确认所需 SFML DLL 位于 exe 同一目录。
8. 不要提交 out、.vs、obj、exe、dll 等生成文件，不要修改或推送 main 分支。
9. 最后告诉我：实际项目路径、配置结果、构建结果、exe 路径，以及如何在 VS2022 中打开和运行。
10. 如果任何命令失败，请展示关键错误并查明原因，不要假装成功。

目标父目录：C:\Users\你的用户名\Desktop
```

如果 AI 只能聊天、不能操作电脑，它只能给出命令，仍需本人在 PowerShell 和 Visual Studio 中执行。

## 常见错误

### CMake 下载 SFML 失败

确认可以访问 GitHub，然后在 Visual Studio 中选择“项目 -> 删除缓存并重新配置”。首次下载不要中途关闭 Visual Studio。

### 找不到 C++ 编译器

打开 Visual Studio Installer，修改 VS2022，安装“使用 C++ 的桌面开发”，并确保包含 MSVC v143 和 Windows 10/11 SDK。

### 按 F5 没有游戏窗口

确认启动目标是 `SlayTheSpire.exe`，不是 `ALL_BUILD` 或 `ZERO_CHECK`。

### 提示找不到 DLL

先重新构建项目。CMake 正常构建后会把 `sfml-graphics`、`sfml-window` 和 `sfml-system` DLL 自动复制到 exe 目录。

### 新增 cpp 后没有参与编译

把新文件加入根目录 `CMakeLists.txt` 中 `add_executable(SlayTheSpire ...)` 的文件列表，然后重新配置 CMake。
