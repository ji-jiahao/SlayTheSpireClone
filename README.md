# SlayTheSpireClone

基于 C++17、SFML 3.0.1 和 CMake 的《杀戮尖塔》第一幕铁甲战士简化复刻。

当前仓库以 `main` 的 `61bf568` 为准。这个版本已经把队友的地图、背景音乐、篝火和商店代码合入，但仍然是“可运行的垂直切片”，不是完整第一幕。请先阅读 [实现现状与差距](docs/实现现状与差距.md)，再参考 [统一协作指南](docs/PROJECT_GUIDE.md) 和 [制作计划](docs/制作计划.md)。

## 当前可玩闭环

已接入的运行流程：

```text
主菜单（开始界面背景/音乐）
→ 6 层随机地图
→ 普通战斗、事件、篝火或商店
→ 返回地图
```

当前真实行为：

- 战斗只有单个固定邪教徒（40 HP、6 点攻击意图），没有战后奖励或三选一卡牌。
- 铁甲战士初始状态为 80 HP、3 能量、5 张打击/4 张防御/1 张痛击；战斗胜利触发燃烧之血回血。
- `CardDatabase` 中有 74 张卡牌定义和升级数据，但运行时牌组默认只使用初始牌组；`assets/data/cards.json` 尚未被加载。
- 地图节点实际只有战斗、事件、篝火、商店和 Boss；没有宝箱场景，地图生成器当前不会生成精英节点。
- 篝火支持一次休息（回复最大生命值的 30%），商店支持购买卡牌、购买遗物和删牌。
- 背景音乐、菜单/地图/篝火/商店资源和事件图片/音频已接入；战斗场景仍使用几何占位图，卡牌和敌人图片目录为空。
- 主菜单的“读档”按钮目前会重新开始新游戏，不是真正读档。

功能完成度和接口/资源缺口见 [docs/实现现状与差距.md](docs/实现现状与差距.md)。

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
4. 配置选择 `windows-x64`，启动目标选择 `SlayTheSpire.exe`。
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
- 启动目标错误：选择 `SlayTheSpire.exe`，不要选择 `ALL_BUILD` 或 `ZERO_CHECK`。
- 图片或字体找不到：使用项目内的相对路径，例如 `assets/images/card.png`，不要写个人电脑的绝对路径。
