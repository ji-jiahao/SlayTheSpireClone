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
        GameOver,
        BelialIntro,
        Ending
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
    void startBattle(bool preserveRetryState = false);
    bool startEvent(const std::string& eventId);
    void startRestRoom();
    void startShopRoom();
    void showMap();
    void showGameOver();
    void requestSceneChange(SceneType target);
    void updateSceneTransition(float deltaSeconds);
    void drawSceneTransitionOverlay();
    void finishBelialIntro();
    void startEndingSequence();
    void handleBattleResult();
    void updateEndingSequence(float deltaSeconds);
    void updateBelialTransition(float deltaSeconds);
    void handleBelialTransitionClick(sf::Vector2f mousePosition);
    void updateBelialTransitionHover(sf::Vector2f mousePosition);
    void startBelialRevivalChoice();
    void finishBelialRevival(bool believesInLight);
    void retryCurrentBattle();
    void prepareBattleReward();
    void updateBattleRewardHover(sf::Vector2f mousePosition);
    void handleBattleRewardClick(sf::Vector2f mousePosition);
    void drawMenuScene();
    void drawMapScene();
    void drawRestScene();
    void drawShopScene();
    void drawResultOverlay();
    void drawBattleRewardOverlay();
    void drawBelialTransitionOverlay();
    void drawGameOver();
    void drawBelialIntroScene();
    void drawEndingSequence();
    sf::FloatRect battleRewardCardBounds(std::size_t index) const;
    sf::FloatRect battleRewardSkipBounds() const;
    bool loadRestResources();
    bool loadShopResources();
    bool loadMapIconTextures();
    bool playMusic(const std::string& path, bool looping);
    void stopMusic();
    const sf::Texture* getMapNodeTexture(MapNodeType type) const;
    bool isMapNodeSelectable(const MapNode& node) const;
    std::vector<Card> buildCombatDeck() const;
    std::vector<MapNodeButton> layoutMapNodes() const;
    float getMaxMapScrollOffset() const;
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
    SceneType pendingScene_ = SceneType::Menu;
    bool sceneFadeActive_ = false;
    bool sceneFadePhase_ = false; // false: 淡出旧场景；true: 淡入新场景
    float sceneFadeTimer_ = 0.0f;
    BattleResult handledResult;
    int relicHealing;
    bool restedInCurrentRoom = false;
    bool removingCardInShop = false;
    std::string statusMessage;
    std::string lastError;
    std::vector<MapNode> mapNodes;
    std::vector<Card> battleRewardCards;
    sf::Texture battleNodeTexture;
    sf::Texture bossNodeTexture;
    sf::Texture restNodeTexture;
    sf::Texture shopNodeTexture;
    sf::Texture eventNodeTexture;
    sf::Texture menuBackgroundTexture;
    sf::Texture battleBackgroundTexture;
    sf::Texture mapBackgroundTexture;
    sf::Texture belialIntroBackgroundTexture;
    sf::Texture belialIntroBelialTexture;
    sf::Texture restBackgroundTexture;
    sf::Texture shopBackgroundTexture;
    sf::Music backgroundMusic;
    sf::Music belialIntroSound_;
    sf::Music belialHeartbeatSound_;
    sf::Music belialChargeSound_;
    bool mapIconsLoaded = false;
    bool menuBackgroundLoaded = false;
    bool battleBackgroundLoaded = false;
    bool mapBackgroundLoaded = false;
    bool belialIntroBackgroundLoaded = false;
    bool belialIntroBelialLoaded = false;
    bool restBackgroundLoaded = false;
    bool shopBackgroundLoaded = false;
    bool battleRewardVisible = false;
    int hoveredBattleRewardIndex = -1;
    std::size_t battleMusicIndex_ = 0;
    float mapScrollOffset_ = 0.0f;
    GameState battleStartState_;
    bool battleStartStateValid_ = false;
    bool battleIsBelial_ = false;
    bool belialReviveUsed_ = false;
    bool belialBelieveHovered_ = false;
    bool belialRejectHovered_ = false;
    enum class BelialTransitionState
    {
        Inactive,
        FadeOut,
        DefeatMessage,
        Choice,
        Charging,
        LightBurst,
        ReviveMessage
    };
    BelialTransitionState belialTransitionState_ = BelialTransitionState::Inactive;
    float belialTransitionTimer_ = 0.0f;
    float belialIntroTimer_ = 0.0f;
    float endingSequenceTimer_ = 0.0f;
};
