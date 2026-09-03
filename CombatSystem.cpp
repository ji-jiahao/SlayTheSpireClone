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

bool hasSingleEnemyTarget(const Card& card)
{
    for (const CardEffect& effect : card.effects)
    {
        if (effect.target == CardTarget::Enemy)
        {
            return true;
        }
    }

    return false;
}
} // namespace

CombatSystem::CombatSystem()
    : player(kPlayerMaxHealth, kPlayerMaxEnergy),
      enemies{Enemy("邪教徒", kCultistMaxHealth)}, deck(), result(BattleResult::Active)
{
    enemies[0].setIntentDamage(kCultistIntentDamage);
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
    enemies.clear();
    for (const EnemySpawnDefinition& spawn : encounter.enemies)
    {
        Enemy enemy(spawn.name, spawn.health);
        enemy.setIntentDamage(spawn.intentDamage);
        enemies.push_back(std::move(enemy));
    }

    if (enemies.empty())
    {
        enemies.emplace_back("邪教徒", kCultistMaxHealth);
        enemies[0].setIntentDamage(kCultistIntentDamage);
    }

    deck = Deck(std::move(cards));
    deck.shuffle(seed);
    result = BattleResult::Active;
    player.gainBlock(startingBlock);
    player.applyStrength(startingStrength);
    player.gainEnergy(startingEnergy);
    drawHand(kHandSize + static_cast<std::size_t>(std::max(0, extraDrawCards)));
}

bool CombatSystem::playCard(int handIndex, int targetEnemyIndex)
{
    if (result != BattleResult::Active || handIndex < 0 ||
        static_cast<std::size_t>(handIndex) >= deck.getHand().size())
    {
        return false;
    }

    const std::size_t index = static_cast<std::size_t>(handIndex);
    const Card card = deck.getHand()[index];

    if (hasSingleEnemyTarget(card) && !isValidEnemyTarget(targetEnemyIndex))
    {
        return false;
    }

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
        resolveEffect(effect, targetEnemyIndex);
        if (player.getCurrentHealth() <= 0)
        {
            break;
        }
    }

    if (std::all_of(enemies.begin(), enemies.end(),
                    [](const Enemy& enemy) { return enemy.isDead(); }))
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

    for (Enemy& enemy : enemies)
    {
        if (enemy.isDead())
        {
            continue;
        }

        player.takeDamage(calculateEnemyDamage(enemy.getIntentDamage(), enemy));
        enemy.endTurn();
    }

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
const std::vector<Enemy>& CombatSystem::getEnemies() const { return enemies; }
const Enemy& CombatSystem::getEnemyAt(std::size_t index) const { return enemies.at(index); }
const std::vector<Card>& CombatSystem::getHandCards() const { return deck.getHand(); }
const Deck& CombatSystem::getDeck() const { return deck; }
BattleResult CombatSystem::getResult() const { return result; }

void CombatSystem::resolveEffect(const CardEffect& effect, int targetEnemyIndex)
{
    switch (effect.type)
    {
    case CardEffectType::Damage:
        if (isValidEnemyTarget(targetEnemyIndex))
        {
            const Enemy& target = enemies[static_cast<std::size_t>(targetEnemyIndex)];
            enemies[static_cast<std::size_t>(targetEnemyIndex)].takeDamage(
                calculatePlayerDamage(effect.value, target));
        }
        break;
    case CardEffectType::MultiDamage:
        for (std::size_t enemyIndex = 0; enemyIndex < enemies.size(); ++enemyIndex)
        {
            if (enemies[enemyIndex].isDead())
            {
                continue;
            }

            for (int hit = 0; hit < hitCount(effect); ++hit)
            {
                if (enemies[enemyIndex].isDead())
                {
                    break;
                }

                enemies[enemyIndex].takeDamage(
                    calculatePlayerDamage(effect.value, enemies[enemyIndex]));
            }
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
        if (effect.target == CardTarget::Enemy)
        {
            if (isValidEnemyTarget(targetEnemyIndex))
            {
                enemies[static_cast<std::size_t>(targetEnemyIndex)].applyStrength(effect.value);
            }
        }
        else if (effect.target == CardTarget::AllEnemies)
        {
            for (Enemy& enemy : enemies)
            {
                if (!enemy.isDead())
                {
                    enemy.applyStrength(effect.value);
                }
            }
        }
        else
        {
            player.applyStrength(effect.value);
        }
        break;
    case CardEffectType::ApplyWeak:
        if (effect.target == CardTarget::Enemy)
        {
            if (isValidEnemyTarget(targetEnemyIndex))
            {
                enemies[static_cast<std::size_t>(targetEnemyIndex)].applyWeak(effect.value);
            }
        }
        else if (effect.target == CardTarget::AllEnemies)
        {
            for (Enemy& enemy : enemies)
            {
                if (!enemy.isDead())
                {
                    enemy.applyWeak(effect.value);
                }
            }
        }
        else
        {
            player.applyWeak(effect.value);
        }
        break;
    case CardEffectType::ApplyVulnerable:
        if (effect.target == CardTarget::Enemy)
        {
            if (isValidEnemyTarget(targetEnemyIndex))
            {
                enemies[static_cast<std::size_t>(targetEnemyIndex)].applyVulnerable(effect.value);
            }
        }
        else if (effect.target == CardTarget::AllEnemies)
        {
            for (Enemy& enemy : enemies)
            {
                if (!enemy.isDead())
                {
                    enemy.applyVulnerable(effect.value);
                }
            }
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

int CombatSystem::calculatePlayerDamage(int baseDamage, const Enemy& target) const
{
    double damage = static_cast<double>(std::max(0, baseDamage + player.getStrength()));
    if (player.getWeak() > 0) damage *= 0.75;
    if (target.getVulnerable() > 0) damage *= 1.5;
    return std::max(0, static_cast<int>(std::floor(damage)));
}

int CombatSystem::calculateEnemyDamage(int baseDamage, const Enemy& enemy) const
{
    double damage = static_cast<double>(std::max(0, baseDamage + enemy.getStrength()));
    if (enemy.getWeak() > 0) damage *= 0.75;
    if (player.getVulnerable() > 0) damage *= 1.5;
    return std::max(0, static_cast<int>(std::floor(damage)));
}

bool CombatSystem::isValidEnemyTarget(int index) const
{
    return index >= 0 && static_cast<std::size_t>(index) < enemies.size() &&
           !enemies[static_cast<std::size_t>(index)].isDead();
}

void CombatSystem::drawHand(std::size_t count)
{
    deck.drawCards(count);
}
