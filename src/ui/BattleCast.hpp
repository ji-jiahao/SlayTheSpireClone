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
BattleTargetKind resolveTargetKind(const Card& card);
bool requiresTargetSelection(const Card& card);
float easeInOutQuad(float value);
sf::Vector2f lerp(sf::Vector2f start, sf::Vector2f end, float ratio);
}
