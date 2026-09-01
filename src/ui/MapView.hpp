#pragma once

#include "core/GameState.hpp"
#include "map/MapState.hpp"

#include <SFML/Graphics.hpp>

class MapView
{
public:
    void setFont(const sf::Font& font);
    void resetScroll();
    void beginPointer(sf::Vector2f position);
    void updatePointer(sf::Vector2f position);
    int endPointer(sf::Vector2f position, const MapState& map);
    void scroll(float wheelDelta);
    void draw(sf::RenderWindow& window, const MapState& map, const GameState& state) const;

private:
    sf::Vector2f nodePosition(const MapNode& node) const;
    void clampScroll();
    const sf::Font* font_ = nullptr;
    float scrollOffset_ = 0.0f;
    bool pointerDown_ = false;
    bool pointerDragged_ = false;
    sf::Vector2f pointerStart_;
    sf::Vector2f pointerLast_;
};
