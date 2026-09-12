#include "room/TowerBottomSystem.hpp"
#include "card/CardDatabase.hpp"
#include "combat/CombatSystem.hpp"
#include "ui/TowerBottomView.hpp"

#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <unordered_set>

namespace
{
void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}
}

int main(int argc, char** argv)
{
    GameState state;
    require(!TowerBottomSystem::chooseBlessing(state, -1, 1), "非法选项不可生效");
    require(!TowerBottomSystem::chooseBlessing(state, 3, 1), "越界选项不可生效");
    require(TowerBottomSystem::chooseBlessing(state, 0, 1), "回血祝福选择失败");
    require(!TowerBottomSystem::chooseBlessing(state, 2, 1), "只能选择一份祝福");
    state.currentHealth = 40;
    require(TowerBottomSystem::completeRoom(state) == 0, "塔底不算通关");
    state.currentNodeId = 1;
    require(TowerBottomSystem::completeRoom(state) == 8, "通关恢复8点生命");
    require(TowerBottomSystem::completeRoom(state) == 0, "同节点不可重复回血");
    state.currentNodeId = 2;
    state.currentHealth = 78;
    require(TowerBottomSystem::completeRoom(state) == 2 && state.currentHealth == 80,
            "回血不得超过上限");
    state.currentNodeId = 3;
    state.currentHealth = 0;
    require(TowerBottomSystem::completeRoom(state) == 0 && state.currentHealth == 0,
            "死亡不可自动复活");
    state.reset();
    require(!state.towerBlessingChosen && state.roomClearHealing == 0 &&
            state.battleStartDexterity == 0 && state.lastCompletedNodeId == -1,
            "新局必须清空祝福");
    std::unordered_set<std::string> results;
    for (unsigned int seed = 0; seed < 50; ++seed)
    {
        state.reset();
        const auto size = state.deck.size();
        require(TowerBottomSystem::chooseBlessing(state, 1, seed), "随机卡选择失败");
        require(state.deck.size() == size + 1, "随机卡必须进入牌组");
        const Card card = CardDatabase::createFromInstance(state.deck.back());
        require(card.rarity != CardRarity::Starter && card.rarity != CardRarity::Status &&
                card.rarity != CardRarity::Curse, "随机卡必须属于奖励卡池");
        results.insert(card.id);
    }
    require(results.size() > 1, "不同随机种子应有不同奖励");
    state.reset();
    require(TowerBottomSystem::chooseBlessing(state, 2, 1), "敏捷祝福选择失败");
    CombatSystem combat;
    for (int battle = 0; battle < 2; ++battle)
    {
        combat.startBattle(80, 1, CardDatabase::createStarterDeck(), EncounterDefinition{},
                           0, 0, 0, 0, 80, 0, state.battleStartDexterity);
        require(combat.getPlayer().getDexterity() == 2, "每场战斗恰好获得2敏捷");
        combat.endPlayerTurn();
        require(combat.getPlayer().getDexterity() == 2, "敏捷不可逐回合叠加或消失");
    }
    TowerBottomView view;
    require(view.loadResources(), "塔底图片加载失败");
    view.reset();
    require(view.choiceAt({800, 350}) == -1, "过渡中禁止选择");
    view.update(-1);
    view.update(std::numeric_limits<float>::quiet_NaN());
    require(!view.isReady(), "非法时间不得跳过过渡");
    sf::Font font;
    require(font.openFromFile("assets/fonts/simhei.ttf"), "字体加载失败");
    sf::RenderWindow window(sf::VideoMode({1280, 720}), "塔底自动验证", sf::Style::None);
    window.setVisible(false);
    auto capture = [&](const char* name)
    {
        window.clear();
        view.draw(window, font);
        if (argc > 1)
        {
            const std::filesystem::path directory(argv[1]);
            std::filesystem::create_directories(directory);
            sf::Texture frame(sf::Vector2u{1280, 720});
            frame.update(window);
            require(frame.copyToImage().saveToFile(directory / name), "截图保存失败");
        }
    };
    view.update(0.6f);
    capture("tower-title.png");
    view.update(3.0f);
    require(view.isReady(), "渐变结束必须开放选项");
    require(view.choiceAt({0, 0}) == -1, "空白区域不得选择");
    for (int choice = 0; choice < 3; ++choice)
    {
        const auto bounds = TowerBottomView::optionBounds(choice);
        require(view.choiceAt(bounds.position + bounds.size / 2.0f) == choice,
                "按钮命中区域不匹配");
        require(bounds.position.y + bounds.size.y <= 720, "按钮不可越界");
    }
    capture("tower-choice.png");
    view.reset();
    require(!view.isReady(), "重新开始必须重播渐变");
    std::cout << "塔底祝福与视图验证通过。\n";
}
