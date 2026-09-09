#include "card/CardDatabase.hpp"
#include "ui/BattleCast.hpp"
#include "ui/BattleHud.hpp"

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

void testHeadbuttPicker()
{
    CombatSystem combat;
    // More than one page of discarded cards, including duplicate names.
    Card block = CardDatabase::createDefend(); block.cost = 0;
    std::vector<Card> deck(14,block);
    deck.push_back(CardDatabase::createById("headbutt"));
    combat.startBattle(80,7,deck,{"测试",200,0,"generic"},0,0,0,10);
    for (int i=static_cast<int>(combat.getHandCards().size())-1;i>=0;--i)
        if(combat.getHandCards()[i].id=="defend") require(combat.playCard(i),"准备弃牌失败");
    require(combat.playCard(0),"头槌未能打出");
    require(combat.getDiscardChoiceCards().size()==14,"必须显示所有可选实例");
    BattleHud hud;
    require(hud.handleKeyPress(sf::Keyboard::Key::Escape,combat),"Esc必须被选牌框消费");
    require(combat.hasPendingDiscardChoice(),"Esc不能绕过选牌");
    require(hud.handleMouseClick({1200,700},combat),"点击背景不能穿透选牌框");
    require(combat.hasPendingDiscardChoice(),"背景点击不能完成选牌");
    hud.handleMouseClick({747,638},combat);
    // Fourth card on page 2 corresponds to the 14th discarded instance.
    hud.handleMouseClick({640,180},combat);
    require(!combat.hasPendingDiscardChoice(),"第二页选择未完成");
    require(combat.getDeck().getDrawPile().back().id=="defend","选中牌没有放到牌堆顶");
}
} // namespace

int main()
{
    try
    {
        testTargetResolution();
        testEaseInOutQuad();
        testLerp();
        testHeadbuttPicker();
        std::cout << "战斗目标选择测试通过。\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "战斗目标选择测试失败: " << error.what() << '\n';
        return 1;
    }
}
