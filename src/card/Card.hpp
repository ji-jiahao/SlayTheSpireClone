#pragma once

#include <string>
#include <vector>

enum class CardType
{
    Attack,
    Skill,
    Power
};

enum class CardRarity
{
    Starter,
    Common,
    Uncommon,
    Rare,
    Status,
    Curse
};

enum class CardTarget
{
    None,
    Self,
    Enemy,
    AllEnemies,
    RandomEnemy
};

enum class CardEffectType
{
    Damage,
    MultiDamage,
    Block,
    Draw,
    Discard,
    GainEnergy,
    LoseHealth,
    Heal,
    ApplyStrength,
    ApplyWeak,
    ApplyVulnerable,
    ApplyDexterity,
    AddCard,
    Exhaust,
    Retain,
    ReduceCost,
    UpgradeCard,
    PlayTopCard,
    EndTurn
};

struct CardEffect
{
    CardEffectType type = CardEffectType::Damage;
    int value = 0;
    CardTarget target = CardTarget::Self;
    std::string parameter;
};

struct Card
{
    std::string id;
    std::string name;
    CardType type = CardType::Skill;
    CardRarity rarity = CardRarity::Common;
    int cost = 0;
    int damage = 0;
    int block = 0;
    std::string description;
    std::vector<CardEffect> effects;
    int upgradedCost = -1;
    std::string upgradedDescription;
    std::vector<CardEffect> upgradedEffects;
};

struct CardInstance
{
    std::string definitionId;
    bool upgraded = false;
};
