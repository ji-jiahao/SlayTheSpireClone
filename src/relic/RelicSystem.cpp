#include "relic/RelicSystem.hpp"

#include "card/CardDatabase.hpp"
#include "relic/RelicDatabase.hpp"

#include <algorithm>
#include <stdexcept>

void RelicSystem::beginBattle()
{
    battleVictoryApplied = false;
}

bool RelicSystem::obtainRelic(GameState& state, const std::string& relicId)
{
    if (std::find(state.relicIds.begin(), state.relicIds.end(), relicId) != state.relicIds.end())
    {
        return false;
    }

    Relic definition;
    try
    {
        definition = RelicDatabase::createById(relicId);
    }
    catch (const std::invalid_argument&)
    {
        return false;
    }

    state.relicIds.push_back(relicId);
    for (const RelicEffect& effect : definition.effects)
    {
        if (effect.trigger != RelicTrigger::OnPickup) continue;
        switch (effect.type)
        {
        case RelicEffectType::GainGold:
            state.gainGold(effect.value);
            break;
        case RelicEffectType::IncreaseMaxHealth:
            state.increaseMaxHealth(effect.value);
            break;
        case RelicEffectType::UpgradeAttacks:
            upgradeCards(state, CardType::Attack, effect.value);
            break;
        case RelicEffectType::UpgradeSkills:
            upgradeCards(state, CardType::Skill, effect.value);
            break;
        default:
            break;
        }
    }
    return true;
}

RelicBattleStartModifiers RelicSystem::applyBattleStart(GameState& state) const
{
    RelicBattleStartModifiers modifiers;
    for (const std::string& relicId : state.relicIds)
    {
        Relic definition;
        try
        {
            definition = RelicDatabase::createById(relicId);
        }
        catch (const std::invalid_argument&)
        {
            continue;
        }

        for (const RelicEffect& effect : definition.effects)
        {
            if (effect.trigger != RelicTrigger::BattleStart) continue;
            switch (effect.type)
            {
            case RelicEffectType::Heal:
                modifiers.healing += state.heal(effect.value);
                break;
            case RelicEffectType::GainBlock:
                modifiers.block += effect.value;
                break;
            case RelicEffectType::GainStrength:
                modifiers.strength += effect.value;
                break;
            case RelicEffectType::GainEnergy:
                modifiers.energy += effect.value;
                break;
            case RelicEffectType::DrawCards:
                modifiers.drawCards += effect.value;
                break;
            default:
                break;
            }
        }
    }
    return modifiers;
}

int RelicSystem::applyBattleVictory(GameState& state)
{
    if (battleVictoryApplied) return 0;
    battleVictoryApplied = true;

    int totalHealing = 0;
    for (const std::string& relicId : state.relicIds)
    {
        Relic definition;
        try
        {
            definition = RelicDatabase::createById(relicId);
        }
        catch (const std::invalid_argument&)
        {
            continue;
        }
        for (const RelicEffect& effect : definition.effects)
        {
            if (effect.trigger == RelicTrigger::BattleVictory &&
                effect.type == RelicEffectType::Heal)
            {
                totalHealing += state.heal(effect.value);
            }
        }
    }
    return totalHealing;
}

int RelicSystem::getBattleStartBlock(const GameState& state) const
{
    GameState copy = state;
    return applyBattleStart(copy).block;
}

int RelicSystem::getBattleStartStrength(const GameState& state) const
{
    GameState copy = state;
    return applyBattleStart(copy).strength;
}

int RelicSystem::upgradeCards(GameState& state, CardType type, int count) const
{
    int upgraded = 0;
    for (CardInstance& instance : state.deck)
    {
        if (upgraded >= count || instance.upgraded) continue;
        try
        {
            if (CardDatabase::createById(instance.definitionId).type == type)
            {
                instance.upgraded = true;
                ++upgraded;
            }
        }
        catch (const std::invalid_argument&)
        {
        }
    }
    return upgraded;
}
