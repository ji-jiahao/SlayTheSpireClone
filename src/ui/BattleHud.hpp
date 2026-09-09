#pragma once

#include "combat/CombatSystem.hpp"
#include <SFML/Graphics.hpp>

class BattleHud
{
public:
    void reset();
    bool isOpen() const { return pile_ >= 0; }
    bool handleMouseMove(sf::Vector2f position);
    bool handleMouseClick(sf::Vector2f position, const CombatSystem& combat);
    bool handleKeyPress(sf::Keyboard::Key key, const CombatSystem& combat);
    void draw(sf::RenderWindow& window, const sf::Font& font, const CombatSystem& combat) const;
    static void drawStatuses(sf::RenderWindow& window, const sf::Font& font,
                             sf::Vector2f position, int strength, int weak, int vulnerable);
    static void drawIntent(sf::RenderWindow& window, const sf::Font& font,
                           const Enemy& enemy, int damage);
private:
    std::vector<Card> cards(const CombatSystem& combat) const;
    void changePage(int direction, const CombatSystem& combat);
    int pile_ = -1;
    int page_ = 0;
    sf::Vector2f mouse_{-1.0f, -1.0f};
};
