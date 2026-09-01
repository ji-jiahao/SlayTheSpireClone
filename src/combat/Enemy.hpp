#pragma once

#include <cstdint>
#include <random>
#include <string>

enum class EnemyArchetype
{
    Generic,
    Cultist,
    JawWorm,
    AcidSlime,
    FungiBeast,
    Lagavulin,
    SlimeBoss,
    SlimePair
};

enum class EnemyIntentType
{
    Attack,
    AttackDefend,
    DefendBuff,
    Buff,
    Debuff,
    Status,
    Sleep,
    Stunned,
    Preparing,
    Split,
    Composite
};

struct EnemyIntent
{
    EnemyIntentType type = EnemyIntentType::Attack;
    std::string name = "攻击";
    std::string description;
    int damage = 0;
    int hits = 1;
    int block = 0;
    int strength = 0;
    int weak = 0;
    int vulnerable = 0;
    int frail = 0;
    int dexterity = 0;
    int slimed = 0;
};

class Enemy
{
public:
    Enemy(std::string name = "Cultist", int maxHealth = 40);
    Enemy(std::string id, std::string name, int maxHealth, std::uint32_t seed);

    void startTurn();
    void endTurn();
    void advanceIntent();
    void takeDamage(int amount);
    void gainBlock(int amount);
    void setIntentDamage(int amount);
    void applyStrength(int amount);
    void applyWeak(int turns);
    void applyVulnerable(int turns);
    void resolveSplit();

    const std::string& getId() const;
    const std::string& getName() const;
    int getCurrentHealth() const;
    int getMaxHealth() const;
    int getBlock() const;
    int getIntentDamage() const;
    int getStrength() const;
    int getWeak() const;
    int getVulnerable() const;
    int getRitual() const;
    int getMetallicize() const;
    int getDeathVulnerable() const;
    EnemyArchetype getArchetype() const;
    const EnemyIntent& getIntent() const;
    std::string getPowerDescription() const;
    bool isDead() const;

private:
    EnemyArchetype archetypeFromId(const std::string& enemyId) const;
    void chooseInitialIntent();
    void chooseJawWormIntent();
    void chooseAcidSlimeIntent();
    void chooseFungiBeastIntent();
    void chooseSlimePairIntent();
    int rollPercent();
    void setIntent(const EnemyIntent& nextIntent);

    std::string id;
    std::string name;
    int maxHealth;
    int currentHealth;
    int block;
    int strength;
    int weak;
    int vulnerable;
    int ritual;
    int metallicize;
    int deathVulnerable;
    int sleepingTurns;
    int patternStep;
    int repeatedIntentCount;
    EnemyIntentType previousIntentType;
    EnemyArchetype archetype;
    EnemyIntent intent;
    std::mt19937 randomEngine;
};
