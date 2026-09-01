#pragma once

#include "card/Deck.hpp"
#include "combat/Enemy.hpp"
#include "combat/Player.hpp"

#include <cstdint>
#include <vector>

enum class BattleResult
{
    Active,
    Victory,
    Defeat
};

struct EncounterDefinition
{
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
    bool playCard(int handIndex);
    void endPlayerTurn();
    void update();

    const Player& getPlayer() const;
    const Enemy& getEnemy() const;
    const std::vector<Card>& getHandCards() const;
    const Deck& getDeck() const;
    BattleResult getResult() const;

private:
    void resolveEffect(const CardEffect& effect);
    int calculatePlayerDamage(int baseDamage) const;
    int calculateEnemyDamage(int baseDamage) const;
    void drawHand(std::size_t count = 5);

    Player player;
    Enemy enemy;
    Deck deck;
    BattleResult result;
};
