#pragma once

#include <string>
#include <vector>

enum class RelicRarity
{
    Starter,
    Common,
    Uncommon,
    Rare,
    Boss
};

enum class RelicTrigger
{
    OnPickup,
    BattleStart,
    BattleVictory
};

enum class RelicEffectType
{
    Heal,
    GainBlock,
    GainStrength,
    GainEnergy,
    DrawCards,
    GainGold,
    IncreaseMaxHealth,
    UpgradeAttacks,
    UpgradeSkills
};

struct RelicEffect
{
    RelicTrigger trigger = RelicTrigger::OnPickup;
    RelicEffectType type = RelicEffectType::Heal;
    int value = 0;
};

struct Relic
{
    std::string id;
    std::string name;
    std::string description;
    RelicRarity rarity = RelicRarity::Common;
    std::vector<RelicEffect> effects;
};
