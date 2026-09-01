#pragma once

#include "combat/CombatSystem.hpp"
#include "core/GameState.hpp"
#include "relic/RelicSystem.hpp"
#include "ui/BattleView.hpp"

#include <SFML/Graphics.hpp>

#include <string>

class Game
{
public:
    Game();
    void run();

private:
    void startBattle();
    void handleBattleResult();
    void drawResultOverlay();
    bool loadFont();

    sf::RenderWindow window;
    sf::Font font;
    bool fontLoaded;
    BattleView battleView;
    CombatSystem combat;
    GameState state;
    RelicSystem relicSystem;
    BattleResult handledResult;
    int relicHealing;
    std::string statusMessage;
};
