#pragma once

#include "core/GameState.hpp"
#include "event/EventDatabase.hpp"
#include "event/EventSystem.hpp"
#include "ui/EventView.hpp"

#include <SFML/Graphics.hpp>

#include <string>

class Game
{
public:
    Game();
    void run();

private:
    enum class SceneType
    {
        Event,
        Map,
        GameOver
    };

    void handleEvents();
    void update(float deltaSeconds);
    void render();
    void startEventIfAvailable();
    void showMap();
    void showGameOver();
    void drawMapPlaceholder();
    void drawGameOver();
    sf::Text makeText(const std::string& text, unsigned int size,
                      sf::Color color) const;

    sf::RenderWindow window_;
    GameState gameState_;
    EventDatabase eventDatabase_;
    EventSystem eventSystem_;
    EventView eventView_;
    sf::Font font_;
    sf::Clock clock_;
    SceneType scene_;
    bool ready_;
    std::string lastError_;
};
