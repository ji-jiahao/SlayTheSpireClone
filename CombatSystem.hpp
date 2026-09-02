#pragma once

#include "combat/Enemy.hpp"
#include "combat/Player.hpp"

#include "card/Deck.hpp"

#include <cstddef>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

enum class BattleResult
{
    Active,
    Victory,
    Defeat
};

struct EncounterDefinition
{
    std::string enemyId = "cultist";
    std::string enemyName = "邪教徒";
    int enemyHealth = 40;
    int intentDamage = 6;
};

class CombatSystem
{
public:
    CombatSystem();

    void startBattle(int currentHealth = 80, std::uint32_t seed = 0);
    void startBattle(int currentHealth, std::uint32_t seed, std::vector<Card> cards);
    void startBattle(int currentHealth, std::uint32_t seed, std::vector<Card> cards,
                     const EncounterDefinition& encounter, int startingBlock,
                     int startingStrength, int startingEnergy = 0,
                     int extraDrawCards = 0, int maxHealth = 80);
    void startBattle(int currentHealth, std::uint32_t seed, std::vector<Card> cards,
                     std::vector<EncounterDefinition> encounters, int startingBlock = 0,
                     int startingStrength = 0, int startingEnergy = 0,
                     int extraDrawCards = 0, int maxHealth = 80);

    // UI 点击敌人后调用；单体目标卡会攻击当前选中的存活敌人。
    bool selectTarget(int enemyIndex);
    bool playCard(int handIndex);
    bool playCard(int handIndex, int targetIndex);
    void endPlayerTurn();
    void update();

    const Player& getPlayer() const;
    // 保留旧接口：返回当前选择的敌人，便于原有 UI 不改动也可继续运行。
    const Enemy& getEnemy() const;
    const std::vector<Enemy>& getEnemies() const;
    int getSelectedTargetIndex() const;
    const std::vector<Card>& getHandCards() const;
    const Deck& getDeck() const;
    int getEnemyIntentDamage() const;
    int getEnemyIntentDamage(std::size_t enemyIndex) const;
    BattleResult getResult() const;

private:
    bool hasLivingEnemies() const;
    bool isLivingTarget(int enemyIndex) const;
    int firstLivingEnemyIndex() const;
    bool requiresTarget(const Card& card) const;
    void resolveEffect(const CardEffect& effect, int targetIndex, int energySpent);
    void resolveEnemyIntent(Enemy& actingEnemy);
    void resolveTriggeredPowers(const char* trigger, int targetIndex = -1);
    int calculatePlayerDamage(int baseDamage, const Enemy& target) const;
    int calculateEnemyDamage(int baseDamage, const Enemy& source) const;
    void drawHand(std::size_t count = 5);
    void updateBattleResult();

    Player player;
    std::vector<Enemy> enemies;
    Deck deck;
    std::vector<CardEffect> activePowerEffects;
    std::vector<bool> deathPowerApplied;
    BattleResult result;
    int selectedTargetIndex;
    std::mt19937 randomEngine;
};
