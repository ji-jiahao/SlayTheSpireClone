#include "app/Game.hpp"

#include "card/CardDatabase.hpp"
#include "map/MapGenerator.hpp"
#include "ui/CardView.hpp"
#include "ui/MapIcons.hpp"
#include "ui/UiHelpers.hpp"

#include <array>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <optional>
#include <random>
#include <unordered_set>

namespace
{
constexpr unsigned int kWindowWidth = 1280;
constexpr unsigned int kWindowHeight = 720;
constexpr const char* kEventDataPath = "assets/data/events.json";
constexpr const char* kMenuBackgroundPath = "assets/images/menu/start_screen.png";
constexpr const char* kBattleBackgroundPath =
    "assets/images/background/battle_background.png";
constexpr const char* kBelialIntroBackgroundPath =
    "assets/images/background/belial_intro_earth.jpg";
constexpr const char* kBelialImagePath = "assets/images/enemies/belial.png";
constexpr const char* kShopBackgroundPath =
    "assets/images/background/shop_background.jpg";
constexpr const char* kRestBackgroundPath = "assets/images/rest/campfire_background.jpg";
constexpr const char* kMenuMusicPath = "assets/sounds/slay_the_spire.mp3";
constexpr const char* kMapMusicPath = "assets/sounds/exordium.mp3";
constexpr const char* kRestMusicPath = "assets/sounds/after_image.mp3";
constexpr std::array<const char*, 2> kBattleMusicPaths = {{
    "assets/sounds/battle_normal_2.mp3",
    "assets/sounds/battle_normal_3.mp3",
}};
constexpr const char* kMerchantMusicPath = "assets/sounds/meet_the_merchant.mp3";
constexpr const char* kBossMusicPath = "assets/sounds/the_heart.mp3";
constexpr const char* kFailureMusicPath = "assets/sounds/laoda_theme.ogg";
constexpr const char* kEndingMusicPath = "assets/sounds/ending_credits.mp3";
constexpr const char* kBelialIntroSoundPath = "assets/sounds/belial_intro.mp3";
constexpr const char* kSlainMusicPath = "assets/sounds/slain.mp3";
constexpr const char* kBelialHeartbeatSoundPath = "assets/sounds/belial_heartbeat.mp3";
constexpr const char* kBelialChargeSoundPath = "assets/sounds/belial_charge.mp3";
constexpr const char* kHeavyCrownMusicPath = "assets/sounds/heavy_is_the_crown.mp3";
constexpr const char* kMerchantFramesPath = "assets/images/shop/merchant_frames";
constexpr const char* kUniversityEventId = "university_choice";
constexpr const char* kNailongEventId = "sacred_nailong";
constexpr float kBattleRewardCardScale = 1.0f;
constexpr float kMapViewportTop = 240.0f;
constexpr float kMapViewportBottom = 625.0f;
constexpr float kMapNodeSpacingX = 250.0f;
constexpr float kMapContentWidth = 950.0f;
constexpr float kMapNodeSize = 72.0f;
constexpr float kMapScrollStep = 90.0f;
constexpr float kBelialFadeSeconds = 1.8f;
constexpr float kSceneFadeOutSeconds = 0.35f;
constexpr float kSceneFadeInSeconds = 0.35f;
constexpr float kBelialDefeatMessageSeconds = 1.2f;
constexpr float kBelialChargeSeconds = 7.0f;
constexpr float kBelialLightSeconds = 1.35f;
constexpr float kBelialMessageSeconds = 1.8f;
constexpr float kBelialIntroTransitionSeconds = 0.65f;
constexpr float kEndingFadeSeconds = 2.0f;
constexpr float kEndingThanksSeconds = 3.0f;
constexpr float kEndingCreditsSeconds = 11.0f;
constexpr float kEndingFinalSeconds = 3.0f;
constexpr std::array<const char*, 5> kEndingCredits = {{
    "贾跃\t总策划，地图逻辑，战斗逻辑测试及优化，角色UI设计",
    "冀家豪\t测试，整合优化",
    "李洛仪\t卡牌，角色及UI设计",
    "肖茗予\t战斗逻辑设计",
    "左心茹\t地图生成逻辑及地图设计优化",
}};

sf::String toSfString(const std::string& text)
{
    return sf::String::fromUtf8(text.begin(), text.end());
}

std::string mapNodeTypeName(MapNodeType type)
{
    switch (type)
    {
    case MapNodeType::Battle:
        return "战斗";
    case MapNodeType::Elite:
        return "精英战斗";
    case MapNodeType::Rest:
        return "休息";
    case MapNodeType::Shop:
        return "商店";
    case MapNodeType::Event:
        return "事件";
    case MapNodeType::Boss:
        return "首领";
    }

    return "未知";
}

sf::Color mapNodeColor(MapNodeType type)
{
    switch (type)
    {
    case MapNodeType::Battle:
        return sf::Color(178, 76, 66);
    case MapNodeType::Elite:
        return sf::Color(230, 96, 54);
    case MapNodeType::Rest:
        return sf::Color(65, 150, 94);
    case MapNodeType::Shop:
        return sf::Color(216, 175, 72);
    case MapNodeType::Event:
        return sf::Color(82, 145, 205);
    case MapNodeType::Boss:
        return sf::Color(230, 96, 54);
    }

    return sf::Color(160, 160, 160);
}

sf::Color mapLineColor()
{
    return sf::Color(116, 120, 126);
}

void drawThickLine(sf::RenderWindow& window, sf::Vector2f start,
                   sf::Vector2f end, float thickness = 8.0f)
{
    const sf::Vector2f delta = end - start;
    const float length = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    if (length <= 0.0f)
    {
        return;
    }

    sf::RectangleShape line({length, thickness});
    line.setOrigin({0.0f, thickness / 2.0f});
    line.setPosition(start);
    line.setRotation(sf::radians(std::atan2(delta.y, delta.x)));
    line.setFillColor(mapLineColor());
    window.draw(line);
}

void drawDashedCurve(sf::RenderWindow& window, sf::Vector2f start, sf::Vector2f end,
                     float curvature)
{
    const sf::Vector2f delta = end - start;
    const float length = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    if (length <= 0.0f)
    {
        return;
    }

    const sf::Vector2f control{(start.x + end.x) / 2.0f, (start.y + end.y) / 2.0f - curvature};
    constexpr int segments = 48;
    constexpr int dashPeriod = 2;

    auto pointAt = [&start, &control, &end](float t)
    {
        const float u = 1.0f - t;
        return sf::Vector2f{u * u * start.x + 2.0f * u * t * control.x + t * t * end.x,
                            u * u * start.y + 2.0f * u * t * control.y + t * t * end.y};
    };

    sf::Vector2f previous = pointAt(0.0f);
    for (int index = 1; index <= segments; ++index)
    {
        const float t = static_cast<float>(index) / static_cast<float>(segments);
        const sf::Vector2f current = pointAt(t);
        const float averageY = (previous.y + current.y) / 2.0f;
        if ((index / dashPeriod) % 2 == 0 && averageY >= kMapViewportTop &&
            averageY <= kMapViewportBottom)
        {
            drawThickLine(window, previous, current, 4.0f);
        }
        previous = current;
    }
}

std::optional<MapNode> findNodeById(const std::vector<MapNode>& nodes, int nodeId)
{
    const auto it = std::find_if(nodes.begin(), nodes.end(),
                                 [nodeId](const MapNode& node)
                                 {
                                     return node.id == nodeId;
                                 });
    if (it == nodes.end())
    {
        return std::nullopt;
    }

    return *it;
}

EncounterDefinition encounterForMapNode(const MapNode& node, unsigned int runSeed)
{
    if (node.type == MapNodeType::Boss)
    {
        return {"黑暗奥特曼 贝利亚", 150, 35, "belial"};
    }

    if (node.type == MapNodeType::Elite)
    {
        return {"乐加维林", 90, 18, "lagavulin"};
    }

    // 普通战斗按地图层数扩展敌人池，使用节点 ID 和运行种子保证可复现但不固定。
    static const std::array<EncounterDefinition, 3> earlyEncounters = {{
        {"邪教徒", 40, 6, "cultist"},
        {"颚虫", 42, 11, "jaw_worm"},
        {"酸液史莱姆", 30, 10, "acid_slime"},
    }};
    static const std::array<EncounterDefinition, 5> laterEncounters = {{
        {"邪教徒", 40, 6, "cultist"},
        {"颚虫", 42, 11, "jaw_worm"},
        {"酸液史莱姆", 30, 10, "acid_slime"},
        {"真菌兽", 40, 6, "fungi_beast"},
        {"乐加维林", 90, 18, "lagavulin"},
    }};

    const unsigned int mixedSeed = runSeed ^
                                    (static_cast<unsigned int>(node.id + 1) *
                                     2654435761u) ^
                                    (static_cast<unsigned int>(node.row + 1) *
                                     1013904223u);
    if (node.row < 2)
    {
        return earlyEncounters[mixedSeed % earlyEncounters.size()];
    }

    return laterEncounters[mixedSeed % laterEncounters.size()];
}
} // namespace

Game::Game()
    : window(sf::VideoMode({kWindowWidth, kWindowHeight}), "Slay the Spire Clone"),
      fontLoaded(false),
      eventSystem(eventDatabase),
      scene(SceneType::Menu),
      handledResult(BattleResult::Active),
      relicHealing(0)
{
    fontLoaded = loadFont();
    window.setFramerateLimit(60);

    if (fontLoaded)
    {
        battleView.setFont(font);
        mainMenuView.setFont(font);
        restView.setFont(font);
        shopView.setFont(font);
    }

    if (!eventView.loadFont("assets/fonts/simhei.ttf"))
    {
        lastError = eventView.getLastError();
        std::cerr << lastError << std::endl;
    }

    menuBackgroundLoaded = menuBackgroundTexture.loadFromFile(kMenuBackgroundPath);
    if (menuBackgroundLoaded)
    {
        mainMenuView.setBackground(menuBackgroundTexture);
    }
    else
    {
        std::cerr << "无法加载开始界面背景: " << kMenuBackgroundPath << std::endl;
    }

    battleBackgroundLoaded = battleBackgroundTexture.loadFromFile(kBattleBackgroundPath);
    if (battleBackgroundLoaded)
    {
        battleView.setBackground(battleBackgroundTexture);
    }
    else
    {
        std::cerr << "无法加载战斗背景: " << kBattleBackgroundPath << std::endl;
    }

    belialIntroBackgroundLoaded =
        belialIntroBackgroundTexture.loadFromFile(kBelialIntroBackgroundPath);
    if (!belialIntroBackgroundLoaded)
    {
        std::cerr << "无法加载贝利亚出场背景: "
                  << kBelialIntroBackgroundPath << std::endl;
    }

    belialIntroBelialLoaded = belialIntroBelialTexture.loadFromFile(kBelialImagePath);
    if (!belialIntroBelialLoaded)
    {
        std::cerr << "无法加载贝利亚出场立绘: " << kBelialImagePath << std::endl;
    }

    shopBackgroundLoaded = shopBackgroundTexture.loadFromFile(kShopBackgroundPath);
    if (shopBackgroundLoaded)
    {
        shopView.setBackground(&shopBackgroundTexture);
    }
    else
    {
        std::cerr << "无法加载商店背景: " << kShopBackgroundPath << std::endl;
    }

    if (!belialIntroSound_.openFromFile(kBelialIntroSoundPath))
    {
        std::cerr << "无法加载贝利亚 Boss 开场音效: " << kBelialIntroSoundPath << std::endl;
    }
    else
    {
        belialIntroSound_.setVolume(100.0f);
        belialIntroSound_.setLooping(false);
    }

    if (!belialHeartbeatSound_.openFromFile(kBelialHeartbeatSoundPath))
    {
        std::cerr << "无法加载贝利亚心跳音效: " << kBelialHeartbeatSoundPath << std::endl;
    }
    else
    {
        belialHeartbeatSound_.setVolume(85.0f);
        belialHeartbeatSound_.setLooping(true);
    }

    if (!belialChargeSound_.openFromFile(kBelialChargeSoundPath))
    {
        std::cerr << "无法加载贝利亚蓄力音效: " << kBelialChargeSoundPath << std::endl;
    }
    else
    {
        belialChargeSound_.setVolume(100.0f);
        belialChargeSound_.setLooping(false);
    }

    if (!eventDatabase.loadFromFile(kEventDataPath))
    {
        lastError = "加载事件数据库失败: " + eventDatabase.getLastError();
        std::cerr << lastError << std::endl;
    }

    if (!loadShopResources())
    {
        std::cerr << lastError << std::endl;
    }

    if (!loadRestResources())
    {
        std::cerr << lastError << std::endl;
    }

    if (restBackgroundLoaded)
    {
        restView.setBackground(&restBackgroundTexture);
    }

    if (!playMusic(kMenuMusicPath, true))
    {
        std::cerr << lastError << std::endl;
    }

    window.setTitle("东南苦行塔 - 主菜单");
}

void Game::run()
{
    while (window.isOpen())
    {
        while (const auto event = window.pollEvent())
        {
            handleWindowEvent(*event);
        }

        update(clock.restart().asSeconds());
        render();
    }
}

void Game::handleWindowEvent(const sf::Event& event)
{
    if (event.is<sf::Event::Closed>())
    {
        window.close();
        return;
    }

    if (sceneFadeActive_)
    {
        return;
    }

    if (const auto* key = event.getIf<sf::Event::KeyPressed>())
    {
        if (scene == SceneType::BelialIntro)
        {
            finishBelialIntro();
            return;
        }

        if (scene == SceneType::Event && eventView.handleAnyInput(eventSystem, state))
        {
            return;
        }

        if (scene == SceneType::Battle && battleView.handleKeyPress(key->code, combat))
        {
            return;
        }

        if (scene == SceneType::Battle &&
            belialTransitionState_ != BelialTransitionState::Inactive)
        {
            if (key->code == sf::Keyboard::Key::Escape &&
                belialTransitionState_ == BelialTransitionState::Choice)
            {
                finishBelialRevival(false);
            }
            return;
        }

        if (scene == SceneType::Battle && battleRewardVisible &&
            key->code == sf::Keyboard::Key::Escape)
        {
            battleRewardVisible = false;
            showMap();
            statusMessage = "跳过卡牌奖励，获得 50 金币。";
            return;
        }

        if (key->code == sf::Keyboard::Key::Escape)
        {
            window.close();
            return;
        }

        if (scene == SceneType::Menu && key->code == sf::Keyboard::Key::Enter)
        {
            startNewRun();
            return;
        }

    }

    if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>())
    {
        if (scene == SceneType::Menu)
        {
            mainMenuView.handleMouseMove(window.mapPixelToCoords(mouseMoved->position));
        }
        else if (scene == SceneType::Rest)
        {
            restView.handleMouseMove(window.mapPixelToCoords(mouseMoved->position));
        }
        else if (scene == SceneType::Shop)
        {
            shopView.handleMouseMove(window.mapPixelToCoords(mouseMoved->position),
                                     shopSystem, state, removingCardInShop);
        }
        else if (scene == SceneType::Battle)
        {
            const sf::Vector2f mousePosition =
                window.mapPixelToCoords(mouseMoved->position);
            if (belialTransitionState_ != BelialTransitionState::Inactive)
            {
                updateBelialTransitionHover(mousePosition);
            }
            else
            {
                battleView.handleMouseMove(mousePosition, combat);
                updateBattleRewardHover(mousePosition);
            }
        }
        else if (scene == SceneType::Event)
        {
            const sf::Vector2f viewSize = window.getView().getSize();
            eventView.handleMouseMove(window.mapPixelToCoords(mouseMoved->position),
                                      {static_cast<unsigned int>(viewSize.x),
                                       static_cast<unsigned int>(viewSize.y)},
                                      eventSystem);
        }
        return;
    }

    if (const auto* wheel = event.getIf<sf::Event::MouseWheelScrolled>())
    {
        if (scene == SceneType::Map)
        {
            mapScrollOffset_ = std::clamp(mapScrollOffset_ - wheel->delta * kMapScrollStep,
                                          0.0f, getMaxMapScrollOffset());
        }
        return;
    }

    if (const auto* mouse = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (scene == SceneType::BelialIntro)
        {
            finishBelialIntro();
            return;
        }

        if (scene == SceneType::Battle && mouse->button == sf::Mouse::Button::Right &&
            battleView.handleKeyPress(sf::Keyboard::Key::Escape, combat))
        {
            return;
        }

        if (mouse->button != sf::Mouse::Button::Left)
        {
            return;
        }

        const sf::Vector2f mousePosition = window.mapPixelToCoords(mouse->position);
        if (scene == SceneType::Battle &&
            belialTransitionState_ != BelialTransitionState::Inactive)
        {
            if (mouse->button == sf::Mouse::Button::Left)
            {
                handleBelialTransitionClick(mousePosition);
            }
            return;
        }
        if (scene == SceneType::Menu)
        {
            handleMenuAction(mainMenuView.handleMouseClick(mousePosition));
        }
        else if (scene == SceneType::Rest)
        {
            handleRestAction(restView.handleMouseClick(mousePosition));
        }
        else if (scene == SceneType::Shop)
        {
            handleShopAction(shopView.handleMouseClick(mousePosition, shopSystem,
                                                       state, removingCardInShop));
        }
        else if (scene == SceneType::Map)
        {
            handleMapMouseClick(mousePosition);
        }
        else if (scene == SceneType::Event)
        {
            const sf::Vector2f viewSize = window.getView().getSize();
            eventView.handleMouseClick(mousePosition,
                                       {static_cast<unsigned int>(viewSize.x),
                                        static_cast<unsigned int>(viewSize.y)},
                                       eventSystem, state);
        }
        else if (scene == SceneType::Battle)
        {
            if (combat.getResult() != BattleResult::Active)
            {
                if (battleView.isVisualLocked())
                {
                    return;
                }

                if (combat.getResult() == BattleResult::Victory && battleRewardVisible)
                {
                    handleBattleRewardClick(mousePosition);
                    return;
                }

                if (sf::FloatRect({520.0f, 416.0f}, {240.0f, 58.0f})
                        .contains(mousePosition))
                {
                    retryCurrentBattle();
                    return;
                }

                if (combat.getResult() == BattleResult::Defeat)
                {
                    return;
                }
                showMap();
                return;
            }

            battleView.handleMouseClick(mousePosition, combat);
            handleBattleResult();
        }
    }
}

void Game::handleMapMouseClick(sf::Vector2f mousePosition)
{
    if (mousePosition.x < 0 || mousePosition.x >= kMapContentWidth ||
        mousePosition.y < kMapViewportTop || mousePosition.y >= kMapViewportBottom) return;
    const sf::Vector2f worldMousePosition{mousePosition.x,
                                          mousePosition.y + mapScrollOffset_};
    for (const MapNodeButton& button : layoutMapNodes())
    {
        if (!button.bounds.contains(worldMousePosition))
        {
            continue;
        }

        const std::optional<MapNode> node = findNodeById(mapNodes, button.nodeId);
        if (!node.has_value())
        {
            return;
        }

        if (!isMapNodeSelectable(*node))
        {
            statusMessage = "只能沿着当前道路向上前进，不能回退或跳到其他分支。";
            return;
        }

        state.currentNodeId = node->id;
        if (node->type == MapNodeType::Event)
        {
            if (!state.hasVisitedEvent(kUniversityEventId) &&
                startEvent(kUniversityEventId))
            {
                return;
            }

        if (!state.hasVisitedEvent(kNailongEventId) &&
                startEvent(kNailongEventId))
            {
                return;
            }

            statusMessage = "本层事件已经触发过，选择战斗节点继续测试。";
            return;
        }

        if (node->type == MapNodeType::Battle || node->type == MapNodeType::Boss)
        {
            startBattle();
            return;
        }

        if (node->type == MapNodeType::Rest)
        {
            startRestRoom();
            return;
        }

        if (node->type == MapNodeType::Shop)
        {
            startShopRoom();
            return;
        }

        statusMessage = mapNodeTypeName(node->type) + "节点暂未接入，已沿当前道路前进。";
        return;
    }
}

void Game::update(float deltaSeconds)
{
    if (sceneFadeActive_)
    {
        updateSceneTransition(deltaSeconds);
        return;
    }

    if (scene == SceneType::Menu)
    {
        mainMenuView.update(deltaSeconds);
        return;
    }

    if (scene == SceneType::Event)
    {
        eventView.update(deltaSeconds);
        if (eventView.shouldReturnToMap())
        {
            eventView.clearReturnToMapRequest();
            if (state.isDead())
            {
                showGameOver();
            }
            else
            {
                showMap();
            }
        }
        return;
    }

    if (scene == SceneType::Battle)
    {
        battleView.update(deltaSeconds, combat);
        if (belialTransitionState_ != BelialTransitionState::Inactive)
        {
            updateBelialTransition(deltaSeconds);
        }
        else
        {
            combat.update();
            handleBattleResult();
        }
        return;
    }

    if (scene == SceneType::BelialIntro)
    {
        belialIntroTimer_ += std::max(0.0f, deltaSeconds);
        if (belialIntroTimer_ >= kBelialIntroTransitionSeconds &&
            belialIntroSound_.getStatus() != sf::SoundSource::Status::Playing)
        {
            finishBelialIntro();
        }
        return;
    }

    if (scene == SceneType::Shop)
    {
        shopView.update(deltaSeconds);
        return;
    }

    if (scene == SceneType::Ending)
    {
        updateEndingSequence(deltaSeconds);
    }
}

void Game::render()
{
    window.clear(sf::Color(35, 38, 42));

    switch (scene)
    {
    case SceneType::Menu:
        drawMenuScene();
        break;
    case SceneType::Map:
        drawMapScene();
        break;
    case SceneType::Event:
        eventView.draw(window, eventSystem, state);
        break;
    case SceneType::Battle:
        battleView.draw(window, combat);
        if (belialTransitionState_ != BelialTransitionState::Inactive)
        {
            drawBelialTransitionOverlay();
        }
        else if (!battleView.isVisualLocked())
        {
            if (battleRewardVisible)
            {
                drawBattleRewardOverlay();
            }
            else
            {
                drawResultOverlay();
            }
        }
        break;
    case SceneType::Rest:
        drawRestScene();
        break;
    case SceneType::Shop:
        drawShopScene();
        break;
    case SceneType::GameOver:
        drawGameOver();
        break;
    case SceneType::BelialIntro:
        drawBelialIntroScene();
        break;
    case SceneType::Ending:
        drawEndingSequence();
        break;
    }

    drawSceneTransitionOverlay();

    window.display();
}

void Game::handleMenuAction(MainMenuView::Action action)
{
    switch (action)
    {
    case MainMenuView::Action::Start:
        startNewRun();
        break;
    case MainMenuView::Action::None:
        break;
    }
}

void Game::handleRestAction(RestView::Action action)
{
    switch (action)
    {
    case RestView::Action::Rest:
        if (restedInCurrentRoom)
        {
            statusMessage = "已经休息过了。";
            return;
        }

        statusMessage = "休息后回复 " + std::to_string(restSystem.rest(state)) + " 点生命。";
        restedInCurrentRoom = true;
        break;
    case RestView::Action::Leave:
        showMap();
        break;
    case RestView::Action::None:
        break;
    }
}

void Game::handleShopAction(const ShopAction& action)
{
    switch (action.type)
    {
    case ShopActionType::BuyCard:
        if (shopSystem.buyCard(action.index, state))
        {
            statusMessage = "已购买卡牌。";
        }
        else
        {
            statusMessage = shopSystem.getLastError();
        }
        break;
    case ShopActionType::OpenRemove:
        if (state.deck.empty())
        {
            statusMessage = "牌组为空，无法删除卡牌。";
        }
        else if (state.gold < shopSystem.getRemoveCardPrice())
        {
            statusMessage = "金币不足，无法删除卡牌。";
        }
        else
        {
            removingCardInShop = true;
            statusMessage = "选择一张牌删除。";
        }
        break;
    case ShopActionType::RemoveCard:
        if (shopSystem.removeCard(state, action.index))
        {
            removingCardInShop = false;
            statusMessage = "已删除卡牌。";
        }
        else
        {
            statusMessage = shopSystem.getLastError();
        }
        break;
    case ShopActionType::CancelRemove:
        removingCardInShop = false;
        statusMessage = "已取消删牌。";
        break;
    case ShopActionType::Leave:
        showMap();
        break;
    case ShopActionType::None:
        break;
    }
}

void Game::startNewRun()
{
    state.reset();
    relicSystem = RelicSystem();
    eventSystem.setDatabase(eventDatabase);
    handledResult = BattleResult::Active;
    relicHealing = 0;
    restedInCurrentRoom = false;
    removingCardInShop = false;
    statusMessage.clear();
    lastError.clear();
    battleMusicIndex_ = 0;

    MapGenerator generator;
    mapNodes = generator.generateMap(8);
    mapScrollOffset_ = getMaxMapScrollOffset();

    requestSceneChange(SceneType::Map);
    state.currentNodeId = -1;
    playMusic(kMapMusicPath, true);
    window.setTitle("东南苦行塔 - 地图");
}

void Game::startBattle(bool preserveRetryState)
{
    if (!preserveRetryState)
    {
        battleStartState_ = state;
        battleStartStateValid_ = true;
        belialReviveUsed_ = false;
    }

    relicSystem.beginBattle();
    const RelicBattleStartModifiers modifiers = relicSystem.applyBattleStart(state);
    handledResult = BattleResult::Active;
    relicHealing = 0;
    statusMessage.clear();
    battleView.reset();
    battleRewardCards.clear();
    battleRewardVisible = false;
    hoveredBattleRewardIndex = -1;
    belialTransitionState_ = BelialTransitionState::Inactive;
    belialTransitionTimer_ = 0.0f;
    belialHeartbeatSound_.stop();
    belialChargeSound_.stop();
    const std::optional<MapNode> currentNode =
        findNodeById(mapNodes, state.currentNodeId);
    const EncounterDefinition encounter =
        currentNode.has_value()
            ? encounterForMapNode(*currentNode, state.seed)
            : EncounterDefinition{};
    battleIsBelial_ = encounter.enemyId == "belial";
    combat.startBattle(state.currentHealth, state.seed, buildCombatDeck(),
                       encounter, modifiers.block,
                       modifiers.strength + state.battleStartStrength,
                       modifiers.energy, modifiers.drawCards, state.maxHealth,
                       state.battleStartEnemyWeak);
    const bool bossBattle = currentNode.has_value() &&
                            currentNode->type == MapNodeType::Boss;
    const char* musicPath = kBossMusicPath;
    if (!bossBattle)
    {
        musicPath = kBattleMusicPaths[battleMusicIndex_ % kBattleMusicPaths.size()];
        ++battleMusicIndex_;
    }
    playMusic(musicPath, true);
    if (battleIsBelial_)
    {
        belialIntroSound_.stop();
        belialIntroSound_.play();
        belialIntroTimer_ = 0.0f;
        scene = SceneType::BelialIntro;
        window.setTitle("东南苦行塔 - 贝利亚出场");
        return;
    }

    requestSceneChange(SceneType::Battle);
    window.setTitle("Slay the Spire Clone - 战斗");
}

bool Game::startEvent(const std::string& eventId)
{
    if (!eventDatabase.hasEvent(eventId))
    {
        statusMessage = "找不到事件: " + eventId;
        return false;
    }

    if (!eventSystem.startEvent(eventId))
    {
        statusMessage = eventSystem.getLastError();
        return false;
    }

    if (!eventView.prepareEvent(eventSystem.getCurrentEvent()))
    {
        statusMessage = eventView.getLastError();
        return false;
    }

    eventView.enterCurrentState(eventSystem);
    requestSceneChange(SceneType::Event);
    stopMusic();
    window.setTitle("Slay the Spire Clone - 事件");
    return true;
}

void Game::startRestRoom()
{
    restedInCurrentRoom = false;
    removingCardInShop = false;
    statusMessage.clear();
    requestSceneChange(SceneType::Rest);
    playMusic(kRestMusicPath, true);
    window.setTitle("Slay the Spire Clone - 篝火");
}

void Game::startShopRoom()
{
    restedInCurrentRoom = false;
    removingCardInShop = false;
    statusMessage.clear();
    shopSystem.open(state.seed, state.currentNodeId);
    shopView.resetDialogue(state.seed ^ static_cast<unsigned int>(state.currentNodeId + 4096));
    requestSceneChange(SceneType::Shop);
    playMusic(kMerchantMusicPath, true);
    window.setTitle("Slay the Spire Clone - 商店");
}

void Game::showMap()
{
    requestSceneChange(SceneType::Map);
    statusMessage = "已返回地图。";
    playMusic(kMapMusicPath, true);
    window.setTitle("Slay the Spire Clone - 地图");
}

void Game::showGameOver()
{
    requestSceneChange(SceneType::GameOver);
    playMusic(kFailureMusicPath, false);
    window.setTitle("Slay the Spire Clone - 游戏结束");
}

void Game::requestSceneChange(SceneType target)
{
    pendingScene_ = target;
    sceneFadePhase_ = false;
    sceneFadeTimer_ = 0.0f;
    sceneFadeActive_ = true;
}

void Game::updateSceneTransition(float deltaSeconds)
{
    sceneFadeTimer_ += std::max(0.0f, deltaSeconds);
    const float duration = sceneFadePhase_ ? kSceneFadeInSeconds : kSceneFadeOutSeconds;
    if (sceneFadeTimer_ < duration)
    {
        return;
    }

    if (!sceneFadePhase_)
    {
        scene = pendingScene_;
        sceneFadePhase_ = true;
        sceneFadeTimer_ = 0.0f;
    }
    else
    {
        sceneFadeActive_ = false;
    }
}

void Game::drawSceneTransitionOverlay()
{
    if (!sceneFadeActive_)
    {
        return;
    }

    const float duration = sceneFadePhase_ ? kSceneFadeInSeconds : kSceneFadeOutSeconds;
    const float progress = std::clamp(sceneFadeTimer_ / duration, 0.0f, 1.0f);
    const float alpha01 = sceneFadePhase_ ? (1.0f - progress) : progress;
    sf::RectangleShape overlay({static_cast<float>(kWindowWidth),
                                static_cast<float>(kWindowHeight)});
    overlay.setFillColor(sf::Color(0, 0, 0,
                                   static_cast<std::uint8_t>(255.0f * alpha01)));
    window.draw(overlay);
}

void Game::finishBelialIntro()
{
    if (scene != SceneType::BelialIntro)
    {
        return;
    }

    belialIntroSound_.stop();
    belialIntroTimer_ = 0.0f;
    scene = SceneType::Battle;
    window.setTitle("Slay the Spire Clone - 战斗");
}

void Game::startEndingSequence()
{
    battleRewardVisible = false;
    battleRewardCards.clear();
    belialHeartbeatSound_.stop();
    belialChargeSound_.stop();
    belialIntroSound_.stop();
    endingSequenceTimer_ = 0.0f;
    belialIntroTimer_ = 0.0f;
    scene = SceneType::Ending;
    playMusic(kEndingMusicPath, false);
    window.setTitle("东南苦行塔 - 感谢游玩");
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
        state.maxHealth = combat.getPlayer().getMaxHealth();
        if (battleIsBelial_)
        {
            startEndingSequence();
            return;
        }

        relicHealing = relicSystem.applyBattleVictory(state);
        state.gainGold(50);
        prepareBattleReward();
        statusMessage = "战斗胜利";
        playMusic(kSlainMusicPath, false);
    }
    else
    {
        state.currentHealth = 0;
        statusMessage = "战斗失败";
        if (battleIsBelial_ && !belialReviveUsed_)
        {
            startBelialRevivalChoice();
        }
        else
        {
            playMusic(kFailureMusicPath, false);
        }
    }
}

void Game::updateEndingSequence(float deltaSeconds)
{
    endingSequenceTimer_ += std::max(0.0f, deltaSeconds);
    const float totalSeconds = kEndingFadeSeconds + kEndingThanksSeconds +
                               kEndingCreditsSeconds + kEndingFinalSeconds;
    if (endingSequenceTimer_ < totalSeconds)
    {
        return;
    }

    state.reset();
    relicSystem = RelicSystem();
    eventSystem.setDatabase(eventDatabase);
    handledResult = BattleResult::Active;
    relicHealing = 0;
    restedInCurrentRoom = false;
    removingCardInShop = false;
    statusMessage.clear();
    lastError.clear();
    mapNodes.clear();
    battleRewardCards.clear();
    battleRewardVisible = false;
    hoveredBattleRewardIndex = -1;
    battleStartStateValid_ = false;
    battleIsBelial_ = false;
    belialReviveUsed_ = false;
    belialTransitionState_ = BelialTransitionState::Inactive;
    belialIntroTimer_ = 0.0f;
    endingSequenceTimer_ = 0.0f;
    combat = CombatSystem();
    mainMenuView.resetFade();
    scene = SceneType::Menu;
    playMusic(kMenuMusicPath, true);
    window.setTitle("东南苦行塔 - 主菜单");
}

void Game::retryCurrentBattle()
{
    battleStartStateValid_ = false;
    battleIsBelial_ = false;
    belialReviveUsed_ = false;
    battleRewardVisible = false;
    battleRewardCards.clear();
    battleView.reset();
    combat = CombatSystem();
    state.reset();
    relicSystem = RelicSystem();
    eventSystem.setDatabase(eventDatabase);
    statusMessage.clear();
    lastError.clear();
    belialTransitionState_ = BelialTransitionState::Inactive;
    belialIntroTimer_ = 0.0f;
    belialHeartbeatSound_.stop();
    belialChargeSound_.stop();
    stopMusic();
    mainMenuView.resetFade();
    requestSceneChange(SceneType::Menu);
    playMusic(kMenuMusicPath, true);
    window.setTitle("东南苦行塔 - 主菜单");
}

void Game::startBelialRevivalChoice()
{
    if (!battleIsBelial_ || belialReviveUsed_ ||
        belialTransitionState_ != BelialTransitionState::Inactive)
    {
        return;
    }

    belialTransitionState_ = BelialTransitionState::FadeOut;
    belialTransitionTimer_ = 0.0f;
    belialBelieveHovered_ = false;
    belialRejectHovered_ = false;
    belialHeartbeatSound_.stop();
    belialHeartbeatSound_.play();
    stopMusic();
}

void Game::finishBelialRevival(bool believesInLight)
{
    if (!believesInLight)
    {
        belialReviveUsed_ = true;
        belialHeartbeatSound_.stop();
        belialChargeSound_.stop();
        belialTransitionState_ = BelialTransitionState::Inactive;
        belialTransitionTimer_ = 0.0f;
        state.currentHealth = 0;
        showGameOver();
        return;
    }

    belialReviveUsed_ = true;
    belialHeartbeatSound_.stop();
    belialChargeSound_.stop();
    belialChargeSound_.play();
    belialTransitionState_ = BelialTransitionState::Charging;
    belialTransitionTimer_ = 0.0f;
}

void Game::updateBelialTransition(float deltaSeconds)
{
    belialTransitionTimer_ += std::max(0.0f, deltaSeconds);
    switch (belialTransitionState_)
    {
    case BelialTransitionState::FadeOut:
        if (belialTransitionTimer_ >= kBelialFadeSeconds)
        {
            belialTransitionState_ = BelialTransitionState::DefeatMessage;
            belialTransitionTimer_ = 0.0f;
        }
        break;
    case BelialTransitionState::DefeatMessage:
        if (belialTransitionTimer_ >= kBelialDefeatMessageSeconds)
        {
            belialTransitionState_ = BelialTransitionState::Choice;
            belialTransitionTimer_ = 0.0f;
        }
        break;
    case BelialTransitionState::Charging:
        if (belialTransitionTimer_ >= kBelialChargeSeconds)
        {
            belialTransitionState_ = BelialTransitionState::LightBurst;
            belialTransitionTimer_ = 0.0f;
        }
        break;
    case BelialTransitionState::LightBurst:
        if (belialTransitionTimer_ >= kBelialLightSeconds)
        {
            if (!combat.reviveFromLastSafeSnapshot(10, 10, true))
            {
                state.currentHealth = 0;
                showGameOver();
                return;
            }
            state.currentHealth = combat.getPlayer().getCurrentHealth();
            statusMessage = "光给予了你力量。";
            playMusic(kHeavyCrownMusicPath, true);
            belialTransitionState_ = BelialTransitionState::ReviveMessage;
            belialTransitionTimer_ = 0.0f;
        }
        break;
    case BelialTransitionState::ReviveMessage:
        if (belialTransitionTimer_ >= kBelialMessageSeconds)
        {
            belialTransitionState_ = BelialTransitionState::Inactive;
            belialTransitionTimer_ = 0.0f;
            handledResult = BattleResult::Active;
            statusMessage.clear();
        }
        break;
    case BelialTransitionState::Choice:
    case BelialTransitionState::Inactive:
        break;
    }
}

void Game::updateBelialTransitionHover(sf::Vector2f mousePosition)
{
    belialBelieveHovered_ =
        sf::FloatRect({430.0f, 480.0f}, {180.0f, 64.0f}).contains(mousePosition);
    belialRejectHovered_ =
        sf::FloatRect({670.0f, 480.0f}, {180.0f, 64.0f}).contains(mousePosition);
}

void Game::handleBelialTransitionClick(sf::Vector2f mousePosition)
{
    if (belialTransitionState_ != BelialTransitionState::Choice)
    {
        return;
    }

    if (sf::FloatRect({430.0f, 480.0f}, {180.0f, 64.0f}).contains(mousePosition))
    {
        finishBelialRevival(true);
    }
    else if (sf::FloatRect({670.0f, 480.0f}, {180.0f, 64.0f}).contains(mousePosition))
    {
        finishBelialRevival(false);
    }
}

void Game::prepareBattleReward()
{
    battleRewardCards.clear();
    hoveredBattleRewardIndex = -1;

    std::vector<Card> pool;
    for (const Card& card : CardDatabase::createIroncladCardPool())
    {
        if (card.rarity == CardRarity::Starter || card.rarity == CardRarity::Status ||
            card.rarity == CardRarity::Curse)
        {
            continue;
        }

        pool.push_back(card);
    }

    std::random_device randomDevice;
    std::seed_seq seedSequence{
        randomDevice(), randomDevice(), randomDevice(), state.seed,
        static_cast<unsigned int>(state.currentNodeId + 1)};
    std::mt19937 engine(seedSequence);
    std::shuffle(pool.begin(), pool.end(), engine);

    std::unordered_set<std::string> selectedIds;
    for (const Card& card : pool)
    {
        if (!selectedIds.insert(card.id).second)
        {
            continue;
        }

        battleRewardCards.push_back(card);
        if (battleRewardCards.size() >= 3)
        {
            break;
        }
    }

    battleRewardVisible = !battleRewardCards.empty();
}

void Game::updateBattleRewardHover(sf::Vector2f mousePosition)
{
    hoveredBattleRewardIndex = -1;
    if (!battleRewardVisible || combat.getResult() != BattleResult::Victory)
    {
        return;
    }

    for (std::size_t index = 0; index < battleRewardCards.size(); ++index)
    {
        if (battleRewardCardBounds(index).contains(mousePosition))
        {
            hoveredBattleRewardIndex = static_cast<int>(index);
            return;
        }
    }

    if (battleRewardSkipBounds().contains(mousePosition))
    {
        hoveredBattleRewardIndex = -2;
    }
}

void Game::handleBattleRewardClick(sf::Vector2f mousePosition)
{
    if (!battleRewardVisible)
    {
        return;
    }

    for (std::size_t index = 0; index < battleRewardCards.size(); ++index)
    {
        if (!battleRewardCardBounds(index).contains(mousePosition))
        {
            continue;
        }

        const std::string cardName = battleRewardCards[index].name;
        state.addCard(battleRewardCards[index].id);
        battleRewardVisible = false;
        showMap();
        statusMessage = "已将「" + cardName + "」加入牌组，并获得 50 金币。";
        return;
    }

    if (battleRewardSkipBounds().contains(mousePosition))
    {
        battleRewardVisible = false;
        showMap();
        statusMessage = "跳过卡牌奖励，获得 50 金币。";
        return;
    }

    if (sf::FloatRect({70.0f, 620.0f}, {240.0f, 60.0f}).contains(mousePosition))
    {
        battleRewardVisible = false;
        retryCurrentBattle();
    }
}

void Game::drawMenuScene()
{
    mainMenuView.draw(window);
}

void Game::drawMapScene()
{
    sf::RectangleShape background({static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)});
    background.setFillColor(sf::Color(224, 211, 184));
    window.draw(background);

    if (!fontLoaded)
    {
        return;
    }

    sf::Text title = makeText("地图", 42, sf::Color(65, 49, 35));
    title.setPosition({70.0f, 48.0f});
    window.draw(title);

    sf::Text hint = makeText("从最底层开始选择路线，之后只能沿连线向上前进。",
                             22, sf::Color(94, 75, 54));
    hint.setPosition({70.0f, 108.0f});
    window.draw(hint);

    sf::Text status = makeText("当前生命: " + std::to_string(state.currentHealth) +
                                   "/" + std::to_string(state.maxHealth) +
                                   "    金币: " + std::to_string(state.gold),
                               24, sf::Color(102, 65, 25));
    status.setPosition({70.0f, 154.0f});
    window.draw(status);

    if (!statusMessage.empty())
    {
        sf::Text message = makeText(statusMessage, 18, sf::Color(94, 75, 54));
        message.setPosition({70.0f, 196.0f});
        window.draw(message);
    }

    // Clip routes and partially visible icons to the same region used for input.
    const sf::View originalView = window.getView();
    const auto originalViewport = originalView.getViewport();
    sf::View mapView(sf::FloatRect({0, kMapViewportTop},
                                  {kMapContentWidth, kMapViewportBottom - kMapViewportTop}));
    mapView.setViewport({
        {originalViewport.position.x, originalViewport.position.y + originalViewport.size.y * kMapViewportTop / kWindowHeight},
        {originalViewport.size.x * kMapContentWidth / kWindowWidth,
         originalViewport.size.y * (kMapViewportBottom - kMapViewportTop) / kWindowHeight}});
    window.setView(mapView);

    const std::vector<MapNodeButton> buttons = layoutMapNodes();
    for (const MapNode& node : mapNodes)
    {
        const auto buttonIt = std::find_if(buttons.begin(), buttons.end(),
                                           [&node](const MapNodeButton& button)
                                           {
                                               return button.nodeId == node.id;
                                           });
        if (buttonIt == buttons.end())
        {
            continue;
        }

        const sf::Vector2f center = buttonIt->bounds.getCenter();
        for (int targetId : node.nextNodeIds)
        {
            const auto targetIt = std::find_if(buttons.begin(), buttons.end(),
                                               [targetId](const MapNodeButton& button)
                                               {
                                                   return button.nodeId == targetId;
                                               });
            if (targetIt == buttons.end())
            {
                continue;
            }

            const sf::Vector2f targetCenter = targetIt->bounds.getCenter();
            drawDashedCurve(window, center - sf::Vector2f{0.0f, mapScrollOffset_},
                            targetCenter - sf::Vector2f{0.0f, mapScrollOffset_},
                            0.0f);
        }
    }

    for (const MapNodeButton& button : buttons)
    {
        const std::optional<MapNode> node = findNodeById(mapNodes, button.nodeId);
        if (!node.has_value())
        {
            continue;
        }

        const bool selected = node->id == state.currentNodeId;
        const bool selectable = isMapNodeSelectable(*node);
        const sf::Vector2f screenPosition{button.bounds.position.x,
                                          button.bounds.position.y - mapScrollOffset_};
        if (screenPosition.y + kMapNodeSize < kMapViewportTop ||
            screenPosition.y > kMapViewportBottom)
        {
            continue;
        }
        sf::CircleShape backing(kMapNodeSize / 2, 40);
        backing.setPosition(screenPosition);
        backing.setFillColor(sf::Color(224,211,184));
        window.draw(backing);
        MapIcons::draw(window, node->type, screenPosition + sf::Vector2f{7,7}, kMapNodeSize - 14,
                       selectable || selected ? sf::Color(55,44,33) : sf::Color(116,103,83));

        if (selectable || selected)
        {
            sf::CircleShape outline(button.bounds.size.x / 2.0f, 48);
            outline.setPosition(screenPosition);
            outline.setFillColor(sf::Color::Transparent);
            outline.setOutlineThickness(selected ? 5.0f : 3.0f);
            outline.setOutlineColor(selected ? sf::Color(144, 65, 34)
                                             : sf::Color(97, 111, 61));
            window.draw(outline);
        }

    }

    window.setView(originalView);
    UiHelpers::drawText(window, font, "滚轮上下查看地图 · 绿色圆圈表示可前往", 18,
                        {70, 666}, sf::Color(94,75,54));
    // 固定图例：不随地图滚动，说明所有节点图标含义。
    sf::RectangleShape legend({265.0f, 442.0f});
    legend.setPosition({985.0f, 150.0f});
    legend.setFillColor(sf::Color(244, 235, 214, 235));
    legend.setOutlineColor(sf::Color(111, 88, 61));
    legend.setOutlineThickness(2.0f);
    window.draw(legend);
    sf::Text legendTitle = makeText("图标说明", 25, sf::Color(65, 49, 35));
    legendTitle.setPosition({1010.0f, 170.0f});
    window.draw(legendTitle);
    struct LegendItem { MapNodeType type; const char* label; const char* description; };
    const std::array<LegendItem, 6> legendItems = {{
        {MapNodeType::Battle, "普通战斗", "获得金币和卡牌"},
        {MapNodeType::Elite, "精英战斗", "更强的敌人"},
        {MapNodeType::Shop, "商店", "购买卡牌或删牌"},
        {MapNodeType::Event, "事件", "做出选择，触发事件"},
        {MapNodeType::Rest, "篝火", "休息恢复生命"},
        {MapNodeType::Boss, "Boss", "本幕最终战"},
    }};
    for (std::size_t i = 0; i < legendItems.size(); ++i)
    {
        const float y = 215.0f + static_cast<float>(i) * 61.0f;
        MapIcons::draw(window,legendItems[i].type,{1000,y},38,sf::Color(55,44,33));
        sf::Text label = makeText(legendItems[i].label, 19, sf::Color(65, 49, 35));
        label.setPosition({1050.0f, y});
        window.draw(label);
        UiHelpers::drawText(window,font,legendItems[i].description,14,{1050,y+25},sf::Color(94,75,54));
    }
}

void Game::drawRestScene()
{
    restView.draw(window, state, restSystem.getHealAmount(state),
                  restedInCurrentRoom, statusMessage);
}

void Game::drawShopScene()
{
    shopView.draw(window, shopSystem, state, removingCardInShop, statusMessage);
}

void Game::drawResultOverlay()
{
    // Victory has exactly one UI: the reward overlay. Never revive the legacy
    // result panel while the reward closes and the map transition fades out.
    if (combat.getResult() != BattleResult::Defeat)
    {
        return;
    }

    sf::RectangleShape overlay({560.0f, 280.0f});
    overlay.setPosition({360.0f, 190.0f});
    overlay.setFillColor(sf::Color(22, 24, 28, 235));
    overlay.setOutlineColor(sf::Color(230, 174, 72));
    overlay.setOutlineThickness(3.0f);
    window.draw(overlay);

    if (!fontLoaded)
    {
        return;
    }

    sf::Text title = makeText(statusMessage, 40, sf::Color(245, 220, 150));
    title.setPosition({510.0f, 220.0f});
    window.draw(title);

    const std::string detail = "本次挑战结束";

    sf::Text detailText = makeText(detail, 22, sf::Color(235, 229, 207));
    detailText.setPosition({430.0f, 295.0f});
    window.draw(detailText);

    UiHelpers::drawButton(window, font, {{520.0f, 416.0f}, {240.0f, 58.0f}},
                          "重试一次", true, false);
}

void Game::drawBelialTransitionOverlay()
{
    const float progress = std::clamp(
        belialTransitionTimer_ /
            (belialTransitionState_ == BelialTransitionState::FadeOut
                 ? kBelialFadeSeconds
                 : belialTransitionState_ == BelialTransitionState::LightBurst
                       ? kBelialLightSeconds
                       : 1.0f),
        0.0f, 1.0f);

    if (belialTransitionState_ == BelialTransitionState::FadeOut ||
        belialTransitionState_ == BelialTransitionState::DefeatMessage ||
        belialTransitionState_ == BelialTransitionState::Choice ||
        belialTransitionState_ == BelialTransitionState::Charging)
    {
        const float alpha = belialTransitionState_ == BelialTransitionState::FadeOut
                                ? progress * 255.0f
                                : 255.0f;
        sf::RectangleShape black({static_cast<float>(kWindowWidth),
                                  static_cast<float>(kWindowHeight)});
        black.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(alpha)));
        window.draw(black);
    }

    if (belialTransitionState_ == BelialTransitionState::LightBurst)
    {
        const float eased = progress * progress * (3.0f - 2.0f * progress);
        sf::RectangleShape gold({static_cast<float>(kWindowWidth),
                                 static_cast<float>(kWindowHeight)});
        gold.setFillColor(sf::Color(255, 210, 72,
                                    static_cast<std::uint8_t>(255.0f * eased)));
        window.draw(gold);
        sf::CircleShape light(80.0f + 780.0f * eased, 64);
        light.setOrigin({light.getRadius(), light.getRadius()});
        light.setPosition({640.0f, 360.0f});
        light.setFillColor(sf::Color(255, 245, 180,
                                     static_cast<std::uint8_t>(180.0f * (1.0f - eased))));
        window.draw(light);
    }

    if (!fontLoaded)
    {
        return;
    }

    if (belialTransitionState_ == BelialTransitionState::DefeatMessage)
    {
        UiHelpers::drawCenteredText(window, font, "你被打倒了", 48,
                                    {{180.0f, 265.0f}, {920.0f, 75.0f}},
                                    sf::Color(255, 255, 255));
    }
    else if (belialTransitionState_ == BelialTransitionState::Choice)
    {
        UiHelpers::drawCenteredText(window, font, "你相信光吗？", 52,
                                    {{120.0f, 205.0f}, {1040.0f, 90.0f}},
                                    sf::Color(255, 255, 255));
        UiHelpers::drawButton(window, font, {{430.0f, 480.0f}, {180.0f, 64.0f}},
                              "相信", true, belialBelieveHovered_);
        UiHelpers::drawButton(window, font, {{670.0f, 480.0f}, {180.0f, 64.0f}},
                              "不相信", true, belialRejectHovered_);
    }
    else if (belialTransitionState_ == BelialTransitionState::Charging)
    {
        UiHelpers::drawCenteredText(window, font, "光正在回应你...", 34,
                                    {{250.0f, 270.0f}, {780.0f, 60.0f}},
                                    sf::Color(255, 235, 166));
    }
    else if (belialTransitionState_ == BelialTransitionState::ReviveMessage)
    {
        sf::RectangleShape veil({static_cast<float>(kWindowWidth),
                                 static_cast<float>(kWindowHeight)});
        veil.setFillColor(sf::Color(255, 213, 84, 36));
        window.draw(veil);
        UiHelpers::drawCenteredText(window, font, "光给予了你力量。", 42,
                                    {{180.0f, 270.0f}, {920.0f, 70.0f}},
                                    sf::Color(255, 245, 190));
    }

    sf::RectangleShape topMask({static_cast<float>(kWindowWidth), 150.0f});
    topMask.setFillColor(sf::Color(8, 12, 17, 105));
    window.draw(topMask);
    sf::RectangleShape bottomMask(
        {static_cast<float>(kWindowWidth), kWindowHeight - kMapViewportBottom});
    bottomMask.setPosition({0.0f, kMapViewportBottom});
    bottomMask.setFillColor(sf::Color(8, 12, 17, 125));
    window.draw(bottomMask);
}

sf::FloatRect Game::battleRewardCardBounds(std::size_t index) const
{
    const sf::Vector2f cardSize = CardView::getCardSize() * kBattleRewardCardScale;
    const float totalWidth = 3.0f * cardSize.x + 2.0f * 28.0f;
    const float startX = (static_cast<float>(kWindowWidth) - totalWidth) / 2.0f;
    return {{startX + static_cast<float>(index) * (cardSize.x + 28.0f), 230.0f},
            cardSize};
}

sf::FloatRect Game::battleRewardSkipBounds() const
{
    return {{930.0f, 620.0f}, {220.0f, 60.0f}};
}

void Game::drawBattleRewardOverlay()
{
    if (!battleRewardVisible || combat.getResult() != BattleResult::Victory)
    {
        return;
    }

    sf::RectangleShape veil(
        {static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)});
    veil.setFillColor(sf::Color(8, 8, 12, 170));
    window.draw(veil);

    sf::RectangleShape panel({900.0f, 560.0f});
    panel.setPosition({190.0f, 75.0f});
    panel.setFillColor(sf::Color(26, 27, 33, 248));
    panel.setOutlineColor(sf::Color(230, 174, 72));
    panel.setOutlineThickness(4.0f);
    window.draw(panel);

    if (!fontLoaded)
    {
        return;
    }

    UiHelpers::drawCenteredText(window, font, "战斗胜利", 42,
                                {{230.0f, 98.0f}, {820.0f, 54.0f}},
                                sf::Color(246, 220, 150));
    UiHelpers::drawCenteredText(window, font, "获得 50 金币，选择一张卡牌加入牌组",
                                24, {{230.0f, 156.0f}, {820.0f, 40.0f}},
                                sf::Color(232, 224, 204));

    for (std::size_t index = 0; index < battleRewardCards.size(); ++index)
    {
        const sf::FloatRect bounds = battleRewardCardBounds(index);
        CardView cardView;
        cardView.setFont(font);
        const float cardOffset =
            CardView::getCardSize().x * (1.0f - kBattleRewardCardScale) / 2.0f;
        cardView.setPosition({bounds.position.x - cardOffset,
                              bounds.position.y - cardOffset});
        cardView.setScale(kBattleRewardCardScale);
        cardView.draw(window, battleRewardCards[index]);

        sf::RectangleShape outline(bounds.size);
        outline.setPosition(bounds.position);
        outline.setFillColor(sf::Color::Transparent);
        outline.setOutlineColor(index == static_cast<std::size_t>(hoveredBattleRewardIndex)
                                   ? sf::Color(255, 230, 126)
                                   : sf::Color(145, 120, 76));
        outline.setOutlineThickness(index == static_cast<std::size_t>(hoveredBattleRewardIndex)
                                        ? 5.0f
                                        : 2.0f);
        window.draw(outline);
    }

    UiHelpers::drawButton(window, font, battleRewardSkipBounds(), "跳过",
                          true, hoveredBattleRewardIndex == -2);
    UiHelpers::drawButton(window, font, {{70.0f, 620.0f}, {240.0f, 60.0f}},
                          "重试一次", true, false);
    UiHelpers::drawCenteredText(window, font, "点击卡牌领取，或跳过本次卡牌奖励",
                                18, {{240.0f, 575.0f}, {800.0f, 34.0f}},
                                sf::Color(190, 186, 174));
}

void Game::drawGameOver()
{
    sf::RectangleShape background({static_cast<float>(kWindowWidth),
                                   static_cast<float>(kWindowHeight)});
    background.setFillColor(sf::Color(34, 18, 20));
    window.draw(background);

    if (!fontLoaded)
    {
        return;
    }

    sf::Text title = makeText("游戏结束", 54, sf::Color(238, 221, 210));
    title.setPosition({80.0f, 80.0f});
    window.draw(title);

    sf::Text message = makeText("生命值降为 0，已进入正常死亡流程占位界面。",
                                26, sf::Color(222, 183, 166));
    message.setPosition({80.0f, 170.0f});
    window.draw(message);
}

void Game::drawBelialIntroScene()
{
    const float transitionProgress =
        std::clamp(belialIntroTimer_ / kBelialIntroTransitionSeconds, 0.0f, 1.0f);
    const float easedProgress =
        transitionProgress * transitionProgress * (3.0f - 2.0f * transitionProgress);
    const auto introAlpha =
        static_cast<std::uint8_t>(std::clamp(255.0f * easedProgress, 0.0f, 255.0f));

    if (transitionProgress < 1.0f)
    {
        drawMapScene();
        sf::RectangleShape mapDim({static_cast<float>(kWindowWidth),
                                   static_cast<float>(kWindowHeight)});
        mapDim.setFillColor(sf::Color(
            0, 0, 0,
            static_cast<std::uint8_t>(std::clamp(180.0f * easedProgress,
                                                 0.0f, 180.0f))));
        window.draw(mapDim);
    }

    if (belialIntroBackgroundLoaded)
    {
        sf::Sprite background(belialIntroBackgroundTexture);
        const sf::FloatRect bounds = background.getLocalBounds();
        const float scale = std::max(static_cast<float>(kWindowWidth) / bounds.size.x,
                                     static_cast<float>(kWindowHeight) / bounds.size.y);
        background.setScale({scale, scale});
        background.setPosition(
            {(static_cast<float>(kWindowWidth) - bounds.size.x * scale) / 2.0f,
             (static_cast<float>(kWindowHeight) - bounds.size.y * scale) / 2.0f});
        background.setColor(sf::Color(255, 255, 255, introAlpha));
        window.draw(background);
    }
    else
    {
        sf::RectangleShape background({static_cast<float>(kWindowWidth),
                                       static_cast<float>(kWindowHeight)});
        background.setFillColor(sf::Color(5, 8, 14, introAlpha));
        window.draw(background);
    }

    sf::RectangleShape veil({static_cast<float>(kWindowWidth),
                             static_cast<float>(kWindowHeight)});
    veil.setFillColor(sf::Color(
        0, 0, 0,
        static_cast<std::uint8_t>(std::clamp(55.0f * easedProgress, 0.0f, 55.0f))));
    window.draw(veil);

    if (!belialIntroBelialLoaded)
    {
        return;
    }

    sf::Sprite belial(belialIntroBelialTexture);
    const sf::Vector2u textureSize = belialIntroBelialTexture.getSize();
    belial.setOrigin({static_cast<float>(textureSize.x) / 2.0f,
                      static_cast<float>(textureSize.y) / 2.0f});
    belial.setPosition({static_cast<float>(kWindowWidth) / 2.0f,
                        static_cast<float>(kWindowHeight) / 2.0f + 28.0f});
    const float targetHeight = 560.0f;
    const float scale = targetHeight / static_cast<float>(textureSize.y);
    belial.setScale({scale, scale});
    belial.setColor(sf::Color(255, 255, 255, introAlpha));
    window.draw(belial);
}

void Game::drawEndingSequence()
{
    const float thanksStart = kEndingFadeSeconds;
    const float creditsStart = thanksStart + kEndingThanksSeconds;
    const float finalStart = creditsStart + kEndingCreditsSeconds;

    if (endingSequenceTimer_ < kEndingFadeSeconds)
    {
        battleView.draw(window, combat);
        const float progress = std::clamp(endingSequenceTimer_ / kEndingFadeSeconds,
                                          0.0f, 1.0f);
        sf::RectangleShape fade({static_cast<float>(kWindowWidth),
                                 static_cast<float>(kWindowHeight)});
        fade.setFillColor(sf::Color(0, 0, 0,
                                    static_cast<std::uint8_t>(255.0f * progress)));
        window.draw(fade);
    }
    else
    {
        sf::RectangleShape background({static_cast<float>(kWindowWidth),
                                       static_cast<float>(kWindowHeight)});
        background.setFillColor(sf::Color::Black);
        window.draw(background);
    }

    if (!fontLoaded)
    {
        return;
    }

    if (endingSequenceTimer_ >= thanksStart &&
        endingSequenceTimer_ < creditsStart)
    {
        UiHelpers::drawCenteredText(window, font, "感谢游玩", 60,
                                    {{120.0f, 292.0f}, {1040.0f, 96.0f}},
                                    sf::Color::White);
    }
    else if (endingSequenceTimer_ >= creditsStart &&
             endingSequenceTimer_ < finalStart)
    {
        const float progress = std::clamp(
            (endingSequenceTimer_ - creditsStart) / kEndingCreditsSeconds,
            0.0f, 1.0f);
        const float endY =
            -80.0f - static_cast<float>(kEndingCredits.size()) * 70.0f;
        const float y = 760.0f + (endY - 760.0f) * progress;
        for (std::size_t index = 0; index < kEndingCredits.size(); ++index)
        {
            sf::Text credit = makeText(kEndingCredits[index], 27,
                                       sf::Color(242, 242, 242));
            credit.setPosition({165.0f, y + static_cast<float>(index) * 70.0f});
            window.draw(credit);
        }
    }
    else if (endingSequenceTimer_ >= finalStart)
    {
        UiHelpers::drawCenteredText(window, font, "致每一个爱爬塔的你", 48,
                                    {{120.0f, 292.0f}, {1040.0f, 96.0f}},
                                    sf::Color::White);
    }

    sf::Text hint = makeText("可按ESC退出", 18, sf::Color(210, 210, 210));
    hint.setPosition({1085.0f, 670.0f});
    window.draw(hint);
}

bool Game::loadShopResources()
{
    if (!shopView.loadMerchantAnimation(kMerchantFramesPath))
    {
        lastError = std::string("无法加载商人动画: ") + kMerchantFramesPath;
        return false;
    }

    return true;
}

bool Game::loadRestResources()
{
    restBackgroundLoaded = restBackgroundTexture.loadFromFile(kRestBackgroundPath);
    if (!restBackgroundLoaded)
    {
        lastError = std::string("无法加载篝火背景: ") + kRestBackgroundPath;
        return false;
    }

    return true;
}

bool Game::playMusic(const std::string& path, bool looping)
{
    if (path.empty())
    {
        stopMusic();
        return true;
    }

    backgroundMusic.stop();
    if (!backgroundMusic.openFromFile(path))
    {
        lastError = "无法加载背景音乐: " + path;
        return false;
    }

    backgroundMusic.setLooping(looping);
    backgroundMusic.setVolume(100.0f);
    backgroundMusic.play();
    return true;
}

void Game::stopMusic()
{
    backgroundMusic.stop();
}

std::vector<Card> Game::buildCombatDeck() const
{
    std::vector<Card> result;
    result.reserve(state.deck.size());
    for (const CardInstance& instance : state.deck)
    {
        try
        {
            result.push_back(CardDatabase::createFromInstance(instance));
        }
        catch (const std::invalid_argument&)
        {
        }
    }

    return result.empty() ? CardDatabase::createStarterDeck() : result;
}

bool Game::isMapNodeSelectable(const MapNode& node) const
{
    if (state.currentNodeId == -1)
    {
        return node.row == 0;
    }

    const std::optional<MapNode> currentNode = findNodeById(mapNodes, state.currentNodeId);
    if (!currentNode.has_value())
    {
        return node.row == 0;
    }

    return std::find(currentNode->nextNodeIds.begin(), currentNode->nextNodeIds.end(),
                     node.id) != currentNode->nextNodeIds.end();
}

std::vector<Game::MapNodeButton> Game::layoutMapNodes() const
{
    std::vector<MapNodeButton> buttons;
    if (mapNodes.empty())
    {
        return buttons;
    }

    int maxRow = 0;
    for (const MapNode& node : mapNodes)
    {
        maxRow = std::max(maxRow, node.row);
    }

    constexpr float worldTop = kMapViewportTop + 50.0f;
    constexpr float rowGap = 150.0f;

    for (const MapNode& node : mapNodes)
    {
        int rowNodeCount = 0;
        for (const MapNode& other : mapNodes)
        {
            if (other.row == node.row)
            {
                ++rowNodeCount;
            }
        }

        const float spacing = rowNodeCount > 1
            ? std::min(kMapNodeSpacingX, (kMapContentWidth - 160.0f) / (rowNodeCount - 1))
            : 0.0f;
        const float totalWidth = static_cast<float>(rowNodeCount - 1) * spacing;
        const float x = kMapContentWidth / 2.0f - totalWidth / 2.0f +
                        static_cast<float>(node.column) * spacing - kMapNodeSize / 2.0f;
        const float y = worldTop + static_cast<float>(maxRow - node.row) * rowGap -
                        kMapNodeSize / 2.0f;
        buttons.push_back({node.id, sf::FloatRect({x, y}, {kMapNodeSize, kMapNodeSize})});
    }

    return buttons;
}

float Game::getMaxMapScrollOffset() const
{
    if (mapNodes.empty())
    {
        return 0.0f;
    }

    int maxRow = 0;
    for (const MapNode& node : mapNodes)
    {
        maxRow = std::max(maxRow, node.row);
    }

    const float worldBottom = kMapViewportTop + 50.0f + static_cast<float>(maxRow) * 150.0f +
                              kMapNodeSize / 2.0f;
    return std::max(0.0f, worldBottom - kMapViewportBottom);
}

sf::Text Game::makeText(const std::string& text, unsigned int size,
                        sf::Color color) const
{
    sf::Text drawableText(font, toSfString(text), size);
    drawableText.setFillColor(color);
    return drawableText;
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
