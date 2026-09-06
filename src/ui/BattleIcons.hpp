#pragma once

#include <SFML/Graphics.hpp>

namespace BattleIcons
{
enum class Kind { Draw, Discard, Exhaust, Strength, Weak, Vulnerable, Attack, Guard, Buff, Unknown };
// All artwork uses a 48 x 48 coordinate system so HUD slots can share it.
void draw(sf::RenderTarget& target, Kind kind, sf::Vector2f position, float size);
}
