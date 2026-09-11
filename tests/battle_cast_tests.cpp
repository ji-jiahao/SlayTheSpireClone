#include "card/CardDatabase.hpp"
#include "ui/BattleCast.hpp"
#include "ui/BattleHud.hpp"
#include "ui/BattleView.hpp"
#include "ui/CardView.hpp"
#include "ui/CardPresentation.hpp"

#include <cmath>
#include <iostream>
#include <filesystem>
#include <limits>
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
    for (const char* id : {"cleave", "whirlwind", "sword_boomerang", "juggernaut", "combust"})
        require(!BattleCast::requiresTargetSelection(CardDatabase::createById(id)),
                "群体、随机目标与持续能力不应进入单体目标选择");
    require(BattleCast::resolveTargetKind(CardDatabase::createById("combust")) == BattleTargetKind::Self,
            "持续能力应在玩家身上激活");
}

void testCastTimeline()
{
    const sf::Vector2f start{350.0f, 560.0f};
    for (auto exit : {BattleCast::ExitKind::Discard, BattleCast::ExitKind::Exhaust, BattleCast::ExitKind::Power})
    {
        const auto first = BattleCast::samplePose(-1.0f, start, 1.15f, exit);
        require(first.center == start && first.scale == 1.15f, "起手必须保持原悬停中心与缩放");
        const auto impact = BattleCast::samplePose(BattleCast::kCommitSeconds, start, 1.15f, exit);
        const auto release = BattleCast::samplePose(BattleCast::kReleaseSeconds, start, 1.15f, exit);
        require(impact.center == sf::Vector2f{640.0f, 330.0f} && impact.center == release.center,
                "命中和短暂停留必须位于统一施放位置");
        const auto end = BattleCast::samplePose(100.0f, start, 1.15f, exit);
        require(end.opacity == 0.0f && end.scale > 0.0f, "越过退场终点必须完全透明且缩放有效");
        if (exit == BattleCast::ExitKind::Discard)
            require(std::fabs(end.center.x - 106.0f) < 0.01f && std::fabs(end.center.y - 659.0f) < 0.01f,
                    "普通牌应退到本项目实际的弃牌堆位置");
        else require(end.center.y < impact.center.y, "消耗和能力应向上消散");
        for (int frame = 1; frame <= 460; ++frame)
        {
            const auto previous = BattleCast::samplePose((frame - 1) * 0.001f, start, 1.15f, exit);
            const auto current = BattleCast::samplePose(frame * 0.001f, start, 1.15f, exit);
            require(std::hypot(current.center.x - previous.center.x, current.center.y - previous.center.y) < 12.0f,
                    "分段轨迹接缝不能发生位置跳跃");
            require(current.opacity >= 0.0f && current.opacity <= 1.0f, "透明度必须处于有效范围");
        }
    }
}

sf::Vector2f handPoint(const CombatSystem& combat, const std::string& id)
{
    const auto& hand = combat.getHandCards();
    const float step = hand.size() > 5 ? 730.0f / static_cast<float>(hand.size() - 1) : 180.0f;
    const float start = hand.size() > 5 ? 190.0f : (1280.0f - (hand.size() * 180.0f - 20.0f)) / 2.0f;
    for (std::size_t i = 0; i < hand.size(); ++i)
        if (hand[i].id == id) return {start + i * step + 8.0f, 530.0f};
    throw std::runtime_error("测试手牌中缺少指定卡牌：" + id);
}

void selectCard(BattleView& view, CombatSystem& combat, const std::string& id)
{
    const auto point = handPoint(combat, id);
    view.handleMouseMove(point, combat);
    view.handleMouseClick(point, combat);
    if (BattleCast::requiresTargetSelection(CardDatabase::createById(id)))
        view.handleMouseClick({1025.0f, 345.0f}, combat);
}

void testBattleView(BattleView& view)
{
    CombatSystem combat;
    auto start = [&](std::vector<Card> cards, int enemyHealth = 100)
    {
        view.reset();
        combat.startBattle(80, 7, std::move(cards), {"测试敌人", enemyHealth, 0, "generic"}, 0, 0, 10, 10);
        view.update(0.0f, combat);
    };
    start({CardDatabase::createStrike(), CardDatabase::createDefend()});
    const int energy = combat.getPlayer().getCurrentEnergy();
    selectCard(view, combat, "strike");
    require(combat.getEnemy().getCurrentHealth() == 100 && combat.getPlayer().getCurrentEnergy() == energy,
            "起手前不得提前扣血或扣费");
    view.update(-1.0f, combat);
    view.update(std::numeric_limits<float>::quiet_NaN(), combat);
    view.update(0.05f, combat);
    view.handleMouseClick({1160.0f, 650.0f}, combat);
    require(combat.getEnemy().getCurrentHealth() == 100 && combat.getHandCards().size() == 2,
            "起手期间结束回合和无效时间步不得改变战斗");
    view.update(0.051f, combat);
    require(combat.getEnemy().getCurrentHealth() == 94 && combat.getPlayer().getCurrentEnergy() == energy - 1,
            "跨越命中点必须恰好扣血扣费一次");
    require(view.isVisualLocked(), "命中后仍需保留退场与反馈");
    selectCard(view, combat, "defend");
    view.update(0.11f, combat);
    require(combat.getPlayer().getBlock() == 5 && combat.getHandCards().empty(),
            "上一张牌退场时应允许施放下一张牌");
    view.update(1.0f, combat);
    require(!view.isVisualLocked() && combat.getDeck().getDiscardPile().size() == 2,
            "退场后解除锁定且不重复结算");

    start({CardDatabase::createStrike()}, 6);
    selectCard(view, combat, "strike");
    view.update(10.0f, combat);
    require(combat.getResult() == BattleResult::Victory && view.isVisualLocked(),
            "大时间步下致死打击仍应显示最后一次命中");
    view.update(1.0f, combat);
    require(!view.isVisualLocked(), "致死反馈结束后应解除结局锁定");

    start({CardDatabase::createById("clash"), CardDatabase::createDefend()});
    selectCard(view, combat, "clash");
    view.update(0.11f, combat);
    require(!view.isVisualLocked() && combat.getEnemy().getCurrentHealth() == 100 &&
                combat.getHandCards().size() == 2, "交锋不满足条件时必须恢复手牌且不留下动画锁");

    start({CardDatabase::createById("juggernaut")});
    selectCard(view, combat, "juggernaut");
    view.update(0.11f, combat);
    require(combat.getHandCards().empty() && combat.getDeck().getExhaustPile().size() == 1,
            "带敌方触发效果的能力应直接施放并进入消耗牌堆");

    start({CardDatabase::createById("corruption"), CardDatabase::createDefend()});
    require(!combat.willExhaustCard(CardDatabase::createDefend()), "普通防御应进入弃牌堆");
    selectCard(view, combat, "corruption");
    view.update(0.11f, combat);
    require(combat.willExhaustCard(CardDatabase::createDefend()), "腐化后技能去向必须使用战斗规则");
    selectCard(view, combat, "defend");
    view.update(0.11f, combat);
    require(combat.getDeck().getExhaustPile().size() == 2 && combat.getDeck().getDiscardPile().empty(),
            "腐化技能退场不得被误判为弃牌");

    start({CardDatabase::createStrike()});
    view.handleMouseClick(handPoint(combat, "strike"), combat);
    view.update(6.0f, combat);
    require(view.isVisualLocked(), "思考超过五秒不能自动取消目标选择");
    require(view.handleKeyPress(sf::Keyboard::Key::Escape, combat) && !view.isVisualLocked(),
            "取消目标选择应立即恢复操作且不结算");

    std::vector<Card> overlap(7, CardDatabase::createDefend());
    start(overlap);
    // x=320 同时处于第一、第二张牌，打出后必须仍只有一次费用与格挡。
    const int overlapEnergy = combat.getPlayer().getCurrentEnergy();
    view.handleMouseClick({320.0f, 530.0f}, combat);
    view.update(0.11f, combat);
    require(combat.getHandCards().size() == 6 && combat.getPlayer().getBlock() == 5 &&
                combat.getPlayer().getCurrentEnergy() == overlapEnergy - 1, "重叠手牌只能打出一张");
    view.reset();
    view.update(1.0f, combat);
    require(!view.isVisualLocked(), "重置必须清除全部动画");
}

void captureAnimations(BattleView& view, const std::filesystem::path& directory)
{
    std::filesystem::create_directories(directory);
    sf::Texture background;
    require(background.loadFromFile("assets/images/background/battle_background.png"), "无法加载战斗验证背景");
    view.setBackground(background);
    sf::RenderWindow window(sf::VideoMode({1280, 720}), "战斗动画本地验证", sf::Style::None);
    window.setVisible(false);
    sf::Texture frame;
    require(frame.resize({1280, 720}), "无法创建截图纹理");
    for (const char* id : {"strike", "defend", "inflame", "offering", "shockwave"})
    {
        CombatSystem combat;
        view.reset();
        combat.startBattle(80, 7, {CardDatabase::createById(id)}, {"测试敌人", 100, 0, "cultist"}, 0, 0);
        view.update(0.0f, combat);
        selectCard(view, combat, id);
        for (int i = 0; i < 4; ++i)
        {
            view.update(i == 0 ? 0.05f : 0.10f, combat);
            window.clear();
            view.draw(window, combat);
            // 在交换双缓冲之前读取本帧，避免截到上一帧或首帧空画面。
            frame.update(window);
            window.display();
            require(frame.copyToImage().saveToFile(directory / (std::string(id) + "_" + std::to_string(i) + ".png")),
                    "无法保存动画验证截图");
        }
    }
}

std::vector<Card> negativeCards()
{
    // 通过实际战斗效果生成负面牌，避免在测试中复制其规则和文字。
    Card generator;
    generator.id = "negative_art_test";
    generator.effects.push_back({CardEffectType::Exhaust, 1});
    for (const char* id : {"wound", "dazed", "burn", "slimed", "dark_erosion"})
        generator.effects.push_back({CardEffectType::AddCard, 1, CardTarget::Self, id});
    CombatSystem combat;
    combat.startBattle(80, 1, {generator});
    require(combat.playCard(0), "无法生成负面牌测试数据");
    return combat.getDeck().getDiscardPile();
}

void testNegativeCardArt(const sf::Font& font, const std::filesystem::path& captureDirectory)
{
    const auto cards = negativeCards();
    require(cards.size() == 5, "应覆盖全部五种实际负面牌");
    CombatSystem combat;
    combat.startBattle(80, 7, {CardDatabase::createById("power_through")});
    require(combat.playCard(0) && combat.getHandCards().size() == 2, "硬撑应生成两张手牌");
    for (const auto& card : combat.getHandCards())
        require(card.id == "wound", "伤口入手牌的效果参数不能错误生成黏液");
    combat.startBattle(80, 7, {CardDatabase::createById("reckless_charge")});
    require(combat.playCard(0) && combat.getDeck().getDrawPile().size() == 1,
            "无谋冲锋应向抽牌堆加入负面牌");
    require(combat.getDeck().getDrawPile().back().id == "dazed", "眩晕入抽牌堆时必须保留正确牌面 ID");
    sf::RenderTexture gallery({1000, 300});
    gallery.clear(sf::Color(28, 25, 29));
    for (std::size_t index = 0; index < cards.size(); ++index)
    {
        const auto& card = cards[index];
        require(card.rarity == CardRarity::Status && CardPresentation::typeLabel(card) == "状态",
                "负面牌卡面和悬停类型不能误显示为技能或攻击");
        sf::Image source;
        require(source.loadFromFile("assets/images/cards/pixel_v2/" + card.id + ".png"),
                "负面牌缺少可解码的牌面：" + card.id);
        require(source.getSize().x >= 160 && source.getSize().y >= 220, "牌面分辨率过低");
        require(card.cost == (card.id == "slimed" ? 1 : -2), "负面牌费用与实际规则不符");
        CardView cardView;
        cardView.setFont(font);
        cardView.setPosition({20.0f + index * 196.0f, 40.0f});
        cardView.draw(gallery, card);
    }
    gallery.display();
    const sf::Image rendered = gallery.getTexture().copyToImage();
    for (std::size_t index = 0; index < cards.size(); ++index)
    {
        sf::Image source;
        require(source.loadFromFile("assets/images/cards/pixel_v2/" + cards[index].id + ".png"),
                "无法读取参考牌面");
        // 中部插画远离动态文字覆盖区域，逐像素比对可证明实际使用专属纹理而非几何后备。
        for (unsigned y : {72u, 100u, 125u})
        {
            const unsigned sourceX = static_cast<unsigned>((80.5f / 160.0f) * source.getSize().x);
            const unsigned sourceY = static_cast<unsigned>(((y + 0.5f) / 220.0f) * source.getSize().y);
            const sf::Color actual = rendered.getPixel({static_cast<unsigned>(20 + index * 196 + 80), 40 + y});
            bool matches = false;
            // 非整数缩放的纹理采样允许一像素舍入差异，仍必须匹配原插画像素。
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                    matches = matches || actual == source.getPixel(
                        {static_cast<unsigned>(static_cast<int>(sourceX) + dx),
                         static_cast<unsigned>(static_cast<int>(sourceY) + dy)});
            require(matches, "负面牌插画未按 ID 正确渲染：" + cards[index].id);
        }
    }
    if (!captureDirectory.empty())
    {
        std::filesystem::create_directories(captureDirectory);
        require(rendered.saveToFile(captureDirectory / "negative_cards.png"), "无法保存负面牌验证图");
    }
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
    // 超过一页的弃牌包含同名实例，验证翻页与实例选择。
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
    // 第二页第四张对应第十四张弃牌实例。
    hud.handleMouseClick({640,180},combat);
    require(!combat.hasPendingDiscardChoice(),"第二页选择未完成");
    require(combat.getDeck().getDrawPile().back().id=="defend","选中牌没有放到牌堆顶");
}
} // namespace

int main(int argc, char** argv)
{
    try
    {
        testTargetResolution();
        testCastTimeline();
        testEaseInOutQuad();
        testLerp();
        testHeadbuttPicker();
        sf::Font font;
        require(font.openFromFile("assets/fonts/simhei.ttf"), "无法加载测试字体");
        BattleView view;
        view.setFont(font);
        testBattleView(view);
        testNegativeCardArt(font, argc > 1 ? argv[1] : "");
        if (argc > 1) captureAnimations(view, argv[1]);
        std::cout << "战斗目标选择测试通过。\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "战斗目标选择测试失败: " << error.what() << '\n';
        return 1;
    }
}
