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
constexpr std::size_t kHandSize = 5;
constexpr std::size_t kMaxHandSize = 10;

int hitCount(const CardEffect& effect, int energySpent)
{
    if (effect.parameter == "x_cost")
    {
        return std::max(0, energySpent);
    }

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

bool hasTrigger(const CardEffect& effect)
{
    return effect.parameter == "start_turn" || effect.parameter == "end_turn" ||
           effect.parameter == "on_attack_played" || effect.parameter == "on_exhaust";
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
    return card;
}
}

CombatSystem::CombatSystem()
    : player(kPlayerMaxHealth, kPlayerMaxEnergy),
      enemies{Enemy("邪教徒", 40)},
      deck(),
      result(BattleResult::Active),
      selectedTargetIndex(0),
      randomEngine(0)
{
    enemies.front().setIntentDamage(6);
}

void CombatSystem::startBattle(int currentHealth, std::uint32_t seed)
{
    startBattle(currentHealth, seed, CardDatabase::createStarterDeck());
}

void CombatSystem::startBattle(int currentHealth, std::uint32_t seed, std::vector<Card> cards)
{
    startBattle(currentHealth, seed, std::move(cards),
                std::vector<EncounterDefinition>{EncounterDefinition{}});
}

void CombatSystem::startBattle(int currentHealth, std::uint32_t seed, std::vector<Card> cards,
                               const EncounterDefinition& encounter, int startingBlock,
                               int startingStrength, int startingEnergy,
                               int extraDrawCards, int maxHealth)
{
    startBattle(currentHealth, seed, std::move(cards),
                std::vector<EncounterDefinition>{encounter}, startingBlock,
                startingStrength, startingEnergy, extraDrawCards, maxHealth);
}

void CombatSystem::startBattle(int currentHealth, std::uint32_t seed, std::vector<Card> cards,
                               std::vector<EncounterDefinition> encounters, int startingBlock,
                               int startingStrength, int startingEnergy,
                               int extraDrawCards, int maxHealth)
{
    if (encounters.empty())
    {
        encounters.push_back(EncounterDefinition{});
    }

    player = Player(maxHealth, kPlayerMaxEnergy, currentHealth);
    enemies.clear();
    enemies.reserve(encounters.size());
    for (std::size_t index = 0; index < encounters.size(); ++index)
    {
        const EncounterDefinition& encounter = encounters[index];
        enemies.emplace_back(encounter.enemyId, encounter.enemyName, encounter.enemyHealth,
                             seed + 97u + static_cast<std::uint32_t>(index));
        if (enemies.back().getArchetype() == EnemyArchetype::Generic)
        {
            enemies.back().setIntentDamage(encounter.intentDamage);
        }
    }

    deck = Deck(std::move(cards));
    deck.shuffle(seed);
    activePowerEffects.clear();
    deathPowerApplied.assign(enemies.size(), false);
    result = BattleResult::Active;
    selectedTargetIndex = 0;
    randomEngine.seed(seed + 31337u);

    player.gainBlock(startingBlock);
    player.applyStrength(startingStrength);
    player.gainEnergy(startingEnergy);
    drawHand(kHandSize + static_cast<std::size_t>(std::max(0, extraDrawCards)));
}

bool CombatSystem::selectTarget(int enemyIndex)
{
    if (!isLivingTarget(enemyIndex))
    {
        return false;
    }

    selectedTargetIndex = enemyIndex;
    return true;
}

bool CombatSystem::playCard(int handIndex)
{
    return playCard(handIndex, selectedTargetIndex);
}

bool CombatSystem::playCard(int handIndex, int targetIndex)
{
    if (result != BattleResult::Active || handIndex < 0 ||
        static_cast<std::size_t>(handIndex) >= deck.getHand().size())
    {
        return false;
    }

    const std::size_t index = static_cast<std::size_t>(handIndex);
    const Card card = deck.getHand()[index];
    if (requiresTarget(card) && !selectTarget(targetIndex))
    {
        return false;
    }

    const int energySpent = card.cost < 0 ? player.getCurrentEnergy() : card.cost;
    if (!player.spendEnergy(energySpent))
    {
        return false;
    }

    const bool exhaustPlayedCard = card.type == CardType::Power || std::any_of(
        card.effects.begin(), card.effects.end(), [](const CardEffect& effect) {
            return effect.type == CardEffectType::Exhaust && effect.parameter.empty();
        });
    if (exhaustPlayedCard)
    {
        deck.exhaustCard(index);
        resolveTriggeredPowers("on_exhaust", targetIndex);
    }
    else
    {
        deck.discardCard(index);
    }

    for (const CardEffect& effect : card.effects)
    {
        // 能力牌的带触发条件效果在本场战斗持续生效，而不是立即结算。
        if (card.type == CardType::Power && hasTrigger(effect))
        {
            activePowerEffects.push_back(effect);
            continue;
        }

        resolveEffect(effect, targetIndex, energySpent);
        updateBattleResult();
        if (result != BattleResult::Active)
        {
            break;
        }
    }

    if (card.type == CardType::Attack && result == BattleResult::Active)
    {
        resolveTriggeredPowers("on_attack_played", targetIndex);
    }
    updateBattleResult();
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
    resolveTriggeredPowers("end_turn");
    updateBattleResult();
    if (result != BattleResult::Active)
    {
        return;
    }

    for (Enemy& enemy : enemies)
    {
        if (enemy.isDead())
        {
            continue;
        }
        enemy.startTurn();
        resolveEnemyIntent(enemy);
        enemy.endTurn();
        if (player.getCurrentHealth() <= 0)
        {
            result = BattleResult::Defeat;
            return;
        }
        if (!enemy.isDead())
        {
            enemy.advanceIntent();
        }
    }

    selectedTargetIndex = firstLivingEnemyIndex();
    player.startTurn();
    resolveTriggeredPowers("start_turn", selectedTargetIndex);
    updateBattleResult();
    if (result == BattleResult::Active)
    {
        drawHand();
    }
}

void CombatSystem::update()
{
}

const Player& CombatSystem::getPlayer() const { return player; }
const Enemy& CombatSystem::getEnemy() const
{
    return enemies[static_cast<std::size_t>(selectedTargetIndex >= 0 ? selectedTargetIndex : 0)];
}
const std::vector<Enemy>& CombatSystem::getEnemies() const { return enemies; }
int CombatSystem::getSelectedTargetIndex() const { return selectedTargetIndex; }
const std::vector<Card>& CombatSystem::getHandCards() const { return deck.getHand(); }
const Deck& CombatSystem::getDeck() const { return deck; }
int CombatSystem::getEnemyIntentDamage() const
{
    return getEnemyIntentDamage(static_cast<std::size_t>(std::max(0, selectedTargetIndex)));
}
int CombatSystem::getEnemyIntentDamage(std::size_t enemyIndex) const
{
    if (enemyIndex >= enemies.size())
    {
        return 0;
    }
    return calculateEnemyDamage(enemies[enemyIndex].getIntent().damage, enemies[enemyIndex]);
}
BattleResult CombatSystem::getResult() const { return result; }

bool CombatSystem::hasLivingEnemies() const
{
    return std::any_of(enemies.begin(), enemies.end(), [](const Enemy& enemy) {
        return !enemy.isDead();
    });
}

bool CombatSystem::isLivingTarget(int enemyIndex) const
{
    return enemyIndex >= 0 && static_cast<std::size_t>(enemyIndex) < enemies.size() &&
           !enemies[static_cast<std::size_t>(enemyIndex)].isDead();
}

int CombatSystem::firstLivingEnemyIndex() const
{
    for (std::size_t index = 0; index < enemies.size(); ++index)
    {
        if (!enemies[index].isDead())
        {
            return static_cast<int>(index);
        }
    }
    return -1;
}

bool CombatSystem::requiresTarget(const Card& card) const
{
    return std::any_of(card.effects.begin(), card.effects.end(), [](const CardEffect& effect) {
        return effect.target == CardTarget::Enemy;
    });
}

void CombatSystem::resolveEnemyIntent(Enemy& actingEnemy)
{
    const EnemyIntent intent = actingEnemy.getIntent();
    for (int hit = 0; hit < std::max(1, intent.hits); ++hit)
    {
        if (intent.damage > 0)
        {
            player.takeDamage(calculateEnemyDamage(intent.damage, actingEnemy));
        }
        if (player.getCurrentHealth() <= 0)
        {
            break;
        }
    }
    actingEnemy.gainBlock(intent.block);
    actingEnemy.applyStrength(intent.strength);
    player.applyWeak(intent.weak);
    player.applyVulnerable(intent.vulnerable);
    player.applyFrail(intent.frail);
    player.applyDexterity(intent.dexterity);
    if (intent.slimed > 0)
    {
        deck.addToDiscardPile(makeSlimedCard(), static_cast<std::size_t>(intent.slimed));
    }
    if (intent.type == EnemyIntentType::Split)
    {
        actingEnemy.resolveSplit();
    }
}

void CombatSystem::resolveEffect(const CardEffect& effect, int targetIndex, int energySpent)
{
    const auto applyToTarget = [&](auto&& action) {
        if (effect.target == CardTarget::AllEnemies)
        {
            for (Enemy& enemy : enemies)
            {
                if (!enemy.isDead()) action(enemy);
            }
        }
        else if (effect.target == CardTarget::RandomEnemy)
        {
            std::vector<std::size_t> living;
            for (std::size_t index = 0; index < enemies.size(); ++index)
            {
                if (!enemies[index].isDead()) living.push_back(index);
            }
            if (!living.empty())
            {
                const std::size_t choice = std::uniform_int_distribution<std::size_t>(0, living.size() - 1)(randomEngine);
                action(enemies[living[choice]]);
            }
        }
        else if (isLivingTarget(targetIndex))
        {
            action(enemies[static_cast<std::size_t>(targetIndex)]);
        }
    };

    switch (effect.type)
    {
    case CardEffectType::Damage:
    case CardEffectType::MultiDamage:
    {
        const int hits = effect.type == CardEffectType::MultiDamage ? hitCount(effect, energySpent) : 1;
        for (int hit = 0; hit < hits; ++hit)
        {
            applyToTarget([&](Enemy& target) {
                if (!target.isDead()) target.takeDamage(calculatePlayerDamage(effect.value, target));
            });
        }
        break;
    }
    case CardEffectType::Block:
        player.gainCardBlock(effect.value);
        break;
    case CardEffectType::Draw:
        drawHand(static_cast<std::size_t>(std::max(0, effect.value)));
        break;
    case CardEffectType::GainEnergy:
        player.gainEnergy(effect.value);
        break;
    case CardEffectType::LoseHealth:
        player.loseHealth(effect.value);
        break;
    case CardEffectType::ApplyStrength:
        if (effect.target == CardTarget::Enemy || effect.target == CardTarget::AllEnemies ||
            effect.target == CardTarget::RandomEnemy)
            applyToTarget([&](Enemy& target) { target.applyStrength(effect.value); });
        else
            player.applyStrength(effect.value);
        break;
    case CardEffectType::ApplyWeak:
        if (effect.target == CardTarget::Enemy || effect.target == CardTarget::AllEnemies ||
            effect.target == CardTarget::RandomEnemy)
            applyToTarget([&](Enemy& target) { target.applyWeak(effect.value); });
        else
            player.applyWeak(effect.value);
        break;
    case CardEffectType::ApplyVulnerable:
        if (effect.target == CardTarget::Enemy || effect.target == CardTarget::AllEnemies ||
            effect.target == CardTarget::RandomEnemy)
            applyToTarget([&](Enemy& target) { target.applyVulnerable(effect.value); });
        else
            player.applyVulnerable(effect.value);
        break;
    case CardEffectType::Exhaust:
        if ((effect.parameter == "random_hand" || effect.parameter == "choose_hand") && !deck.getHand().empty())
        {
            deck.exhaustCard(0);
            resolveTriggeredPowers("on_exhaust", targetIndex);
        }
        break;
    default:
        break;
    }
}

void CombatSystem::resolveTriggeredPowers(const char* trigger, int targetIndex)
{
    for (const CardEffect& effect : activePowerEffects)
    {
        if (effect.parameter == trigger)
        {
            resolveEffect(effect, targetIndex, 0);
        }
    }
}

int CombatSystem::calculatePlayerDamage(int baseDamage, const Enemy& target) const
{
    double damage = static_cast<double>(std::max(0, baseDamage + player.getStrength()));
    if (player.getWeak() > 0) damage *= 0.75;
    if (target.getVulnerable() > 0) damage *= 1.5;
    return std::max(0, static_cast<int>(std::floor(damage)));
}

int CombatSystem::calculateEnemyDamage(int baseDamage, const Enemy& source) const
{
    double damage = static_cast<double>(std::max(0, baseDamage + source.getStrength()));
    if (source.getWeak() > 0) damage *= 0.75;
    if (player.getVulnerable() > 0) damage *= 1.5;
    return std::max(0, static_cast<int>(std::floor(damage)));
}

void CombatSystem::drawHand(std::size_t count)
{
    const std::size_t handSize = deck.getHand().size();
    if (handSize < kMaxHandSize)
    {
        deck.drawCards(std::min(count, kMaxHandSize - handSize));
    }
}

void CombatSystem::updateBattleResult()
{
    for (std::size_t index = 0; index < enemies.size(); ++index)
    {
        if (enemies[index].isDead() && !deathPowerApplied[index] &&
            enemies[index].getDeathVulnerable() > 0)
        {
            player.applyVulnerable(enemies[index].getDeathVulnerable());
            deathPowerApplied[index] = true;
        }
    }
    if (player.getCurrentHealth() <= 0)
    {
        result = BattleResult::Defeat;
    }
    else if (!hasLivingEnemies())
    {
        result = BattleResult::Victory;
    }
}
