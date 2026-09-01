#include "combat/CombatSystem.hpp"

#include "card/CardDatabase.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <utility>

namespace
{
constexpr int kPlayerMaxHealth = 80;
constexpr int kPlayerMaxEnergy = 3;
constexpr int kCultistMaxHealth = 40;
constexpr int kCultistIntentDamage = 6;
constexpr std::size_t kHandSize = 5;

int hitCount(const CardEffect& effect)
{
    constexpr const char* prefix = "hits_";
    if (effect.parameter.rfind(prefix, 0) != 0)
    {
        return 1;
    }

    try
    {
        return std::max(1, std::stoi(effect.parameter.substr(5)));
    }
    catch (...)
    {
        return 1;
    }
}
}

CombatSystem::CombatSystem()
    : player(kPlayerMaxHealth, kPlayerMaxEnergy),
      enemy("邪教徒", kCultistMaxHealth), deck(), result(BattleResult::Active)
{
    enemy.setIntentDamage(kCultistIntentDamage);
}

void CombatSystem::startBattle(int currentHealth, std::uint32_t seed)
{
    startBattle(currentHealth, seed, CardDatabase::createStarterDeck());
}

void CombatSystem::startBattle(int currentHealth, std::uint32_t seed, std::vector<Card> cards)
{
    startBattle(currentHealth, seed, std::move(cards), EncounterDefinition{}, 0, 0);
}

void CombatSystem::startBattle(int currentHealth, std::uint32_t seed, std::vector<Card> cards,
                               const EncounterDefinition& encounter, int startingBlock,
                               int startingStrength, int startingEnergy,
                               int extraDrawCards, int maxHealth)
{
    player = Player(maxHealth, kPlayerMaxEnergy, currentHealth);
    enemy = Enemy(encounter.enemyName, encounter.enemyHealth);
    enemy.setIntentDamage(encounter.intentDamage);
    deck = Deck(std::move(cards));
    deck.shuffle(seed);
    result = BattleResult::Active;
    player.gainBlock(startingBlock);
    player.applyStrength(startingStrength);
    player.gainEnergy(startingEnergy);
    drawHand(kHandSize + static_cast<std::size_t>(std::max(0, extraDrawCards)));
}

bool CombatSystem::playCard(int handIndex)
{
    if (result != BattleResult::Active || handIndex < 0 ||
        static_cast<std::size_t>(handIndex) >= deck.getHand().size())
    {
        return false;
    }

    const std::size_t index = static_cast<std::size_t>(handIndex);
    const Card card = deck.getHand()[index];
    if (card.cost < 0 || !player.spendEnergy(card.cost))
    {
        return false;
    }

    const bool exhaustPlayedCard = std::any_of(
        card.effects.begin(), card.effects.end(), [](const CardEffect& effect) {
            return effect.type == CardEffectType::Exhaust && effect.parameter.empty();
        });

    if (exhaustPlayedCard)
    {
        deck.exhaustCard(index);
    }
    else
    {
        deck.discardCard(index);
    }

    for (const CardEffect& effect : card.effects)
    {
        resolveEffect(effect);
        if (enemy.isDead() || player.getCurrentHealth() <= 0)
        {
            break;
        }
    }

    if (enemy.isDead())
    {
        result = BattleResult::Victory;
    }
    else if (player.getCurrentHealth() <= 0)
    {
        result = BattleResult::Defeat;
    }
    return true;
}

void CombatSystem::endPlayerTurn()
{
    if (result != BattleResult::Active)
    {
        return;
    }

    deck.discardHand();
    player.endTurn();
    player.takeDamage(calculateEnemyDamage(enemy.getIntentDamage()));
    enemy.endTurn();

    if (player.getCurrentHealth() <= 0)
    {
        result = BattleResult::Defeat;
        return;
    }

    player.startTurn();
    drawHand();
}

void CombatSystem::update()
{
}

const Player& CombatSystem::getPlayer() const { return player; }
const Enemy& CombatSystem::getEnemy() const { return enemy; }
const std::vector<Card>& CombatSystem::getHandCards() const { return deck.getHand(); }
const Deck& CombatSystem::getDeck() const { return deck; }
BattleResult CombatSystem::getResult() const { return result; }

void CombatSystem::resolveEffect(const CardEffect& effect)
{
    switch (effect.type)
    {
    case CardEffectType::Damage:
        enemy.takeDamage(calculatePlayerDamage(effect.value));
        break;
    case CardEffectType::MultiDamage:
        for (int hit = 0; hit < hitCount(effect) && !enemy.isDead(); ++hit)
        {
            enemy.takeDamage(calculatePlayerDamage(effect.value));
        }
        break;
    case CardEffectType::Block:
        player.gainBlock(effect.value);
        break;
    case CardEffectType::Draw:
        deck.drawCards(static_cast<std::size_t>(std::max(0, effect.value)));
        break;
    case CardEffectType::GainEnergy:
        player.gainEnergy(effect.value);
        break;
    case CardEffectType::LoseHealth:
        player.loseHealth(effect.value);
        break;
    case CardEffectType::ApplyStrength:
        if (effect.target == CardTarget::Enemy || effect.target == CardTarget::AllEnemies)
        {
            enemy.applyStrength(effect.value);
        }
        else
        {
            player.applyStrength(effect.value);
        }
        break;
    case CardEffectType::ApplyWeak:
        if (effect.target == CardTarget::Enemy || effect.target == CardTarget::AllEnemies)
        {
            enemy.applyWeak(effect.value);
        }
        else
        {
            player.applyWeak(effect.value);
        }
        break;
    case CardEffectType::ApplyVulnerable:
        if (effect.target == CardTarget::Enemy || effect.target == CardTarget::AllEnemies)
        {
            enemy.applyVulnerable(effect.value);
        }
        else
        {
            player.applyVulnerable(effect.value);
        }
        break;
    case CardEffectType::Exhaust:
        if ((effect.parameter == "random_hand" || effect.parameter == "choose_hand") &&
            !deck.getHand().empty())
        {
            deck.exhaustCard(0);
        }
        break;
    default:
        break;
    }
}

int CombatSystem::calculatePlayerDamage(int baseDamage) const
{
    double damage = static_cast<double>(std::max(0, baseDamage + player.getStrength()));
    if (player.getWeak() > 0) damage *= 0.75;
    if (enemy.getVulnerable() > 0) damage *= 1.5;
    return std::max(0, static_cast<int>(std::floor(damage)));
}

int CombatSystem::calculateEnemyDamage(int baseDamage) const
{
    double damage = static_cast<double>(std::max(0, baseDamage + enemy.getStrength()));
    if (enemy.getWeak() > 0) damage *= 0.75;
    if (player.getVulnerable() > 0) damage *= 1.5;
    return std::max(0, static_cast<int>(std::floor(damage)));
}

void CombatSystem::drawHand(std::size_t count)
{
    deck.drawCards(count);
}
