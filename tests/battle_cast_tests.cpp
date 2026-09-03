#include "card/CardDatabase.hpp"
#include "ui/BattleCast.hpp"

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

void testTargetResolution()
{
    require(BattleCast::requiresTargetSelection(CardDatabase::createStrike()),
            "打击牌应进入目标选择");
    require(BattleCast::requiresTargetSelection(CardDatabase::createBash()),
            "痛击牌应进入目标选择");
    require(BattleCast::resolveTargetKind(CardDatabase::createDefend()) ==
                BattleTargetKind::Self,
            "防御牌应视作自身目标");
    require(BattleCast::resolveTargetKind(CardDatabase::createBash()) ==
                BattleTargetKind::Enemy,
            "攻击牌应视作敌方目标");
}

void testEaseInOutQuad()
{
    require(std::fabs(BattleCast::easeInOutQuad(0.0f)) < 0.0001f,
            "缓动起点应为 0");
    require(std::fabs(BattleCast::easeInOutQuad(1.0f) - 1.0f) < 0.0001f,
            "缓动终点应为 1");
    require(BattleCast::easeInOutQuad(0.25f) < 0.25f,
            "缓入缓出在前半段应慢于线性插值");
}

void testLerp()
{
    const sf::Vector2f value = BattleCast::lerp({0.0f, 10.0f}, {20.0f, 30.0f}, 0.5f);
    require(std::fabs(value.x - 10.0f) < 0.0001f && std::fabs(value.y - 20.0f) < 0.0001f,
            "插值结果应处于中点");
}
} // namespace

int main()
{
    try
    {
        testTargetResolution();
        testEaseInOutQuad();
        testLerp();
        std::cout << "战斗目标选择测试通过。\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "战斗目标选择测试失败: " << error.what() << '\n';
        return 1;
    }
}
