# 接口与命名约定

本文档是项目的最高优先级协作规范。开发范围与里程碑参考 `FIRST_ACT_PROJECT_PLAN.md`；当两份文档的职责分配冲突时，以本文档为准。

## 1. 技术与架构

项目采用 C++17、SFML 3.0.1 和 CMake：

```text
main.cpp
  -> app/Game
  -> Scene
  -> card/combat/ui/map/event 等业务模块
```

目录约定：

```text
src/
├─ main.cpp
├─ app/        Game、场景切换和整合
├─ core/       GameState、Scene、Random等公共基础类型
├─ card/       卡牌定义与卡牌数据库
├─ combat/     玩家、敌人、战斗规则和牌堆
├─ ui/         SFML绘制与输入转换
├─ map/        地图节点、连线、生成和可达性
└─ event/      问号事件定义、选择和效果

assets/
├─ fonts/
├─ images/
├─ sounds/
└─ data/
   ├─ cards.json
   ├─ enemies.json
   └─ events.json
```

## 2. 正式分工

团队共有 A/B/C/D/E 五名成员，其中 A 兼任组长：

```text
A（组长）：src/app/、src/core/、src/card/、main.cpp、CMakeLists.txt、整合与存档
B：src/combat/
C：src/ui/
D：src/map/
E：src/event/、assets/、数据表、字体、图片、README和说明文档
```

事件部分的协作边界：

- D 负责地图上问号节点的生成、连线和可达性。
- E 负责进入问号后选择哪个事件、事件选项以及事件效果。
- C 负责事件文字、图片、按钮和鼠标点击。
- A 的核心模块负责 `MapScene -> EventScene -> MapScene` 的切换和整局状态保存。
- A 的卡牌模块提供添加、删除、升级卡牌需要的接口。
- B 只在事件转化为战斗时接管战斗规则。

每个人优先只改自己的目录。公共头文件或函数签名修改前必须通知所有使用者。

## 3. 分支归属

```text
A（组长）：feature/game-core、feature/card-system
B：feature/combat-system
C：feature/ui
D：feature/map-system
E：feature/event-data
```

`feature/map-events` 是旧的混合分支，不再用于新开发。确认无人使用后再删除。

## 4. 命名约定

- 文件名和主要类名一致，例如 `CombatSystem.hpp`、`CombatSystem.cpp`。
- 头文件使用 `.hpp`，实现使用 `.cpp`。
- 类、结构体、枚举使用大驼峰，例如 `EventSystem`、`EventOption`。
- 函数使用小驼峰且以动词开头，例如 `playCard()`、`chooseOption()`。
- 变量使用小驼峰，例如 `currentHealth`、`currentEventId`。
- 编译期全局常量使用 `k` 前缀，例如 `kMaxHandSize`。

Include 顺序：本模块头文件、项目内头文件、第三方头文件、标准库头文件。

```cpp
#include "event/EventSystem.hpp"

#include "core/GameState.hpp"

#include <SFML/Graphics.hpp>

#include <string>
#include <vector>
```

## 5. 卡牌模块（A）

卡牌模块描述卡牌数据和卡牌来源，不绘制 UI，不控制完整战斗回合。

第一阶段接口：

```cpp
enum class CardType
{
    Attack,
    Skill,
    Power
};

struct Card
{
    std::string id;
    std::string name;
    CardType type;
    int cost;
    int damage;
    int block;
    std::string description;
};

class CardDatabase
{
public:
    static Card createStrike();
    static Card createDefend();
    static Card createBash();
};
```

第一阶段至少实现打击、防御和痛击。复杂卡牌效果在基础闭环完成后由 A 与 B 共同冻结新接口。

## 6. 战斗模块（B）

战斗模块保存并修改玩家战斗状态、敌人血量和攻击意图，不负责绘制。

```cpp
class Player
{
public:
    Player(int maxHealth, int maxEnergy);
    void startTurn();
    void endTurn();
    void takeDamage(int amount);
    void gainBlock(int amount);
    bool spendEnergy(int amount);

    int getCurrentHealth() const;
    int getMaxHealth() const;
    int getBlock() const;
    int getCurrentEnergy() const;
    int getMaxEnergy() const;
};

class Enemy
{
public:
    Enemy(std::string name, int maxHealth);
    void takeDamage(int amount);
    void setIntentDamage(int amount);

    const std::string& getName() const;
    int getCurrentHealth() const;
    int getMaxHealth() const;
    int getIntentDamage() const;
    bool isDead() const;
};

class CombatSystem
{
public:
    void startBattle();
    void playCard(int handIndex);
    void endPlayerTurn();
    void update();

    const Player& getPlayer() const;
    const Enemy& getEnemy() const;
    const std::vector<Card>& getHandCards() const;
};
```

`playCard()` 接收手牌索引，不接收鼠标坐标。UI 不得直接修改玩家或敌人血量。

## 7. UI 模块（C）

UI 负责显示和把输入转换为业务索引，不直接实现战斗、地图和事件规则。

```cpp
class CardView
{
public:
    sf::FloatRect getBounds() const;
    void setPosition(sf::Vector2f position);
    void draw(sf::RenderWindow& window, const Card& card) const;
};

class BattleView
{
public:
    void handleMouseClick(sf::Vector2f mousePosition, CombatSystem& combat);
    void draw(sf::RenderWindow& window, const CombatSystem& combat);
};

class EventView
{
public:
    void handleMouseClick(sf::Vector2f mousePosition, EventSystem& eventSystem,
                          GameState& gameState);
    void draw(sf::RenderWindow& window, const EventSystem& eventSystem) const;
};
```

## 8. 地图模块（D）

地图模块负责路线结构，不负责事件文本、选项效果或战斗内部规则。

```cpp
enum class MapNodeType
{
    Battle,
    Elite,
    Rest,
    Shop,
    Treasure,
    Unknown,
    Boss
};

struct MapNode
{
    int id;
    int row;
    int column;
    MapNodeType type;
    std::vector<int> nextNodeIds;
};

class MapGenerator
{
public:
    std::vector<MapNode> generateMap(int seed) const;
};
```

`Unknown` 表示地图上的问号。进入节点后由 E 的事件模块决定具体事件；若问号解析为战斗、商店或宝箱，则由 A 的核心模块切换到对应场景。

## 9. 事件模块（E）

事件模块负责事件数据、抽取、条件判断和选项结果，不负责地图连线或 SFML 绘制。

```cpp
enum class EventEffectType
{
    None,
    Heal,
    LoseHealth,
    GainGold,
    LoseGold,
    AddCard,
    RemoveCard,
    UpgradeCard
};

struct EventEffect
{
    EventEffectType type;
    int value;
    std::string parameter;
};

struct EventOption
{
    std::string text;
    std::string condition;
    std::vector<EventEffect> effects;
    int state;
    int nextState;
    bool closesEvent;
};

struct EventState
{
    std::string text;
    std::string imagePath;
    std::string soundPath;
};

struct EventDefinition
{
    std::string id;
    std::string title;
    std::string description;
    std::string backgroundPath;
    std::string imagePath;
    std::string soundPath;
    int act;
    int weight;
    std::vector<EventState> states;
    std::vector<EventOption> options;
};

class EventDatabase
{
public:
    bool loadFromFile(const std::string& filePath);
    const EventDefinition& getEvent(const std::string& eventId) const;
    std::vector<std::string> getActOneEventIds() const;
};

class EventSystem
{
public:
    bool startEvent(const std::string& eventId);
    bool chooseOption(int optionIndex, GameState& gameState);
    const EventDefinition& getCurrentEvent() const;
    std::size_t getCurrentStateIndex() const;
    bool isFinished() const;
};
```

第一阶段只实现稳定、容易验证的效果：回血、失血、获得/失去金币、添加卡牌、删除卡牌、升级卡牌和离开。添加、删除、升级卡牌必须调用 A 提供的公共接口，不能由 E 直接改卡牌模块内部数据。

多阶段事件通过 `EventOption::state`、`EventOption::nextState` 和 `EventOption::closesEvent` 表达。UI 只显示当前 `state` 的选项；选项执行成功后，若 `closesEvent` 为 `false` 且 `nextState` 有效，则切换到下一阶段，否则关闭事件并记录为已访问。

事件最少先实现：大鱼、世界黏液、发光祭坛、牧师、废弃软泥、金色神像。事件应保存在 `assets/data/events.json`，同一种子和同一访问顺序必须得到相同结果。

## 10. Game 与整局状态（A，兼任组长）

`main.cpp` 只创建并运行 `Game`：

```cpp
#include "app/Game.hpp"

int main()
{
    Game game;
    game.run();
    return 0;
}
```

```cpp
class Game
{
public:
    Game();
    void run();

private:
    void handleEvents();
    void update();
    void render();

    sf::RenderWindow window;
};
```

`GameState` 保存跨房间状态，例如生命、金币、长期牌组、遗物、药水、当前节点和随机种子。事件和战斗通过公开方法修改它，不得各自保存另一份真实状态。

## 11. 依赖方向

```text
card 不依赖 combat、ui、map、event
combat 可以依赖 card
map 不依赖 combat、ui、event
event 可以依赖 core 和 card 的公共类型
ui 可以读取 card、combat、map、event
app/core 负责组合模块，但不复制模块内部规则
```

禁止循环依赖。例如 `Card.hpp` 不能 include `CombatSystem.hpp`，`MapNode.hpp` 不能 include `EventSystem.hpp`。

## 12. CMake、合并与验收

每个新增 `.cpp` 必须加入根目录 `CMakeLists.txt` 的 `add_executable()`。新增 `.hpp` 可以列出，但不是构建必需。

开始工作：

```powershell
git switch main
git pull
git switch 对应负责分支
git merge main
```

提交前必须确认：

- Debug 构建成功。
- 新增 `.cpp` 已加入 CMake。
- 没有提交 `out/`、`.vs/`、EXE、DLL、OBJ、PDB或日志。
- 没有个人绝对路径。
- 没有直接修改其他模块内部状态。
- 公共接口变更已通知相关成员。
- 函数、类和文件名符合本规范。

第一阶段共同目标：

```text
打开游戏
显示玩家、敌人和手牌
点击攻击牌后敌人扣血
点击结束回合后敌人攻击玩家
玩家开始下一回合并抽新牌
```

完成第一阶段后，再接入地图、事件、遗物、更多卡牌、动画和正式资源。
