#pragma once

#include "core/GameState.hpp"

#include <SFML/Graphics.hpp>

#include <string>
#include <vector>

struct RoomAction
{
    std::string label;
    std::string detail;
    bool enabled = true;
};

class RoomView
{
public:
    void setFont(const sf::Font& font);
    void setBackground(const sf::Texture& texture);
    int handleMouseClick(sf::Vector2f position, const std::vector<RoomAction>& actions) const;
    void draw(sf::RenderWindow& window, const GameState& state,
              const std::string& eyebrow, const std::string& title,
              const std::string& description,
              const std::vector<RoomAction>& actions) const;

private:
    sf::FloatRect actionBounds(std::size_t index, std::size_t count) const;

    const sf::Font* font_ = nullptr;
    const sf::Texture* background_ = nullptr;
};
