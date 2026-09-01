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

Card makeSlimedCard()
{
    Card card;
    card.id = "slimed";
    card.name = "黏液";
    card.type = CardType::Skill;
    card.rarity = CardRarity::Status;
    card.cost = 1;
    card.description = "消耗。";
    card.effects = {{CardEffectType::Exhaust, 1, CardTarget::Self, {}}};
    card.upgradedCost = 1;
    card.upgradedDescription = card.description;
    card.upgradedEffects = card.effects;
    return card;
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
    enemy = Enemy(encounter.enemyId, encounter.enemyName, encounter.enemyHealth, seed + 97u);
    if (enemy.getArchetype() == EnemyArchetype::Generic)
        enemy.setIntentDamage(encounter.intentDamage);
    deck = Deck(std::move(cards));
    deck.shuffle(seed);
    result = BattleResult::Active;
    deathPowerApplied = false;
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
        if (!deathPowerApplied && enemy.getDeathVulnerable() > 0)
        {
            player.applyVulnerable(enemy.getDeathVulnerable());
            deathPowerApplied = true;
        }
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
    enemy.startTurn();
    const bool splitIntent = enemy.getIntent().type == EnemyIntentType::Split;
    resolveEnemyIntent();
    enemy.endTurn();

    if (player.getCurrentHealth() <= 0)
    {
        result = BattleResult::Defeat;
        return;
    }

    if (!splitIntent)
        enemy.advanceIntent();
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
int CombatSystem::getEnemyIntentDamage() const
{
    return calculateEnemyDamage(enemy.getIntent().damage);
}
BattleResult CombatSystem::getResult() const { return result; }

void CombatSystem::resolveEnemyIntent()
{
    const EnemyIntent intent = enemy.getIntent();
    for (int hit = 0; hit < std::max(1, intent.hits); ++hit)
    {
        if (intent.damage > 0) player.takeDamage(calculateEnemyDamage(intent.damage));
        if (player.getCurrentHealth() <= 0) break;
    }
    enemy.gainBlock(intent.block);
    enemy.applyStrength(intent.strength);
    player.applyWeak(intent.weak);
    player.applyVulnerable(intent.vulnerable);
    player.applyFrail(intent.frail);
    player.applyDexterity(intent.dexterity);
    if (intent.slimed > 0)
        deck.addToDiscardPile(makeSlimedCard(), static_cast<std::size_t>(intent.slimed));
    if (intent.type == EnemyIntentType::Split)
        enemy.resolveSplit();
}

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
        player.gainCardBlock(effect.value);
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
