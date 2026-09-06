#include "card/Card.hpp"

#include <algorithm>
#include <string>

namespace
{
bool hasRepeatableUpgrade(const Card& card)
{
    const auto isRepeatable = [](const CardEffect& effect)
    {
        return effect.parameter == "repeatable_upgrade";
    };

    return std::any_of(card.effects.begin(), card.effects.end(), isRepeatable) ||
           std::any_of(card.upgradedEffects.begin(), card.upgradedEffects.end(),
                       isRepeatable);
}

void refreshDerivedValues(Card& card)
{
    card.damage = 0;
    card.block = 0;
    for (const CardEffect& effect : card.effects)
    {
        if ((effect.type == CardEffectType::Damage ||
             effect.type == CardEffectType::MultiDamage) &&
            card.damage == 0)
        {
            card.damage = effect.value;
        }
        if (effect.type == CardEffectType::Block && card.block == 0)
        {
            card.block = effect.value;
        }
    }
}
} // namespace

bool Card::upgrade()
{
    const bool repeatable = hasRepeatableUpgrade(*this);
    if (!repeatable && upgraded)
    {
        return false;
    }

    if (repeatable)
    {
        ++upgradeLevel;
        upgraded = true;
        if (upgradeLevel == 1)
        {
            name += "+";
        }

        for (CardEffect& effect : effects)
        {
            if (effect.parameter == "repeatable_upgrade" &&
                (effect.type == CardEffectType::Damage ||
                 effect.type == CardEffectType::MultiDamage))
            {
                effect.value += 4;
            }
        }

        if (id == "searing_blow" && !effects.empty())
        {
            const int damageValue = effects.front().value;
            description = "造成 " + std::to_string(damageValue) +
                          " 点伤害。这张牌可以被升级任意次数。";
        }
        else
        {
            description = upgradedDescription;
        }
        refreshDerivedValues(*this);
        return true;
    }

    if (upgradedEffects.empty())
    {
        return false;
    }

    upgraded = true;
    upgradeLevel = 1;
    name += "+";
    cost = upgradedCost;
    description = upgradedDescription;
    effects = upgradedEffects;
    refreshDerivedValues(*this);
    return true;
}
