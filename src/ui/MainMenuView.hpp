#pragma once

#include <SFML/Graphics.hpp>

class MainMenuView
{
public:
    enum class Action
    {
        None,
        Start
    };

    void setFont(const sf::Font& font);
    void setBackground(const sf::Texture& texture);
    void resetFade();
    void update(float deltaSeconds);
    void handleMouseMove(sf::Vector2f position);
    Action handleMouseClick(sf::Vector2f position) const;
    void draw(sf::RenderWindow& window) const;

private:
    sf::FloatRect startButtonBounds() const;

    const sf::Font* font_ = nullptr;
    const sf::Texture* background_ = nullptr;
    Action hoveredAction_ = Action::None;
    float fadeTimer_ = 0.0f;
};
