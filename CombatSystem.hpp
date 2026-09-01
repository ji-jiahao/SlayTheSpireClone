#pragma once

#include "combat/Enemy.hpp"
#include "combat/Player.hpp"
#include "card/Deck.hpp"

#include <vector>

class CombatSystem
{
public:
    CombatSystem();

    void startBattle();
    void playCard(int handIndex);
    void endPlayerTurn();
    void update();

    const Player& getPlayer() const;
    const Enemy& getEnemy() const;
    const std::vector<Card>& getHandCards() const;

private:
    void drawStartingHand();
    void resolveCardEffect(const Card& card);

    Player player;
    Enemy enemy;
    Deck deck;
    bool isBattleActive;
};
