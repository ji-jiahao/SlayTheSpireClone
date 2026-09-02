#pragma once

#include "core/GameState.hpp"

#include <SFML/Graphics.hpp>

#include <string>

class RestView
{
public:
    enum class Action
    {
        None,
        Rest,
        Leave
    };

    void setBackground(const sf::Texture* texture);
    void setFont(const sf::Font& font);
    void handleMouseMove(sf::Vector2f position);
    Action handleMouseClick(sf::Vector2f position) const;
    void draw(sf::RenderWindow& window, const GameState& state,
              int healAmount, bool rested, const std::string& message) const;

private:
    sf::FloatRect restButtonBounds() const;
    sf::FloatRect leaveButtonBounds() const;

    const sf::Texture* background_ = nullptr;
    const sf::Font* font_ = nullptr;
    Action hoveredAction_ = Action::None;
};
