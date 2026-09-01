#pragma once

#include "card/Card.hpp"
#include "combat/CombatSystem.hpp"

#include <SFML/Graphics.hpp>

#include <vector>

// 战斗界面：绘制玩家、敌人、手牌和“结束回合”按钮，
// 并把鼠标点击转换为手牌索引或结束回合操作。
// 只通过 CombatSystem 的公开接口读写，不直接修改玩家或敌人血量。
class BattleView
{
public:
    BattleView();

    void setFont(const sf::Font& font);
    void handleMouseClick(sf::Vector2f mousePosition, CombatSystem& combat);
    void draw(sf::RenderWindow& window, const CombatSystem& combat) const;

    // 结束回合按钮所在区域，便于上层做额外判断或提示。
    sf::FloatRect getEndTurnButtonBounds() const;

private:
    struct HandCardLayout
    {
        int handIndex;
        sf::FloatRect bounds;
    };

    std::vector<HandCardLayout> layoutHand(const std::vector<Card>& hand) const;
    void drawPlayerPanel(sf::RenderWindow& window, const Player& player) const;
    void drawEnemyPanel(sf::RenderWindow& window, const Enemy& enemy) const;
    void drawHand(sf::RenderWindow& window, const std::vector<Card>& hand) const;
    void drawEndTurnButton(sf::RenderWindow& window) const;

    const sf::Font* font_;
};
