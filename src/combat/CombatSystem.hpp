#pragma once

#include "card/Deck.hpp"
#include "combat/Enemy.hpp"
#include "combat/Player.hpp"

#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
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
    std::string enemyId = "cultist";
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
                     int extraDrawCards = 0, int maxHealth = 80,
                     int startingEnemyWeak = 0);
    bool playCard(int handIndex);
    int getPlayableCardCost(const Card& card) const;
    void endPlayerTurn();
    void update();

    // 仅用于最终 Boss 的一次性复活：恢复最近一次可能造成致命伤害前的完整战斗状态。
    bool reviveFromLastSafeSnapshot(int bonusStrength = 0, int bonusDexterity = 0,
                                    bool healToFull = false,
                                    bool startFreshTurn = false);

    const Player& getPlayer() const;
    const Enemy& getEnemy() const;
    const std::vector<Card>& getHandCards() const;
    const Deck& getDeck() const;
    int getEnemyIntentDamage() const;
    BattleResult getResult() const;
    bool hasPendingDiscardChoice() const;
    std::vector<Card> getDiscardChoiceCards() const;
    bool chooseDiscardCard(std::size_t choiceIndex);

private:
    struct BattleSnapshot
    {
        Player player;
        Enemy enemy;
        Deck deck;
        BattleResult result = BattleResult::Active;
        std::mt19937 randomEngine;
        bool deathPowerApplied = false;
        bool noDrawThisTurn = false;
        bool corruptionActive = false;
        bool blockRetained = false;
        int temporaryStrengthLoss = 0;
        int combustHealthLoss = 0;
        int combustDamage = 0;
        int metallicizeBlock = 0;
        int demonFormStrength = 0;
        int berserkEnergy = 0;
        int brutalityHealthLoss = 0;
        int brutalityDraw = 0;
        int darkEmbraceDraw = 0;
        int evolveDraw = 0;
        int feelNoPainBlock = 0;
        int fireBreathingDamage = 0;
        int flameBarrierDamage = 0;
        int rageBlock = 0;
        int ruptureStrength = 0;
        int juggernautDamage = 0;
        int doubleTapRemaining = 0;
        int bloodForBloodDiscount = 0;
        int lastSpentEnergy = 0;
        int lastExhaustedCount = 0;
        std::unordered_map<std::string, int> rampageBonuses;
        std::string activeCardId;
        std::vector<std::size_t> discardChoices;
        int discardChoicesRemaining = 0;
        bool enemyTurnInProgress = false;
    };

    void captureSafeSnapshot();
    void startFreshPlayerTurnAfterRevival();
    void resolveCardEffects(const Card& card, int repetitions = 1);
    void resolveEffect(const CardEffect& effect);
    void resolveEnemyIntent();
    void resolvePlayerStartTurnEffects();
    void resolvePlayerEndTurnEffects();
    void resolveEtherealHandCards();
    void processExhaustedCard(const Card& card);
    void onCardPlayed(const Card& card);
    void playTopCard();
    std::size_t drawCards(std::size_t count);
    std::size_t exhaustMatchingHand(bool allCards, bool nonAttackOnly);
    void gainPlayerBlock(int amount, bool applyModifiers = true);
    int losePlayerHealth(int amount, bool causedByCard);
    int takePlayerDamage(int amount);
    int dealCardDamage(int baseDamage, int strengthMultiplier = 1);
    int dealFixedDamage(int amount);
    bool enemyIsAttacking() const;
    Card createStatusCard(const std::string& statusId) const;
    bool cardHasEffect(const Card& card, CardEffectType type,
                       const std::string& parameter = {}) const;
    int countStrikeCards() const;
    void gainPlayerEnergy(int amount);
    int calculatePlayerDamage(int baseDamage, int strengthMultiplier = 1) const;
    int calculateEnemyDamage(int baseDamage) const;
    void drawHand(std::size_t count = 5);

    Player player;
    Enemy enemy;
    Deck deck;
    BattleResult result;
    std::uint32_t battleSeed = 0;
    std::mt19937 randomEngine;
    bool deathPowerApplied = false;
    bool noDrawThisTurn = false;
    bool corruptionActive = false;
    bool blockRetained = false;
    int temporaryStrengthLoss = 0;
    int combustHealthLoss = 0;
    int combustDamage = 0;
    int metallicizeBlock = 0;
    int demonFormStrength = 0;
    int berserkEnergy = 0;
    int brutalityHealthLoss = 0;
    int brutalityDraw = 0;
    int darkEmbraceDraw = 0;
    int evolveDraw = 0;
    int feelNoPainBlock = 0;
    int fireBreathingDamage = 0;
    int flameBarrierDamage = 0;
    int rageBlock = 0;
    int ruptureStrength = 0;
    int juggernautDamage = 0;
    int doubleTapRemaining = 0;
    int bloodForBloodDiscount = 0;
    int lastSpentEnergy = 0;
    int lastExhaustedCount = 0;
    std::unordered_map<std::string, int> rampageBonuses;
    std::string activeCardId;
    std::unique_ptr<BattleSnapshot> lastSafeSnapshot;
    std::vector<std::size_t> discardChoices;
    int discardChoicesRemaining = 0;
    int resolvingDiscardIndex = -1;
    bool enemyTurnInProgress = false;
};
