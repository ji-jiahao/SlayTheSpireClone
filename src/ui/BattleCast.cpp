#include "ui/BattleCast.hpp"

#include <algorithm>
#include <cmath>

namespace
{
bool hasEnemyTarget(const Card& card)
{
    for (const CardEffect& effect : card.effects)
    {
        if (effect.target == CardTarget::Enemy || effect.target == CardTarget::AllEnemies ||
            effect.target == CardTarget::RandomEnemy)
        {
            return true;
        }
    }

    return false;
}
} // namespace

namespace BattleCast
{
BattleTargetKind resolveTargetKind(const Card& card)
{
    return hasEnemyTarget(card) ? BattleTargetKind::Enemy : BattleTargetKind::Self;
}

bool requiresTargetSelection(const Card& card)
{
    return resolveTargetKind(card) == BattleTargetKind::Enemy;
}

float easeInOutQuad(float value)
{
    const float t = std::clamp(value, 0.0f, 1.0f);
    if (t < 0.5f)
    {
        return 2.0f * t * t;
    }

    return 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

sf::Vector2f lerp(sf::Vector2f start, sf::Vector2f end, float ratio)
{
    const float t = std::clamp(ratio, 0.0f, 1.0f);
    return {start.x + (end.x - start.x) * t, start.y + (end.y - start.y) * t};
}
} // namespace BattleCast
