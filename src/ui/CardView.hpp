#pragma once

#include "card/Card.hpp"

#include <SFML/Graphics.hpp>

// 单张卡牌的绘制视图：只负责显示，不修改卡牌数据。
class CardView
{
public:
    CardView();

    // 绘制文字需要字体；未设置字体时只绘制卡牌底色与边框。
    void setFont(const sf::Font& font);
    void setPosition(sf::Vector2f position);
    sf::FloatRect getBounds() const;
    void draw(sf::RenderWindow& window, const Card& card) const;

    // 卡牌固定尺寸，供战斗界面排版复用。
    static sf::Vector2f getCardSize();

private:
    sf::Color colorForType(CardType type) const;

    sf::Vector2f position_;
    const sf::Font* font_;
};
