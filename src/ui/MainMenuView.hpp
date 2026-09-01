#pragma once

#include <SFML/Graphics.hpp>

class MainMenuView
{
public:
    enum class Action { None, Start, Quit };

    void setFont(const sf::Font& font);
    void setBackground(const sf::Texture& texture);
    Action handleMouseClick(sf::Vector2f position) const;
    void draw(sf::RenderWindow& window) const;

private:
    const sf::Font* font_ = nullptr;
    const sf::Texture* background_ = nullptr;
};
