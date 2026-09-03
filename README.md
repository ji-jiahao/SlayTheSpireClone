# 战斗模块（combat）

负责单场战斗的规则结算：玩家与敌人的状态、抽牌/弃牌/消耗、出牌与目标选择、回合流程、伤害公式、状态效果以及胜负判定。本模块**只做规则，不绘制 UI**，也不直接修改整局 `GameState`。

> 分支：`feature/combat-system`。依赖 `src/card/`（卡牌定义与牌堆 `Deck`），不依赖 `ui/`、`map/`、`event/`。

## 文件清单

```text
src/combat/
├── Player.hpp / .cpp          玩家战斗状态（生命、能量、格挡、力量、虚弱、易伤）
├── Enemy.hpp / .cpp           敌人状态（生命、意图伤害、力量、虚弱、易伤）
├── CombatSystem.hpp / .cpp    战斗总控：多敌人、出牌、目标、回合、胜负
```

## 公共接口

### BattleResult

```cpp
enum class BattleResult { Active, Victory, Defeat };
```

### 敌人与遭遇

```cpp
struct EnemySpawnDefinition {
    std::string name = "邪教徒";
    int health = 40;
    int intentDamage = 6;
};

struct EncounterDefinition {
    std::vector<EnemySpawnDefinition> enemies{{}}; // 默认 1 个邪教徒，EncounterDefinition{} 兼容旧调用
};
```

### CombatSystem

```cpp
class CombatSystem {
public:
    CombatSystem();

    void startBattle(int currentHealth = 80, std::uint32_t seed = 0);
    void startBattle(int currentHealth, std::uint32_t seed, std::vector<Card> cards);
    void startBattle(int currentHealth, std::uint32_t seed, std::vector<Card> cards,
                     const EncounterDefinition& encounter, int startingBlock,
                     int startingStrength, int startingEnergy = 0,
                     int extraDrawCards = 0, int maxHealth = 80);

    bool playCard(int handIndex, int targetEnemyIndex);
    void endPlayerTurn();
    void update();

    const Player& getPlayer() const;
    const std::vector<Enemy>& getEnemies() const;
    const Enemy& getEnemyAt(std::size_t index) const;
    const std::vector<Card>& getHandCards() const;
    const Deck& getDeck() const;
    BattleResult getResult() const;
};
```

- `startBattle(...)`：用 `EncounterDefinition` 构造多个敌人，洗牌后抽 5 张（外加遗物带来的额外抽牌），并应用开战格挡/力量/能量等修正。
- `playCard(handIndex, targetEnemyIndex)`：打出手牌中第 `handIndex` 张牌，作用于第 `targetEnemyIndex` 个敌人。
- `endPlayerTurn()`：弃掉手牌，每个存活敌人各攻击玩家一次，然后玩家进入下一回合并抽牌。
- `getEnemyAt(index)`：越界会抛 `std::out_of_range`，调用方需自行保证索引有效。

### Player（战斗状态）

```cpp
class Player {
public:
    Player(int maxHealth = 80, int maxEnergy = 3, int currentHealth = -1);
    void startTurn(); void endTurn();
    void takeDamage(int amount); void loseHealth(int amount);
    void gainBlock(int amount); void gainEnergy(int amount);
    bool spendEnergy(int amount);
    void applyStrength(int amount); void applyWeak(int turns); void applyVulnerable(int turns);
    // 及对应 getter
};
```

### Enemy

```cpp
class Enemy {
public:
    Enemy(std::string name = "Cultist", int maxHealth = 40);
    void takeDamage(int amount); void setIntentDamage(int amount);
    void applyStrength(int amount); void applyWeak(int turns); void applyVulnerable(int turns);
    void endTurn();
    bool isDead() const;
    // 及对应 getter
};
```

## 目标选择语义

`playCard` 的目标参数行为取决于卡牌效果里的 `CardTarget`（定义于 `src/card/Card.hpp`）：

| 卡牌目标 | 代表卡牌 | `targetEnemyIndex` 行为 |
|---|---|---|
| `Enemy`（单目标） | 打击、痛击、上勾拳 | **必须**为有效存活敌人索引，否则 `playCard` 返回 `false` 且不消耗能量 |
| `AllEnemies` | 顺劈斩、献祭 | 索引被忽略，对全体存活敌人结算 |
| `RandomEnemy` | 剑舞 | 索引被忽略，随机命中（当前实现等价于全体均摊） |
| `Self` / `None` | 防御、肌肉强化 | 索引被忽略，作用于玩家 |

判定“单目标牌”的依据：卡牌效果中存在 `target == CardTarget::Enemy` 即视为单目标。

## 出牌流程（playCard）

```text
校验手牌索引
→ 若为单目标牌，校验 targetEnemyIndex 有效且目标存活（否则返回 false）
→ 校验能量足够并消耗能量（cost < 0 视为不可打出）
→ 消耗牌进入消耗堆，或进入弃牌堆
→ 按顺序结算卡牌效果（resolveEffect）
→ 若所有敌人死亡 → Victory；若玩家生命 ≤ 0 → Defeat
```

## 效果结算（resolveEffect）

| CardEffectType | 结算 |
|---|---|
| `Damage` | 对目标敌人造成 `calculatePlayerDamage(value, target)` |
| `MultiDamage` | 对每个存活敌人各结算 `hits_N` 次（`hits_` 前缀参数控制段数，默认 1 段） |
| `Block` / `Draw` / `GainEnergy` / `LoseHealth` | 作用于玩家 |
| `ApplyStrength` / `ApplyWeak` / `ApplyVulnerable` | `Enemy` 目标作用于指定敌人；`AllEnemies` 作用于全体存活敌人；其余作用于玩家 |
| `Exhaust` | `random_hand` / `choose_hand` 当前消耗手牌第一张 |

## 伤害公式

- 玩家对敌人：`max(0, base + 玩家力量)`，玩家虚弱时 `×0.75`，目标易伤时 `×1.5`，向下取整。
- 敌人对玩家：`max(0, base + 敌人力量)`，敌人虚弱时 `×0.75`，玩家易伤时 `×1.5`，向下取整。

## 回合流程（endPlayerTurn）

```text
弃掉手牌 → 玩家结束回合（虚弱/易伤层数 -1）
→ 每个存活敌人依次攻击玩家
→ 玩家生命 ≤ 0 → Defeat
→ 玩家开始新回合（格挡清零、能量回满）并抽 5 张
```

## 使用示例

### 单敌人

```cpp
CombatSystem combat;
combat.startBattle(80, seed, CardDatabase::createStarterDeck());
combat.playCard(0, 0);   // 打出第 0 张牌，目标为第 0 个敌人
```

### 多敌人点选

```cpp
EncounterDefinition encounter;
encounter.enemies = {{"邪教徒", 40, 6}, {"酸液史莱姆", 30, 8}};

CombatSystem combat;
combat.startBattle(80, seed, CardDatabase::createStarterDeck(), encounter, 0, 0);
// UI 层把鼠标点到的敌人换算成索引传入：
combat.playCard(0, 1);   // 只对第 1 个敌人结算
```

### AOE（无需指定目标）

```cpp
CombatSystem combat;
combat.startBattle(80, seed, {CardDatabase::createById("cleave")}, twoEnemies, 0, 0);
combat.playCard(0, 0);   // 顺劈斩：索引被忽略，全体敌人掉血
```

## 与 UI / 上层对接要点

- 战斗结果通过 `getResult()` 读取；`Victory` 表示**所有敌人死亡**，`Defeat` 表示玩家生命 ≤ 0。
- UI 需要：把鼠标位置换算为敌人索引，单目标牌要求玩家点选一个存活敌人，AOE/无目标牌直接 `playCard(index, 0)`。
- 敌人数量与状态通过 `getEnemies()` / `getEnemyAt(index)` 读取并绘制。
- 多敌人战斗由上层在 `startBattle` 时通过 `EncounterDefinition` 传入。

## 测试

`tests/combat_tests.cpp` 覆盖：单敌人伤害/格挡/易伤结算、能量不足禁止出牌、胜利判定、遗物开战修正，以及多敌人的点选目标、AOE、无效目标拒绝、全体阵亡胜利、多敌人各自攻击。

```powershell
cmake --build --preset debug
ctest -R CombatTests
```
