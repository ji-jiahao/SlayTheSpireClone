# 项目统一协作指南

本文件是项目唯一的团队协作入口。目标是用 C++17、SFML 3.0.1 和 CMake，完成《杀戮尖塔》第一层、铁甲战士的简化复刻。

> 重要：本文件描述的是团队目标和协作流程，不代表功能已经实现。当前源码审计见 [实现现状与差距.md](实现现状与差距.md)；当前 `main` 的运行闭环只有主菜单、6 层地图、单敌人战斗、事件、篝火和商店。

## 1. 开始开发

环境：Visual Studio 2022（使用 C++ 的桌面开发）、Git、可访问 GitHub 的网络。队员不需要手动安装 SFML，CMake 会自动下载固定版本。

```powershell
git clone https://github.com/ji-jiahao/SlayTheSpireClone.git
cd SlayTheSpireClone
git switch main
git pull
cmake --preset windows-x64
cmake --build --preset debug
```

也可以用 VS2022 的“打开本地文件夹”打开仓库根目录，等待 CMake 配置后运行 `SlayTheSpire.exe`。不要运行 `ALL_BUILD` 或 `ZERO_CHECK`。

## 2. 五人分工

A 同时担任组长，负责核心和卡牌；B、C、D、E 分别负责战斗、UI、地图、事件与资源。

| 成员 | 负责内容 | 目录/分支 |
|---|---|---|
| A（组长） | Game、Scene、GameState、存档、整合、卡牌和牌组 | `src/app/`、`src/core/`、`src/card/`；`feature/game-core`、`feature/card-system` |
| B | Player、Enemy、CombatSystem、伤害、能量、敌人意图 | `src/combat/`；`feature/combat-system` |
| C | SFML 绘制、鼠标输入、各场景界面 | `src/ui/`；`feature/ui` |
| D | 地图节点、连线、随机地图、可达性和访问状态 | `src/map/`；`feature/map-system` |
| E | EventSystem、事件数据、图片字体音效和项目文档 | `src/event/`、`assets/`；`feature/event-data` |

每个人只直接修改自己的模块。公共头文件或函数签名修改前，先在群里说明并让相关成员确认。

## 3. 模块边界

```text
main.cpp -> A 的 Game -> Scene -> card / combat / map / event / ui
```

- A 的 `GameState` 保存跨房间状态：生命、金币、长期牌组、当前节点和随机种子。
- A 的 `CardDatabase` 只描述卡牌；B 的 `CombatSystem` 执行卡牌效果。
- B 保存并修改战斗中的玩家、敌人、格挡、能量、牌堆和攻击意图。
- C 只负责显示和把鼠标位置转换为索引，不直接修改血量或金币。
- D 当前生成 `Event` 节点；`Unknown`（问号）节点类型尚未在 `src/map/MapNode.hpp` 中定义。
- E 决定问号事件、选项、条件和结果，不负责地图连线和 SFML 绘制。
- A 负责场景切换；事件变成战斗时交给 B。

## 4. 最小接口约定

文件名使用 `PascalCase.hpp/.cpp`；类、结构体、枚举使用大驼峰；函数和变量使用小驼峰；编译期常量使用 `k` 前缀。

```cpp
// A: src/card/Card.hpp
enum class CardType { Attack, Skill, Power };
struct Card {
    std::string id, name, description;
    CardType type;
    int cost = 0, damage = 0, block = 0;
};
```

```cpp
// B: src/combat/CombatSystem.hpp
void startBattle(int currentHealth = 80, std::uint32_t seed = 0);
bool playCard(int handIndex); // 当前为单敌人战斗，只接收手牌索引
void endPlayerTurn();
```

```cpp
// D: src/map/MapNode.hpp
enum class MapNodeType { Battle, Elite, Rest, Shop, Event, Boss };
struct MapNode { int id, row, column; MapNodeType type; std::vector<int> nextNodeIds; };
```

```cpp
// E: src/event/EventSystem.hpp
bool chooseOption(int optionIndex, GameState& gameState);
const EventDefinition& getCurrentEvent() const;
bool isFinished() const;
```

事件第一阶段只实现回血、失血、获得/失去金币和离开。添加、删除、升级卡牌必须调用 A 的公开接口。

## 5. Git 协作

功能分支可以直接 push；推荐保护 `main`，成员平时不直接改它。标准流程是“功能分支 push -> Pull Request -> 检查 -> 合并 main”。

```powershell
git switch 自己的分支
git pull
git add .
git commit -m "feat: describe the change"
git push
git branch --show-current
```

新增 `.cpp` 必须加入根目录 `CMakeLists.txt` 的 `add_executable()`。不要提交 `out/`、`.vs/`、`.obj`、`.exe`、`.dll`、`.pdb` 或个人绝对路径。

## 6. 开发顺序

1. M0：冻结接口，完成窗口和占位场景。
2. M1：显示玩家、敌人和手牌，打出攻击/防御牌，结束回合并受到敌人攻击。
3. M2：完成基础卡牌、战斗状态和战斗 UI。
4. M3：接入地图选择、问号节点、事件界面和战斗后奖励。
5. M4：加入篝火、商店、宝箱、精英和 Boss，完成第一层闭环。
6. M5：存档/读档、平衡、资源替换、Release 打包和答辩演示。

一个功能必须能编译、正常使用、非法输入不崩溃、界面显示结果、状态能回到地图，并有测试或人工验收步骤。

## 7. 合并前检查

- Debug 构建成功，新增 `.cpp` 已加入 CMake。
- 没有个人绝对路径和生成文件。
- 没有绕过其他模块直接修改内部状态。
- 公共接口变更已通知相关成员。
- 至少一名其他成员检查过代码。

每次合并后按当前能力验证：新游戏 -> 地图 -> 一场战斗或事件 -> 篝火/商店 -> 返回地图。战斗奖励和保存/读档尚未接入，不能把它们写成已通过的验收项。
