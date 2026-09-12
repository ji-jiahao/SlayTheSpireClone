#pragma once

#include <SFML/Graphics.hpp>

class TowerBottomView
{
public:
    bool loadResources();
    void reset();
    void update(float deltaSeconds);
    bool isReady() const;
    int choiceAt(sf::Vector2f position) const;
    void draw(sf::RenderWindow& window, const sf::Font& font) const;
    static sf::FloatRect optionBounds(int index);

private:
    sf::Texture background_;
    sf::Texture character_;
    float timer_ = 0.0f;
};
