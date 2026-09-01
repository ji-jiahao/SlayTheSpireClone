#include "app/Game.hpp"

#include <array>
#include <filesystem>

namespace
{
constexpr unsigned int kWindowWidth = 1280;
constexpr unsigned int kWindowHeight = 720;

sf::String toSfString(const std::string& text)
{
    return sf::String::fromUtf8(text.begin(), text.end());
}
}

Game::Game()
    : window(sf::VideoMode({kWindowWidth, kWindowHeight}), "Slay the Spire Clone"),
      fontLoaded(false), handledResult(BattleResult::Active), relicHealing(0)
{
    fontLoaded = loadFont();
    window.setFramerateLimit(60);
    if (fontLoaded)
    {
        battleView.setFont(font);
    }
    startBattle();
}

void Game::run()
{
    while (window.isOpen())
    {
        while (const auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
            else if (const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>())
            {
                if (mouse->button == sf::Mouse::Button::Left &&
                    combat.getResult() == BattleResult::Active)
                {
                    battleView.handleMouseClick(window.mapPixelToCoords(mouse->position), combat);
                    handleBattleResult();
                }
            }
            else if (const auto* key = event->getIf<sf::Event::KeyPressed>())
            {
                if (key->code == sf::Keyboard::Key::R &&
                    combat.getResult() != BattleResult::Active)
                {
                    if (combat.getResult() == BattleResult::Defeat)
                    {
                        state.currentHealth = state.maxHealth;
                    }
                    startBattle();
                }
            }
        }

        combat.update();
        handleBattleResult();
        window.clear(sf::Color(35, 38, 42));
        battleView.draw(window, combat);
        drawResultOverlay();
        window.display();
    }
}

void Game::startBattle()
{
    relicSystem.beginBattle();
    handledResult = BattleResult::Active;
    relicHealing = 0;
    statusMessage.clear();
    combat.startBattle(state.currentHealth);
}

void Game::handleBattleResult()
{
    const BattleResult result = combat.getResult();
    if (result == BattleResult::Active || result == handledResult)
    {
        return;
    }

    handledResult = result;
    if (result == BattleResult::Victory)
    {
        state.currentHealth = combat.getPlayer().getCurrentHealth();
        relicHealing = relicSystem.applyBattleVictory(state);
        statusMessage = "战斗胜利";
    }
    else
    {
        statusMessage = "战斗失败";
    }
}

void Game::drawResultOverlay()
{
    if (combat.getResult() == BattleResult::Active)
    {
        return;
    }

    sf::RectangleShape overlay({560.0f, 210.0f});
    overlay.setPosition({360.0f, 230.0f});
    overlay.setFillColor(sf::Color(22, 24, 28, 235));
    overlay.setOutlineColor(sf::Color(230, 174, 72));
    overlay.setOutlineThickness(3.0f);
    window.draw(overlay);

    if (!fontLoaded)
    {
        return;
    }

    sf::Text title(font, toSfString(statusMessage), 40);
    title.setFillColor(sf::Color(245, 220, 150));
    title.setPosition({510.0f, 260.0f});
    window.draw(title);

    std::string detail;
    if (combat.getResult() == BattleResult::Victory)
    {
        detail = "燃烧之血回复 " + std::to_string(relicHealing) + " 点生命  当前生命 " +
                 std::to_string(state.currentHealth) + "/" + std::to_string(state.maxHealth);
    }
    else
    {
        detail = "本次挑战结束";
    }

    sf::Text detailText(font, toSfString(detail), 22);
    detailText.setFillColor(sf::Color(235, 229, 207));
    detailText.setPosition({430.0f, 335.0f});
    window.draw(detailText);

    sf::Text restart(font, toSfString("按 R 重新战斗"), 22);
    restart.setFillColor(sf::Color(210, 210, 210));
    restart.setPosition({545.0f, 390.0f});
    window.draw(restart);
}

bool Game::loadFont()
{
    const std::array<std::filesystem::path, 3> candidates = {
        std::filesystem::path("assets/fonts/simhei.ttf"),
        std::filesystem::path("Debug/assets/fonts/simhei.ttf"),
        std::filesystem::path("../assets/fonts/simhei.ttf")};

    for (const auto& path : candidates)
    {
        if (font.openFromFile(path))
        {
            return true;
        }
    }
    return false;
}
