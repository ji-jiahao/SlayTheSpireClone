#pragma once

#include "card/Deck.hpp"
#include "combat/Enemy.hpp"
#include "combat/Player.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

enum class BattleResult
{
    Active,
    Victory,
    Defeat
};

struct EnemySpawnDefinition
{
    std::string name = "邪教徒";
    int health = 40;
    int intentDamage = 6;
};

struct EncounterDefinition
{
    // 默认一个邪教徒，保证 EncounterDefinition{} 仍可用。
    std::vector<EnemySpawnDefinition> enemies{{}};
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
    bool playCard(int handIndex, int targetEnemyIndex);
    void endPlayerTurn();
    void update();

    const Player& getPlayer() const;
    const std::vector<Enemy>& getEnemies() const;
    const Enemy& getEnemyAt(std::size_t index) const;
    const std::vector<Card>& getHandCards() const;
    const Deck& getDeck() const;
    BattleResult getResult() const;

private:
    void resolveEffect(const CardEffect& effect, int targetEnemyIndex);
    int calculatePlayerDamage(int baseDamage, const Enemy& target) const;
    int calculateEnemyDamage(int baseDamage, const Enemy& enemy) const;
    bool isValidEnemyTarget(int index) const;
    void drawHand(std::size_t count = 5);

    Player player;
    std::vector<Enemy> enemies;
    Deck deck;
    BattleResult result;
};
