#pragma once

#include "card/Card.hpp"

#include <SFML/Graphics.hpp>

enum class BattleTargetKind
{
    Self,
    Enemy
};

namespace BattleCast
{
enum class ExitKind { Discard, Exhaust, Power };
enum class EffectKind { Slash, Guard, Power, Debuff, Skill };

// 起手结束时提交战斗结算，退场期间允许继续操作下一张牌。
constexpr float kCommitSeconds = 0.10f;
constexpr float kReleaseSeconds = 0.18f;
constexpr float kFinishSeconds = 0.46f;

struct Pose
{
    sf::Vector2f center;
    float scale = 1.0f;
    float rotation = 0.0f;
    float opacity = 1.0f;
};

BattleTargetKind resolveTargetKind(const Card& card);
bool requiresTargetSelection(const Card& card);
EffectKind resolveEffectKind(const Card& card);
Pose samplePose(float seconds, sf::Vector2f startCenter, float startScale, ExitKind exit);
float easeInOutQuad(float value);
sf::Vector2f lerp(sf::Vector2f start, sf::Vector2f end, float ratio);
}
