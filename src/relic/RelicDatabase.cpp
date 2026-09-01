#include "relic/RelicDatabase.hpp"

#include <stdexcept>

namespace
{
Relic relic(const char* id, const char* name, const char* description,
            RelicRarity rarity, RelicTrigger trigger, RelicEffectType effectType,
            int value)
{
    return {id, name, description, rarity, {{trigger, effectType, value}}};
}
}

Relic RelicDatabase::createBurningBlood()
{
    return relic("burning_blood", "燃烧之血", "战斗胜利后回复 6 点生命。",
                 RelicRarity::Starter, RelicTrigger::BattleVictory,
                 RelicEffectType::Heal, 6);
}

Relic RelicDatabase::createAnchor()
{
    return relic("anchor", "锚", "每场战斗开始时获得 10 点格挡。",
                 RelicRarity::Common, RelicTrigger::BattleStart,
                 RelicEffectType::GainBlock, 10);
}

Relic RelicDatabase::createVajra()
{
    return relic("vajra", "金刚杵", "每场战斗开始时获得 1 点力量。",
                 RelicRarity::Common, RelicTrigger::BattleStart,
                 RelicEffectType::GainStrength, 1);
}

std::vector<Relic> RelicDatabase::createFoundationRelics()
{
    return {
        createBurningBlood(),
        createAnchor(),
        createVajra(),
        relic("lantern", "灯笼", "每场战斗的第一回合获得 1 点能量。",
              RelicRarity::Common, RelicTrigger::BattleStart,
              RelicEffectType::GainEnergy, 1),
        relic("bag_of_preparation", "准备背包", "每场战斗开始时额外抽 2 张牌。",
              RelicRarity::Common, RelicTrigger::BattleStart,
              RelicEffectType::DrawCards, 2),
        relic("blood_vial", "血瓶", "每场战斗开始时回复 2 点生命。",
              RelicRarity::Common, RelicTrigger::BattleStart,
              RelicEffectType::Heal, 2),
        relic("strawberry", "草莓", "拾取时最大生命增加 7。",
              RelicRarity::Common, RelicTrigger::OnPickup,
              RelicEffectType::IncreaseMaxHealth, 7),
        relic("pear", "梨", "拾取时最大生命增加 10。",
              RelicRarity::Uncommon, RelicTrigger::OnPickup,
              RelicEffectType::IncreaseMaxHealth, 10),
        relic("mango", "芒果", "拾取时最大生命增加 14。",
              RelicRarity::Rare, RelicTrigger::OnPickup,
              RelicEffectType::IncreaseMaxHealth, 14),
        relic("old_coin", "古钱币", "拾取时获得 300 金币。",
              RelicRarity::Rare, RelicTrigger::OnPickup,
              RelicEffectType::GainGold, 300),
        relic("whetstone", "磨刀石", "拾取时升级牌组中的 2 张攻击牌。",
              RelicRarity::Common, RelicTrigger::OnPickup,
              RelicEffectType::UpgradeAttacks, 2),
        relic("war_paint", "战纹", "拾取时升级牌组中的 2 张技能牌。",
              RelicRarity::Common, RelicTrigger::OnPickup,
              RelicEffectType::UpgradeSkills, 2)};
}

Relic RelicDatabase::createById(const std::string& id)
{
    for (const Relic& definition : createFoundationRelics())
    {
        if (definition.id == id) return definition;
    }
    throw std::invalid_argument("Unknown relic id: " + id);
}
