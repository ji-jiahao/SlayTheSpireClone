#include "card/CardDatabase.hpp"
#include "ui/BattleHover.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace
{
void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

void testLayoutHandCards()
{
    const std::vector<Card> hand = {
        CardDatabase::createStrike(),
        CardDatabase::createDefend(),
        CardDatabase::createBash(),
    };

    const std::vector<BattleHover::HandCardLayout> layouts =
        BattleHover::layoutHandCards(hand, {160.0f, 220.0f}, 1280.0f, 470.0f, 20.0f);

    require(layouts.size() == 3, "手牌布局数量应与手牌数量一致");
    require(std::fabs(layouts[0].bounds.position.x - 380.0f) < 0.01f,
            "手牌布局起点应居中");
    require(layouts[2].handIndex == 2, "手牌布局索引应保持顺序");
}

void testPickHoveredCardIndex()
{
    const std::vector<BattleHover::HandCardLayout> layouts = {
        {0, {{100.0f, 100.0f}, {120.0f, 160.0f}}},
        {1, {{150.0f, 110.0f}, {120.0f, 160.0f}}},
    };

    const int index = BattleHover::pickHoveredCardIndex({170.0f, 120.0f}, layouts);
    require(index == 1, "重叠卡牌应优先命中最后绘制的卡牌");
}

void testComputeTooltipPosition()
{
    const sf::FloatRect leftCard({80.0f, 500.0f}, {160.0f, 220.0f});
    const sf::FloatRect rightCard({1040.0f, 500.0f}, {160.0f, 220.0f});
    const sf::Vector2f panelSize{284.0f, 180.0f};
    const sf::Vector2f windowSize{1280.0f, 720.0f};

    const sf::Vector2f leftPanel =
        BattleHover::computeTooltipPosition(leftCard, panelSize, windowSize);
    const sf::Vector2f rightPanel =
        BattleHover::computeTooltipPosition(rightCard, panelSize, windowSize);

    require(leftPanel.x >= 16.0f && leftPanel.y >= 16.0f,
            "提示框左侧位置应留出安全边距");
    require(rightPanel.x + panelSize.x <= windowSize.x - 16.0f + 0.01f,
            "提示框右侧位置应避开屏幕边缘");
}

void testEaseOutCubic()
{
    require(std::fabs(BattleHover::easeOutCubic(0.0f)) < 0.0001f,
            "缓动起点应为 0");
    require(std::fabs(BattleHover::easeOutCubic(1.0f) - 1.0f) < 0.0001f,
            "缓动终点应为 1");
    require(BattleHover::easeOutCubic(0.5f) > 0.5f,
            "ease-out 在中点应快于线性插值");
}
} // namespace

int main()
{
    try
    {
        testLayoutHandCards();
        testPickHoveredCardIndex();
        testComputeTooltipPosition();
        testEaseOutCubic();
        std::cout << "战斗悬停测试通过。\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "战斗悬停测试失败: " << error.what() << '\n';
        return 1;
    }
}
