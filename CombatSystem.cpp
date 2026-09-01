#include "combat/CombatSystem.hpp"

#include "card/CardDatabase.hpp"

#include <cstddef>
#include <cstdint>

namespace
{
constexpr int kPlayerMaxHealth = 80;
constexpr int kPlayerMaxEnergy = 3;
constexpr int kCultistMaxHealth = 40;
constexpr int kCultistIntentDamage = 6;
constexpr std::size_t kInitialHandSize = 5;
constexpr std::uint32_t kBattleShuffleSeed = 0;
}

CombatSystem::CombatSystem()
    : player(kPlayerMaxHealth, kPlayerMaxEnergy),
      enemy("Cultist", kCultistMaxHealth),
      deck(),
      isBattleActive(false)
{
    enemy.setIntentDamage(kCultistIntentDamage);
}

void CombatSystem::startBattle()
{
    player = Player(kPlayerMaxHealth, kPlayerMaxEnergy);
    enemy = Enemy("Cultist", kCultistMaxHealth);
    enemy.setIntentDamage(kCultistIntentDamage);
    deck = Deck(CardDatabase::createStarterDeck());
    deck.shuffle(kBattleShuffleSeed);
    isBattleActive = true;
    drawStartingHand();
}

void CombatSystem::playCard(int handIndex)
{
    if (!isBattleActive || enemy.isDead() || player.getCurrentHealth() <= 0 ||
        handIndex < 0 || static_cast<std::size_t>(handIndex) >= deck.getHand().size())
    {
        return;
    }

    const Card card = deck.getHand()[static_cast<std::size_t>(handIndex)];
    if (!player.spendEnergy(card.cost))
    {
        return;
    }

    resolveCardEffect(card);
    deck.discardCard(static_cast<std::size_t>(handIndex));

    if (enemy.isDead())
    {
        isBattleActive = false;
    }
}

void CombatSystem::endPlayerTurn()
{
    if (!isBattleActive || enemy.isDead() || player.getCurrentHealth() <= 0)
    {
        return;
    }

    player.endTurn();
    deck.discardHand();
    player.takeDamage(enemy.getIntentDamage());

    if (player.getCurrentHealth() <= 0)
    {
        isBattleActive = false;
        return;
    }

    player.startTurn();
    drawStartingHand();
}

void CombatSystem::update()
{
}

const Player& CombatSystem::getPlayer() const
{
    return player;
}

const Enemy& CombatSystem::getEnemy() const
{
    return enemy;
}

const std::vector<Card>& CombatSystem::getHandCards() const
{
    return deck.getHand();
}

void CombatSystem::drawStartingHand()
{
    deck.drawCards(kInitialHandSize);
}

void CombatSystem::resolveCardEffect(const Card& card)
{
    if (card.damage > 0)
    {
        enemy.takeDamage(card.damage);
    }

    if (card.block > 0)
    {
        player.gainBlock(card.block);
    }
}
