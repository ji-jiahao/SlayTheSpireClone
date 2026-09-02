#pragma once

#include "combat/CombatSystem.hpp"
#include "core/GameState.hpp"
#include "event/EventDatabase.hpp"
#include "event/EventSystem.hpp"
#include "map/MapNode.hpp"
#include "relic/RelicSystem.hpp"
#include "room/RestSystem.hpp"
#include "room/ShopSystem.hpp"
#include "ui/BattleView.hpp"
#include "ui/EventView.hpp"
#include "ui/MainMenuView.hpp"
#include "ui/RestView.hpp"
#include "ui/ShopView.hpp"

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>

#include <string>
#include <vector>

class Game
{
public:
    Game();
    void run();

private:
    enum class SceneType
    {
        Menu,
        Map,
        Event,
        Battle,
        Rest,
        Shop,
        GameOver
    };

    struct MapNodeButton
    {
        int nodeId = -1;
        sf::FloatRect bounds;
    };

    void handleWindowEvent(const sf::Event& event);
    void handleMenuAction(MainMenuView::Action action);
    void handleMapMouseClick(sf::Vector2f mousePosition);
    void handleRestAction(RestView::Action action);
    void handleShopAction(const ShopAction& action);
    void update(float deltaSeconds);
    void render();
    void startNewRun();
    void startBattle();
    bool startEvent(const std::string& eventId);
    void startRestRoom();
    void startShopRoom();
    void showMap();
    void showGameOver();
    void handleBattleResult();
    void drawMenuScene();
    void drawMapScene();
    void drawRestScene();
    void drawShopScene();
    void drawResultOverlay();
    void drawGameOver();
    bool loadRestResources();
    bool loadShopResources();
    bool loadMapIconTextures();
    bool playMusic(const std::string& path, bool looping);
    void stopMusic();
    const sf::Texture* getMapNodeTexture(MapNodeType type) const;
    bool isMapNodeSelectable(const MapNode& node) const;
    std::vector<Card> buildCombatDeck() const;
    std::vector<MapNodeButton> layoutMapNodes() const;
    sf::Text makeText(const std::string& text, unsigned int size,
                      sf::Color color) const;
    bool loadFont();

    sf::RenderWindow window;
    sf::Font font;
    bool fontLoaded;
    BattleView battleView;
    MainMenuView mainMenuView;
    RestView restView;
    ShopView shopView;
    CombatSystem combat;
    GameState state;
    RelicSystem relicSystem;
    RestSystem restSystem;
    ShopSystem shopSystem;
    EventDatabase eventDatabase;
    EventSystem eventSystem;
    EventView eventView;
    sf::Clock clock;
    SceneType scene;
    BattleResult handledResult;
    int relicHealing;
    bool restedInCurrentRoom = false;
    bool removingCardInShop = false;
    std::string statusMessage;
    std::string lastError;
    std::vector<MapNode> mapNodes;
    sf::Texture battleNodeTexture;
    sf::Texture bossNodeTexture;
    sf::Texture restNodeTexture;
    sf::Texture shopNodeTexture;
    sf::Texture eventNodeTexture;
    sf::Texture menuBackgroundTexture;
    sf::Texture restBackgroundTexture;
    sf::Music backgroundMusic;
    bool mapIconsLoaded = false;
    bool menuBackgroundLoaded = false;
    bool restBackgroundLoaded = false;
};
