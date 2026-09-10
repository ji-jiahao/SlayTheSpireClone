#include "combat/CombatSystem.hpp"

#include "card/CardDatabase.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <random>
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

Card makeStatusCard(const std::string& statusId)
{
    Card card;
    card.id = statusId;
    card.type = CardType::Skill;
    card.rarity = CardRarity::Status;
    card.cost = -2;

    if (statusId == "wound")
    {
        card.name = "伤口";
        card.description = "无法打出。";
    }
    else if (statusId == "dazed")
    {
        card.name = "眩晕";
        card.description = "无法打出。回合结束时消耗。";
        card.effects = {{CardEffectType::Exhaust, 1, CardTarget::Self, "ethereal"}};
    }
    else if (statusId == "burn")
    {
        card.name = "灼伤";
        card.description = "回合结束时失去 2 点生命。消耗。";
    }
    else if (statusId == "dark_erosion")
    {
        card.name = "黑暗侵蚀";
        card.description = "无法打出。回合结束时失去 2 点生命。消耗。";
        card.effects = {{CardEffectType::Exhaust, 1, CardTarget::Self, "ethereal"}};
    }
    else
    {
        card.id = "slimed";
        card.name = "黏液";
        card.cost = 1;
        card.description = "消耗。";
        card.effects = {{CardEffectType::Exhaust, 1, CardTarget::Self, {}}};
    }

    card.upgradedCost = card.cost;
    card.upgradedDescription = card.description;
    card.upgradedEffects = card.effects;
    return card;
}
} // namespace

CombatSystem::CombatSystem()
    : player(kPlayerMaxHealth, kPlayerMaxEnergy),
      enemy("邪教徒", kCultistMaxHealth),
      deck(),
      result(BattleResult::Active),
      randomEngine(0)
{
    enemy.setIntentDamage(kCultistIntentDamage);
}

void CombatSystem::startBattle(int currentHealth, std::uint32_t seed)
{
    startBattle(currentHealth, seed, CardDatabase::createStarterDeck());
}

void CombatSystem::startBattle(int currentHealth, std::uint32_t seed,
                               std::vector<Card> cards)
{
    startBattle(currentHealth, seed, std::move(cards), EncounterDefinition{}, 0, 0);
}

void CombatSystem::startBattle(int currentHealth, std::uint32_t seed,
                               std::vector<Card> cards,
                               const EncounterDefinition& encounter, int startingBlock,
                               int startingStrength, int startingEnergy,
                               int extraDrawCards, int maxHealth, int startingEnemyWeak)
{
    battleSeed = seed;
    randomEngine.seed(seed ^ 0x9e3779b9u);
    player = Player(maxHealth, kPlayerMaxEnergy, currentHealth);
    enemy = Enemy(encounter.enemyId, encounter.enemyName, encounter.enemyHealth, seed + 97u);
    if (enemy.getArchetype() == EnemyArchetype::Generic)
    {
        enemy.setIntentDamage(encounter.intentDamage);
    }
    deck = Deck(std::move(cards));
    deck.shuffle(seed);
    result = BattleResult::Active;
    lastSafeSnapshot.reset();

    deathPowerApplied = false;
    noDrawThisTurn = false;
    corruptionActive = false;
    blockRetained = false;
    temporaryStrengthLoss = 0;
    combustHealthLoss = 0;
    combustDamage = 0;
    metallicizeBlock = 0;
    demonFormStrength = 0;
    berserkEnergy = 0;
    brutalityHealthLoss = 0;
    brutalityDraw = 0;
    darkEmbraceDraw = 0;
    evolveDraw = 0;
    feelNoPainBlock = 0;
    fireBreathingDamage = 0;
    flameBarrierDamage = 0;
    rageBlock = 0;
    ruptureStrength = 0;
    juggernautDamage = 0;
    doubleTapRemaining = 0;
    bloodForBloodDiscount = 0;
    lastSpentEnergy = 0;
    lastExhaustedCount = 0;
    rampageBonuses.clear();
    activeCardId.clear();
    discardChoices.clear();
    discardChoicesRemaining = 0;
    resolvingDiscardIndex = -1;
    enemyTurnInProgress = false;

    player.gainBlock(startingBlock);
    player.applyStrength(startingStrength);
    enemy.applyWeak(startingEnemyWeak);
    player.gainEnergy(startingEnergy);
    drawHand(kHandSize + static_cast<std::size_t>(std::max(0, extraDrawCards)));
    captureSafeSnapshot();
}

int CombatSystem::getPlayableCardCost(const Card& card) const
{
    if (card.cost <= -2)
    {
        return -1;
    }

    if (card.cost == -1)
    {
        return player.getCurrentEnergy();
    }

    int cost = card.cost;
    if (card.id == "blood_for_blood")
    {
        cost = std::max(0, cost - bloodForBloodDiscount);
    }
    if (corruptionActive && card.type == CardType::Skill)
    {
        cost = 0;
    }
    return cost;
}

bool CombatSystem::playCard(int handIndex)
{
    if (result != BattleResult::Active || hasPendingDiscardChoice() || handIndex < 0 ||
        static_cast<std::size_t>(handIndex) >= deck.getHand().size())
    {
        return false;
    }

    const std::size_t index = static_cast<std::size_t>(handIndex);
    const Card card = deck.getHand()[index];
    if (card.id == "clash" &&
        std::any_of(deck.getHand().begin(), deck.getHand().end(),
                    [](const Card& handCard)
                    {
                        return handCard.type != CardType::Attack;
                    }))
    {
        return false;
    }

    const int cost = getPlayableCardCost(card);
    if (cost < 0 || !player.spendEnergy(cost))
    {
        return false;
    }

    lastSpentEnergy = cost;
    resolvingDiscardIndex = -1;
    const bool exhaustPlayedCard =
        card.type == CardType::Power ||
        cardHasEffect(card, CardEffectType::Exhaust) ||
        (corruptionActive && card.type == CardType::Skill);
    if (exhaustPlayedCard)
    {
        deck.exhaustCard(index);
        processExhaustedCard(card);
    }
    else
    {
        deck.discardCard(index);
        resolvingDiscardIndex = static_cast<int>(deck.getDiscardPile().size()) - 1;
    }

    onCardPlayed(card);
    int repetitions = 1;
    if (card.type == CardType::Attack && doubleTapRemaining > 0)
    {
        repetitions = 2;
        --doubleTapRemaining;
    }
    resolveCardEffects(card, repetitions);
    resolvingDiscardIndex = -1;

    if (card.id == "rampage")
    {
        ++rampageBonuses[card.id];
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

    if (result == BattleResult::Active)
    {
        captureSafeSnapshot();
    }
    return true;
}

void CombatSystem::endPlayerTurn()
{
    if (result != BattleResult::Active || hasPendingDiscardChoice())
    {
        return;
    }

    resolvePlayerEndTurnEffects();
    rageBlock = 0;
    doubleTapRemaining = 0;
    if (player.getCurrentHealth() <= 0)
    {
        result = BattleResult::Defeat;
        return;
    }
    if (enemy.isDead())
    {
        result = BattleResult::Victory;
        return;
    }

    resolveEtherealHandCards();
    if (player.getCurrentHealth() <= 0)
    {
        result = BattleResult::Defeat;
        return;
    }

    deck.discardHand();
    player.endTurn();
    enemyTurnInProgress = true;
    enemy.startTurn();
    const bool splitIntent = enemy.getIntent().type == EnemyIntentType::Split;
    resolveEnemyIntent();
    enemy.endTurn();
    enemyTurnInProgress = false;

    if (player.getCurrentHealth() <= 0)
    {
        result = BattleResult::Defeat;
        return;
    }
    if (enemy.isDead())
    {
        result = BattleResult::Victory;
        return;
    }

    if (!splitIntent)
    {
        enemy.advanceIntent();
    }

    resolvePlayerStartTurnEffects();
    if (player.getCurrentHealth() <= 0)
    {
        result = BattleResult::Defeat;
        return;
    }
    drawHand();
    if (result == BattleResult::Active)
    {
        captureSafeSnapshot();
    }
}

void CombatSystem::update()
{
}

bool CombatSystem::reviveFromLastSafeSnapshot(int bonusStrength, int bonusDexterity,
                                              bool healToFull, bool startFreshTurn)
{
    if (!lastSafeSnapshot)
    {
        return false;
    }

    const BattleSnapshot snapshot = *lastSafeSnapshot;
    player = snapshot.player;
    enemy = snapshot.enemy;
    deck = snapshot.deck;
    result = snapshot.result;
    randomEngine = snapshot.randomEngine;
    deathPowerApplied = snapshot.deathPowerApplied;
    noDrawThisTurn = snapshot.noDrawThisTurn;
    corruptionActive = snapshot.corruptionActive;
    blockRetained = snapshot.blockRetained;
    temporaryStrengthLoss = snapshot.temporaryStrengthLoss;
    combustHealthLoss = snapshot.combustHealthLoss;
    combustDamage = snapshot.combustDamage;
    metallicizeBlock = snapshot.metallicizeBlock;
    demonFormStrength = snapshot.demonFormStrength;
    berserkEnergy = snapshot.berserkEnergy;
    brutalityHealthLoss = snapshot.brutalityHealthLoss;
    brutalityDraw = snapshot.brutalityDraw;
    darkEmbraceDraw = snapshot.darkEmbraceDraw;
    evolveDraw = snapshot.evolveDraw;
    feelNoPainBlock = snapshot.feelNoPainBlock;
    fireBreathingDamage = snapshot.fireBreathingDamage;
    flameBarrierDamage = snapshot.flameBarrierDamage;
    rageBlock = snapshot.rageBlock;
    ruptureStrength = snapshot.ruptureStrength;
    juggernautDamage = snapshot.juggernautDamage;
    doubleTapRemaining = snapshot.doubleTapRemaining;
    bloodForBloodDiscount = snapshot.bloodForBloodDiscount;
    lastSpentEnergy = snapshot.lastSpentEnergy;
    lastExhaustedCount = snapshot.lastExhaustedCount;
    rampageBonuses = snapshot.rampageBonuses;
    activeCardId = snapshot.activeCardId;
    discardChoices = snapshot.discardChoices;
    discardChoicesRemaining = snapshot.discardChoicesRemaining;
    enemyTurnInProgress = snapshot.enemyTurnInProgress;
    resolvingDiscardIndex = -1;
    if (bonusStrength != 0)
    {
        player.applyStrength(bonusStrength);
    }
    if (bonusDexterity != 0)
    {
        player.applyDexterity(bonusDexterity);
    }
    if (healToFull)
    {
        player.heal(player.getMaxHealth());
    }
    result = BattleResult::Active;
    if (startFreshTurn)
    {
        startFreshPlayerTurnAfterRevival();
    }
    return true;
}

void CombatSystem::startFreshPlayerTurnAfterRevival()
{
    // 只有敌方行动确实被复活打断时，才结束该行动并推进意图。
    const EnemyIntent intent = enemy.getIntent();
    if (enemyTurnInProgress && enemy.getArchetype() == EnemyArchetype::Belial &&
        intent.damage > 0 &&
        intent.name.find("光线") != std::string::npos)
    {
        enemy.consumeDarkCharge();
    }

    if (enemyTurnInProgress)
    {
        enemy.endTurn();
        enemy.advanceIntent();
        enemyTurnInProgress = false;
    }
    resolvePlayerStartTurnEffects();
    if (player.getCurrentHealth() <= 0)
    {
        result = BattleResult::Defeat;
        return;
    }

    deck.discardHand();
    drawHand();
    captureSafeSnapshot();
}

void CombatSystem::captureSafeSnapshot()
{
    auto snapshot = std::make_unique<BattleSnapshot>();
    snapshot->player = player;
    snapshot->enemy = enemy;
    snapshot->deck = deck;
    snapshot->result = result;
    snapshot->randomEngine = randomEngine;
    snapshot->deathPowerApplied = deathPowerApplied;
    snapshot->noDrawThisTurn = noDrawThisTurn;
    snapshot->corruptionActive = corruptionActive;
    snapshot->blockRetained = blockRetained;
    snapshot->temporaryStrengthLoss = temporaryStrengthLoss;
    snapshot->combustHealthLoss = combustHealthLoss;
    snapshot->combustDamage = combustDamage;
    snapshot->metallicizeBlock = metallicizeBlock;
    snapshot->demonFormStrength = demonFormStrength;
    snapshot->berserkEnergy = berserkEnergy;
    snapshot->brutalityHealthLoss = brutalityHealthLoss;
    snapshot->brutalityDraw = brutalityDraw;
    snapshot->darkEmbraceDraw = darkEmbraceDraw;
    snapshot->evolveDraw = evolveDraw;
    snapshot->feelNoPainBlock = feelNoPainBlock;
    snapshot->fireBreathingDamage = fireBreathingDamage;
    snapshot->flameBarrierDamage = flameBarrierDamage;
    snapshot->rageBlock = rageBlock;
    snapshot->ruptureStrength = ruptureStrength;
    snapshot->juggernautDamage = juggernautDamage;
    snapshot->doubleTapRemaining = doubleTapRemaining;
    snapshot->bloodForBloodDiscount = bloodForBloodDiscount;
    snapshot->lastSpentEnergy = lastSpentEnergy;
    snapshot->lastExhaustedCount = lastExhaustedCount;
    snapshot->rampageBonuses = rampageBonuses;
    snapshot->activeCardId = activeCardId;
    snapshot->discardChoices = discardChoices;
    snapshot->discardChoicesRemaining = discardChoicesRemaining;
    snapshot->enemyTurnInProgress = enemyTurnInProgress;
    lastSafeSnapshot = std::move(snapshot);
}

const Player& CombatSystem::getPlayer() const { return player; }
const Enemy& CombatSystem::getEnemy() const { return enemy; }
const std::vector<Card>& CombatSystem::getHandCards() const
{
    return deck.getHand();
}
const Deck& CombatSystem::getDeck() const { return deck; }
int CombatSystem::getEnemyIntentDamage() const
{
    return calculateEnemyDamage(enemy.getIntentDamage());
}
BattleResult CombatSystem::getResult() const { return result; }

bool CombatSystem::hasPendingDiscardChoice() const
{
    return result == BattleResult::Active && discardChoicesRemaining > 0 && !discardChoices.empty();
}

std::vector<Card> CombatSystem::getDiscardChoiceCards() const
{
    std::vector<Card> cards;
    if (hasPendingDiscardChoice())
        for (auto i : discardChoices) cards.push_back(deck.getDiscardPile()[i]);
    return cards;
}

bool CombatSystem::chooseDiscardCard(std::size_t choiceIndex)
{
    if (!hasPendingDiscardChoice() || choiceIndex >= discardChoices.size()) return false;
    const auto index = discardChoices[choiceIndex];
    if (!deck.moveDiscardCardToDrawPileTop(index)) return false;
    discardChoices.erase(discardChoices.begin() + static_cast<std::ptrdiff_t>(choiceIndex));
    for (auto& i : discardChoices) if (i > index) --i;
    if (--discardChoicesRemaining == 0 || discardChoices.empty())
    {
        discardChoicesRemaining = 0;
        discardChoices.clear();
    }
    captureSafeSnapshot();
    return true;
}

void CombatSystem::resolveCardEffects(const Card& card, int repetitions)
{
    const std::string previousCardId = activeCardId;
    activeCardId = card.id;
    lastExhaustedCount = 0;

    for (int repetition = 0; repetition < repetitions && !enemy.isDead() &&
                               player.getCurrentHealth() > 0;
         ++repetition)
    {
        for (const CardEffect& effect : card.effects)
        {
            resolveEffect(effect);
            if (player.getCurrentHealth() <= 0)
            {
                break;
            }
        }
    }

    activeCardId = previousCardId;
}

void CombatSystem::resolveEffect(const CardEffect& effect)
{
    if (effect.parameter == "if_enemy_attacking" && !enemyIsAttacking())
    {
        return;
    }
    if (effect.parameter == "if_vulnerable" && enemy.getVulnerable() <= 0)
    {
        return;
    }

    switch (effect.type)
    {
    case CardEffectType::Damage:
    {
        if (effect.parameter == "on_gain_block")
        {
            juggernautDamage += effect.value;
            return;
        }

        int baseDamage = effect.value;
        int strengthMultiplier = 1;
        if (effect.parameter == "player_block")
        {
            baseDamage = player.getBlock();
            strengthMultiplier = 0;
        }
        else if (effect.parameter == "strength_multiplier_3")
        {
            strengthMultiplier = 3;
        }
        else if (effect.parameter == "strength_multiplier_5")
        {
            strengthMultiplier = 5;
        }
        else if (effect.parameter == "strike_count" ||
                 effect.parameter == "strike_count_upgraded")
        {
            const int strikeBonus = effect.parameter == "strike_count_upgraded" ? 3 : 2;
            baseDamage += countStrikeCards() * strikeBonus;
        }
        else if (effect.parameter == "repeated_plus_5")
        {
            baseDamage += rampageBonuses[activeCardId] * 5;
        }

        const int unblockedDamage = dealCardDamage(baseDamage, strengthMultiplier);
        if (effect.parameter == "gain_max_health_on_kill" && enemy.isDead())
        {
            player.increaseMaxHealth(effect.value >= 12 ? 4 : 3);
        }
        (void)unblockedDamage;
        break;
    }
    case CardEffectType::MultiDamage:
    {
        if (effect.parameter == "on_status_draw")
        {
            fireBreathingDamage += effect.value;
            return;
        }
        if (effect.parameter == "end_turn")
        {
            combustDamage += effect.value;
            return;
        }

        int hits = hitCount(effect);
        if (effect.parameter == "x_cost")
        {
            hits = lastSpentEnergy;
        }
        else if (effect.parameter == "per_exhausted_card")
        {
            hits = lastExhaustedCount;
        }
        if (hits <= 0)
        {
            return;
        }

        int totalUnblockedDamage = 0;
        for (int hit = 0; hit < hits && !enemy.isDead(); ++hit)
        {
            totalUnblockedDamage += dealCardDamage(effect.value);
        }
        if (effect.parameter == "heal_unblocked")
        {
            player.heal(totalUnblockedDamage);
        }
        break;
    }
    case CardEffectType::Block:
        if (effect.parameter == "double_block")
        {
            gainPlayerBlock(player.getBlock(), false);
        }
        else if (effect.parameter == "on_attack_played")
        {
            rageBlock += effect.value;
        }
        else if (effect.parameter == "on_exhaust")
        {
            feelNoPainBlock += effect.value;
        }
        else if (effect.parameter == "per_non_attack_exhausted")
        {
            lastExhaustedCount =
                static_cast<int>(exhaustMatchingHand(true, true));
            gainPlayerBlock(effect.value * lastExhaustedCount);
        }
        else if (effect.parameter == "end_turn")
        {
            metallicizeBlock += effect.value;
        }
        else
        {
            gainPlayerBlock(effect.value);
        }
        break;
    case CardEffectType::Draw:
        if (effect.parameter == "on_exhaust")
        {
            darkEmbraceDraw += effect.value;
        }
        else if (effect.parameter == "on_status_draw")
        {
            evolveDraw += effect.value;
        }
        else if (effect.parameter == "start_turn")
        {
            brutalityDraw += effect.value;
        }
        else
        {
            drawCards(static_cast<std::size_t>(std::max(0, effect.value)));
        }
        break;
    case CardEffectType::GainEnergy:
        if (effect.parameter == "on_exhaust")
        {
            return;
        }
        if (effect.parameter == "start_turn")
        {
            berserkEnergy += effect.value;
        }
        else
        {
            gainPlayerEnergy(effect.value);
        }
        break;
    case CardEffectType::LoseHealth:
        if (effect.parameter == "end_turn")
        {
            combustHealthLoss += effect.value;
        }
        else if (effect.parameter == "start_turn")
        {
            brutalityHealthLoss += effect.value;
        }
        else
        {
            losePlayerHealth(effect.value, true);
        }
        break;
    case CardEffectType::ApplyStrength:
        if (effect.parameter == "end_turn")
        {
            temporaryStrengthLoss += std::max(0, -effect.value);
        }
        else if (effect.parameter == "start_turn")
        {
            demonFormStrength += effect.value;
        }
        else if (effect.parameter == "on_card_lose_health")
        {
            ruptureStrength += effect.value;
        }
        else if (effect.parameter == "double")
        {
            player.applyStrength(player.getStrength());
        }
        else if (effect.target == CardTarget::Enemy ||
                 effect.target == CardTarget::AllEnemies)
        {
            enemy.applyStrength(effect.value);
        }
        else
        {
            player.applyStrength(effect.value);
        }
        break;
    case CardEffectType::ApplyWeak:
        if (effect.target == CardTarget::Enemy ||
            effect.target == CardTarget::AllEnemies)
        {
            enemy.applyWeak(effect.value);
        }
        else
        {
            player.applyWeak(effect.value);
        }
        break;
    case CardEffectType::ApplyVulnerable:
        if (effect.target == CardTarget::Enemy ||
            effect.target == CardTarget::AllEnemies)
        {
            enemy.applyVulnerable(effect.value);
        }
        else
        {
            player.applyVulnerable(effect.value);
        }
        break;
    case CardEffectType::ApplyDexterity:
        if (effect.target != CardTarget::Enemy &&
            effect.target != CardTarget::AllEnemies)
        {
            player.applyDexterity(effect.value);
        }
        break;
    case CardEffectType::AddCard:
        if (effect.parameter == "flame_barrier")
        {
            flameBarrierDamage += effect.value;
        }
        else if (effect.parameter == "copy_attack_or_power")
        {
            Card selected;
            bool found = false;
            for (const Card& card : deck.getHand())
            {
                if (card.type == CardType::Attack || card.type == CardType::Power)
                {
                    selected = card;
                    found = true;
                    break;
                }
            }
            if (found)
            {
                for (int copy = 0; copy < effect.value; ++copy)
                {
                    deck.addToHand(selected);
                }
            }
        }
        else if (effect.parameter == "random_attack_cost_zero")
        {
            std::vector<Card> attacks;
            for (const Card& card : CardDatabase::createIroncladCardPool())
            {
                if (card.type == CardType::Attack && card.rarity != CardRarity::Starter)
                {
                    attacks.push_back(card);
                }
            }
            if (!attacks.empty())
            {
                const std::size_t index = std::uniform_int_distribution<std::size_t>(
                    0, attacks.size() - 1)(randomEngine);
                attacks[index].cost = 0;
                deck.addToHand(attacks[index]);
            }
        }
        else if (effect.parameter == "from_exhaust")
        {
            bool skippedCurrentCard = false;
            for (std::size_t index = deck.getExhaustPile().size(); index > 0; --index)
            {
                const std::size_t exhaustIndex = index - 1;
                if (!skippedCurrentCard &&
                    deck.getExhaustPile()[exhaustIndex].id == activeCardId)
                {
                    skippedCurrentCard = true;
                    continue;
                }
                if (deck.moveExhaustCardToHand(exhaustIndex))
                {
                    break;
                }
            }
        }
        else
        {
            const Card status = createStatusCard(effect.parameter);
            if (effect.parameter == "wound_to_hand")
            {
                for (int count = 0; count < effect.value; ++count)
                {
                    deck.addToHand(status);
                }
            }
            else if (effect.parameter == "dazed_to_draw")
            {
                for (int count = 0; count < effect.value; ++count)
                {
                    deck.addToDrawPileTop(status);
                }
            }
            else
            {
                deck.addToDiscardPile(status, static_cast<std::size_t>(
                                                    std::max(0, effect.value)));
            }
        }
        break;
    case CardEffectType::Exhaust:
        if (effect.parameter.empty() || effect.parameter == "ethereal")
        {
            return;
        }
        if (effect.parameter == "random_hand" ||
            effect.parameter == "choose_hand")
        {
            if (!deck.getHand().empty())
            {
                const Card card = deck.getHand().front();
                if (deck.exhaustCard(0))
                {
                    processExhaustedCard(card);
                    lastExhaustedCount = 1;
                }
            }
        }
        else if (effect.parameter == "all_non_attacks")
        {
            lastExhaustedCount = static_cast<int>(exhaustMatchingHand(true, true));
        }
        else if (effect.parameter == "per_non_attack_exhausted")
        {
            lastExhaustedCount = static_cast<int>(exhaustMatchingHand(true, true));
            gainPlayerBlock(effect.value * lastExhaustedCount);
        }
        else if (effect.parameter == "all_hand")
        {
            lastExhaustedCount = static_cast<int>(exhaustMatchingHand(true, false));
        }
        break;
    case CardEffectType::Heal:
        player.heal(effect.value);
        break;
    case CardEffectType::Retain:
        if (effect.parameter == "block")
        {
            blockRetained = true;
            player.setBlockRetained(true);
        }
        else if (effect.parameter == "discard_to_top")
        {
            if (enemy.isDead() || player.getCurrentHealth() <= 0) break;
            if (discardChoicesRemaining == 0)
                for (std::size_t i = 0; i < deck.getDiscardPile().size(); ++i)
                    if (static_cast<int>(i) != resolvingDiscardIndex) discardChoices.push_back(i);
            if (!discardChoices.empty()) ++discardChoicesRemaining;
        }
        else if (effect.parameter == "hand_to_top" && !deck.getHand().empty())
        {
            deck.moveHandCardToDrawPileTop(0);
        }
        break;
    case CardEffectType::ReduceCost:
        if (effect.parameter == "skills_cost_zero_exhaust")
        {
            corruptionActive = true;
        }
        break;
    case CardEffectType::UpgradeCard:
        if (effect.value < 0)
        {
            deck.upgradeAllHandCards();
        }
        else
        {
            for (int count = 0; count < effect.value; ++count)
            {
                bool upgraded = false;
                for (std::size_t index = 0; index < deck.getHand().size(); ++index)
                {
                    if (deck.upgradeHandCard(index))
                    {
                        upgraded = true;
                        break;
                    }
                }
                if (!upgraded)
                {
                    break;
                }
            }
        }
        break;
    case CardEffectType::PlayTopCard:
        if (effect.parameter == "next_attack_twice" ||
            effect.parameter == "next_two_attacks_twice")
        {
            doubleTapRemaining += effect.value;
        }
        else
        {
            playTopCard();
        }
        break;
    case CardEffectType::EndTurn:
        if (effect.parameter == "no_draw")
        {
            noDrawThisTurn = true;
        }
        break;
    case CardEffectType::Discard:
        break;
    }
}

void CombatSystem::resolveEnemyIntent()
{
    const EnemyIntent intent = enemy.getIntent();
    const int perHitDamage = intent.damage > 0 ? getEnemyIntentDamage() : 0;
    for (int hit = 0; hit < std::max(1, intent.hits); ++hit)
    {
        if (intent.damage > 0)
        {
            takePlayerDamage(perHitDamage);
            if (flameBarrierDamage > 0 && !enemy.isDead())
            {
                enemy.takeDamage(flameBarrierDamage);
            }
        }
        if (player.getCurrentHealth() <= 0 || enemy.isDead())
        {
            break;
        }
    }

    if (player.getCurrentHealth() <= 0 || enemy.isDead())
    {
        return;
    }

    enemy.gainBlock(intent.block);
    enemy.applyStrength(intent.strength);
    player.applyWeak(intent.weak);
    player.applyVulnerable(intent.vulnerable);
    player.applyFrail(intent.frail);
    player.applyDexterity(intent.dexterity);
    if (intent.slimed > 0)
    {
        deck.addToDiscardPile(createStatusCard("slimed"),
                              static_cast<std::size_t>(intent.slimed));
    }
    if (intent.darkErosion > 0)
    {
        deck.addToDrawPileTop(createStatusCard("dark_erosion"));
    }
    if (enemy.getArchetype() == EnemyArchetype::Belial && intent.damage > 0 &&
        intent.name.find("光线") != std::string::npos)
    {
        enemy.consumeDarkCharge();
    }
    if (intent.type == EnemyIntentType::Split)
    {
        enemy.resolveSplit();
    }
}

void CombatSystem::resolvePlayerStartTurnEffects()
{
    noDrawThisTurn = false;
    player.startTurn(blockRetained);
    player.applyStrength(demonFormStrength);
    gainPlayerEnergy(berserkEnergy);

    if (brutalityHealthLoss > 0)
    {
        losePlayerHealth(brutalityHealthLoss, true);
    }
    if (player.getCurrentHealth() > 0 && brutalityDraw > 0)
    {
        drawCards(static_cast<std::size_t>(brutalityDraw));
    }
}

void CombatSystem::resolvePlayerEndTurnEffects()
{
    if (temporaryStrengthLoss > 0)
    {
        player.applyStrength(-temporaryStrengthLoss);
        temporaryStrengthLoss = 0;
    }
    if (combustHealthLoss > 0)
    {
        losePlayerHealth(combustHealthLoss, true);
    }
    if (player.getCurrentHealth() <= 0)
    {
        return;
    }
    if (combustDamage > 0)
    {
        dealFixedDamage(combustDamage);
    }
    if (metallicizeBlock > 0)
    {
        gainPlayerBlock(metallicizeBlock, false);
    }
}

void CombatSystem::resolveEtherealHandCards()
{
    const std::size_t initialSize = deck.getHand().size();
    for (std::size_t index = initialSize; index > 0; --index)
    {
        const std::size_t handIndex = index - 1;
        const Card& card = deck.getHand()[handIndex];
        const bool burnCard = card.id == "burn" || card.id == "dark_erosion";
        const bool ethereal = cardHasEffect(card, CardEffectType::Exhaust, "ethereal");
        if (!burnCard && !ethereal)
        {
            continue;
        }

        const Card exhaustedCard = card;
        if (burnCard)
        {
            losePlayerHealth(2, true);
        }
        if (deck.exhaustCard(handIndex))
        {
            processExhaustedCard(exhaustedCard);
        }
        if (player.getCurrentHealth() <= 0)
        {
            return;
        }
    }
}

void CombatSystem::processExhaustedCard(const Card& card)
{
    if (darkEmbraceDraw > 0)
    {
        drawCards(static_cast<std::size_t>(darkEmbraceDraw));
    }
    if (feelNoPainBlock > 0)
    {
        gainPlayerBlock(feelNoPainBlock);
    }

    for (const CardEffect& effect : card.effects)
    {
        if (effect.parameter == "on_exhaust" &&
            effect.type == CardEffectType::GainEnergy)
        {
            gainPlayerEnergy(effect.value);
        }
    }
}

void CombatSystem::onCardPlayed(const Card& card)
{
    if (card.type == CardType::Attack && rageBlock > 0)
    {
        gainPlayerBlock(rageBlock);
    }
}

void CombatSystem::playTopCard()
{
    const std::optional<Card> topCard = deck.takeTopDrawCard();
    if (!topCard.has_value())
    {
        return;
    }

    const Card card = *topCard;
    const int previousDiscardIndex = resolvingDiscardIndex;
    resolvingDiscardIndex = -1;
    const int previousSpentEnergy = lastSpentEnergy;
    lastSpentEnergy = 0;
    onCardPlayed(card);
    deck.addToExhaustPile(card);
    processExhaustedCard(card);

    int repetitions = 1;
    if (card.type == CardType::Attack && doubleTapRemaining > 0)
    {
        repetitions = 2;
        --doubleTapRemaining;
    }
    resolveCardEffects(card, repetitions);
    lastSpentEnergy = previousSpentEnergy;
    resolvingDiscardIndex = previousDiscardIndex;
}

std::size_t CombatSystem::drawCards(std::size_t count)
{
    if (noDrawThisTurn || count == 0)
    {
        return 0;
    }

    const std::vector<Card> drawnCards = deck.drawCards(count);
    for (const Card& card : drawnCards)
    {
        if (card.rarity != CardRarity::Status && card.rarity != CardRarity::Curse)
        {
            continue;
        }

        if (evolveDraw > 0)
        {
            drawCards(static_cast<std::size_t>(evolveDraw));
        }
        if (fireBreathingDamage > 0 && !enemy.isDead())
        {
            dealFixedDamage(fireBreathingDamage);
        }
    }
    return drawnCards.size();
}

std::size_t CombatSystem::exhaustMatchingHand(bool allCards, bool nonAttackOnly)
{
    const std::size_t initialSize = deck.getHand().size();
    std::size_t exhaustedCount = 0;
    for (std::size_t index = initialSize; index > 0; --index)
    {
        const std::size_t handIndex = index - 1;
        const Card& card = deck.getHand()[handIndex];
        if (nonAttackOnly && card.type == CardType::Attack)
        {
            continue;
        }

        const Card exhaustedCard = card;
        if (deck.exhaustCard(handIndex))
        {
            processExhaustedCard(exhaustedCard);
            ++exhaustedCount;
        }
        if (!allCards)
        {
            break;
        }
    }
    return exhaustedCount;
}

void CombatSystem::gainPlayerBlock(int amount, bool applyModifiers)
{
    const int gained = applyModifiers ? player.gainCardBlock(amount)
                                      : player.gainBlock(amount);
    if (gained > 0 && juggernautDamage > 0 && !enemy.isDead())
    {
        enemy.takeDamage(juggernautDamage);
    }
}

int CombatSystem::losePlayerHealth(int amount, bool causedByCard)
{
    const int lost = player.loseHealth(amount);
    if (lost > 0)
    {
        ++bloodForBloodDiscount;
        if (causedByCard && ruptureStrength > 0)
        {
            player.applyStrength(ruptureStrength);
        }
    }
    return lost;
}

int CombatSystem::takePlayerDamage(int amount)
{
    const int potentialUnblockedDamage =
        std::max(0, amount - player.getBlock());
    if (potentialUnblockedDamage >= player.getCurrentHealth())
    {
        captureSafeSnapshot();
    }

    const int lost = player.takeDamage(amount);
    if (lost > 0)
    {
        ++bloodForBloodDiscount;
    }
    return lost;
}

int CombatSystem::dealCardDamage(int baseDamage, int strengthMultiplier)
{
    return enemy.takeDamage(calculatePlayerDamage(baseDamage, strengthMultiplier));
}

int CombatSystem::dealFixedDamage(int amount)
{
    return enemy.takeDamage(std::max(0, amount));
}

bool CombatSystem::enemyIsAttacking() const
{
    return enemy.getIntent().damage > 0;
}

Card CombatSystem::createStatusCard(const std::string& statusId) const
{
    return makeStatusCard(statusId);
}

bool CombatSystem::cardHasEffect(const Card& card, CardEffectType type,
                                 const std::string& parameter) const
{
    return std::any_of(card.effects.begin(), card.effects.end(),
                       [type, &parameter](const CardEffect& effect)
                       {
                           return effect.type == type &&
                                  effect.parameter == parameter;
                       });
}

int CombatSystem::calculatePlayerDamage(int baseDamage, int strengthMultiplier) const
{
    double damage = static_cast<double>(
        std::max(0, baseDamage + player.getStrength() * strengthMultiplier));
    if (player.getWeak() > 0)
    {
        damage *= 0.75;
    }
    if (enemy.getVulnerable() > 0)
    {
        damage *= 1.5;
    }
    return std::max(0, static_cast<int>(std::floor(damage)));
}

int CombatSystem::calculateEnemyDamage(int baseDamage) const
{
    double damage = static_cast<double>(
        std::max(0, baseDamage + enemy.getStrength()));
    if (enemy.getWeak() > 0)
    {
        damage *= 0.75;
    }
    if (player.getVulnerable() > 0)
    {
        damage *= 1.5;
    }
    return std::max(0, static_cast<int>(std::floor(damage)));
}

int CombatSystem::countStrikeCards() const
{
    int count = 0;
    const auto countIn = [&count](const std::vector<Card>& cards)
    {
        for (const Card& card : cards)
        {
            if (card.name.find("打击") != std::string::npos)
            {
                ++count;
            }
        }
    };

    countIn(deck.getDrawPile());
    countIn(deck.getHand());
    countIn(deck.getDiscardPile());
    countIn(deck.getExhaustPile());
    return count;
}

void CombatSystem::gainPlayerEnergy(int amount)
{
    player.gainEnergy(amount);
}

void CombatSystem::drawHand(std::size_t count)
{
    drawCards(count);
}
