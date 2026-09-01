# SlayTheSpireClone

基于 C++17 和 SFML 3 的课程设计项目。

所有环境配置、五人分工、接口约定、Event 规则、Git 流程和验收标准统一见 [docs/PROJECT_GUIDE.md](docs/PROJECT_GUIDE.md)。

## 当前可玩版本

目前 `main` 提供第一版单场战斗闭环：

- 铁甲战士 80 点生命、3 点能量和原版 5/4/1 初始牌组。
- 邪教徒 40 点生命，显示 6 点攻击意图。
- 点击手牌出牌，支持伤害、格挡、能量、力量、虚弱和易伤的基础结算。
- 痛击施加易伤，易伤期间攻击造成 1.5 倍伤害。
- 点击“结束回合”后，弃掉手牌、敌人攻击并重新抽 5 张牌。
- 战斗胜利后，“燃烧之血”回复 6 点生命；按 `R` 可重新战斗。

铁甲战士 74 张卡牌的数据和升级数据已经建立，但当前可玩闭环只使用初始牌组。复杂卡牌的条件触发、能力牌、选牌、X 费、多敌人和完整消耗机制仍需后续实现。

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

## 协作规则

完整协作步骤见 [docs/PROJECT_GUIDE.md](docs/PROJECT_GUIDE.md)。

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
