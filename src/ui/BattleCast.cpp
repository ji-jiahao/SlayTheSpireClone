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
    if (card.type == CardType::Power) return BattleTargetKind::Self;
    return hasEnemyTarget(card) ? BattleTargetKind::Enemy : BattleTargetKind::Self;
}

bool requiresTargetSelection(const Card& card)
{
    if (card.type == CardType::Power) return false;
    return std::any_of(card.effects.begin(), card.effects.end(), [](const CardEffect& effect)
    {
        return effect.target == CardTarget::Enemy;
    });
}

EffectKind resolveEffectKind(const Card& card)
{
    if (card.type == CardType::Power) return EffectKind::Power;
    if (card.type == CardType::Attack) return EffectKind::Slash;
    if (hasEnemyTarget(card)) return EffectKind::Debuff;
    if (std::any_of(card.effects.begin(), card.effects.end(), [](const CardEffect& effect)
        { return effect.type == CardEffectType::Block; })) return EffectKind::Guard;
    return EffectKind::Skill;
}

Pose samplePose(float seconds, sf::Vector2f startCenter, float startScale, ExitKind exit)
{
    const float time = std::clamp(seconds, 0.0f, kFinishSeconds);
    const sf::Vector2f castCenter{640.0f, 330.0f};
    Pose pose;
    if (time <= kCommitSeconds)
    {
        const float t = time / kCommitSeconds;
        const float eased = 1.0f - std::pow(1.0f - t, 3.0f);
        pose.center = lerp(startCenter, castCenter, eased);
        pose.scale = startScale + (0.86f - startScale) * eased;
        return pose;
    }
    pose.center = castCenter;
    pose.scale = 0.86f;
    if (time <= kReleaseSeconds) return pose;

    const float t = (time - kReleaseSeconds) / (kFinishSeconds - kReleaseSeconds);
    const float eased = easeInOutQuad(t);
    if (exit == ExitKind::Discard)
    {
        // 弃牌堆在当前 HUD 的左下角，不能沿用原版右下角坐标。
        pose.center = lerp(castCenter, {106.0f, 659.0f}, eased);
        pose.center.y -= std::sin(t * 3.14159265f) * 48.0f;
        pose.scale *= 1.0f - 0.9f * eased;
        pose.rotation = -28.0f * eased;
        pose.opacity = 1.0f - t * t;
    }
    else
    {
        pose.center.y -= (exit == ExitKind::Power ? 105.0f : 35.0f) * eased;
        pose.scale *= exit == ExitKind::Power ? 1.0f + 0.16f * eased : 1.0f - 0.35f * eased;
        pose.opacity = 1.0f - eased;
    }
    return pose;
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
