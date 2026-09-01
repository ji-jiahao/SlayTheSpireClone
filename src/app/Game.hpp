#pragma once

#include "combat/CombatSystem.hpp"
#include "core/GameState.hpp"
#include "map/MapGenerator.hpp"
#include "map/MapState.hpp"
#include "relic/RelicSystem.hpp"
#include "ui/BattleView.hpp"
#include "ui/MainMenuView.hpp"
#include "ui/MapView.hpp"
#include "ui/RoomView.hpp"

#include <SFML/Graphics.hpp>

#include <string>
#include <vector>

enum class SceneType
{
    MainMenu,
    Intro,
    Map,
    Battle,
    CardReward,
    Event,
    Rest,
    Shop,
    Treasure,
    ActResult,
    GameOver,
    DeckView
};

class Game
{
public:
    Game();
    void run();

private:
    void handleClick(sf::Vector2f position);
    void handleMenuClick(sf::Vector2f position);
    void handleRoomClick(sf::Vector2f position);
    void handleRoomAction(int actionIndex);
    void handleDeckClick(sf::Vector2f position);
    void draw();
    void drawRoom();
    void drawDeckView();
    void drawDeckButton();
    void startNewRun();
    void enterMapNode(int nodeId);
    void startCurrentBattle();
    void finishCurrentNode();
    void handleBattleResult();
    void prepareCardReward();
    void returnToMenu();
    std::vector<Card> buildCombatDeck() const;
    std::vector<RoomAction> currentActions() const;
    bool loadResources();

    sf::RenderWindow window_;
    sf::Font font_;
    sf::Texture dungeonTexture_;
    bool resourcesLoaded_ = false;
    MainMenuView mainMenuView_;
    MapView mapView_;
    BattleView battleView_;
    RoomView roomView_;
    SceneType scene_ = SceneType::MainMenu;
    GameState state_;
    MapGenerator mapGenerator_;
    MapState mapState_;
    CombatSystem combat_;
    RelicSystem relicSystem_;
    std::vector<Card> rewardCards_;
    std::string roomEyebrow_;
    std::string roomTitle_;
    std::string roomDescription_;
    int lastRelicHealing_ = 0;
    int battleNumber_ = 0;
    SceneType deckReturnScene_ = SceneType::Map;
    bool deckUpgradeMode_ = false;
};
