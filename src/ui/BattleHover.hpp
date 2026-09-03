#pragma once

#include "card/Card.hpp"

#include <SFML/Graphics.hpp>

#include <vector>

namespace BattleHover
{
struct HandCardLayout
{
    int handIndex = -1;
    sf::FloatRect bounds;
};

std::vector<HandCardLayout> layoutHandCards(const std::vector<Card>& hand,
                                            sf::Vector2f cardSize, float windowWidth,
                                            float handY, float gap);
int pickHoveredCardIndex(sf::Vector2f mousePosition,
                         const std::vector<HandCardLayout>& layouts);
sf::Vector2f computeTooltipPosition(sf::FloatRect cardBounds, sf::Vector2f panelSize,
                                    sf::Vector2f windowSize);
float easeOutCubic(float value);
} // namespace BattleHover
