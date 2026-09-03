#include "ui/BattleHover.hpp"

#include <algorithm>

namespace BattleHover
{
namespace
{
float clamp01(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}
} // namespace

std::vector<HandCardLayout> layoutHandCards(const std::vector<Card>& hand,
                                            sf::Vector2f cardSize, float windowWidth,
                                            float handY, float gap)
{
    const std::size_t count = hand.size();
    std::vector<HandCardLayout> layouts;
    layouts.reserve(count);

    if (count == 0)
    {
        return layouts;
    }

    const float totalWidth =
        static_cast<float>(count) * cardSize.x + static_cast<float>(count - 1) * gap;
    const float startX = (windowWidth - totalWidth) / 2.0f;

    for (std::size_t index = 0; index < count; ++index)
    {
        HandCardLayout layout;
        layout.handIndex = static_cast<int>(index);
        layout.bounds = {
            {startX + static_cast<float>(index) * (cardSize.x + gap), handY},
            cardSize};
        layouts.push_back(layout);
    }

    return layouts;
}

int pickHoveredCardIndex(sf::Vector2f mousePosition,
                         const std::vector<HandCardLayout>& layouts)
{
    for (auto it = layouts.rbegin(); it != layouts.rend(); ++it)
    {
        if (it->bounds.contains(mousePosition))
        {
            return it->handIndex;
        }
    }

    return -1;
}

sf::Vector2f computeTooltipPosition(sf::FloatRect cardBounds, sf::Vector2f panelSize,
                                    sf::Vector2f windowSize)
{
    constexpr float kMargin = 16.0f;
    constexpr float kGap = 14.0f;

    sf::Vector2f position;
    const float cardCenterX = cardBounds.position.x + cardBounds.size.x / 2.0f;
    const float cardCenterY = cardBounds.position.y + cardBounds.size.y / 2.0f;

    if (cardCenterX < windowSize.x / 2.0f)
    {
        position.x = cardBounds.position.x + cardBounds.size.x + kGap;
    }
    else
    {
        position.x = cardBounds.position.x - panelSize.x - kGap;
    }

    if (cardCenterY < windowSize.y / 2.0f)
    {
        position.y = cardBounds.position.y + cardBounds.size.y + kGap;
    }
    else
    {
        position.y = cardBounds.position.y - panelSize.y - kGap;
    }

    position.x = std::clamp(position.x, kMargin, windowSize.x - panelSize.x - kMargin);
    position.y = std::clamp(position.y, kMargin, windowSize.y - panelSize.y - kMargin);
    return position;
}

float easeOutCubic(float value)
{
    const float t = clamp01(value);
    const float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
}
} // namespace BattleHover
