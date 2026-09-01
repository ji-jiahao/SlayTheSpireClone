#include "app/Game.hpp"

#include "map/MapGenerator.hpp"

#include <array>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <optional>

namespace
{
constexpr unsigned int kWindowWidth = 1280;
constexpr unsigned int kWindowHeight = 720;
constexpr const char* kEventDataPath = "assets/data/events.json";
constexpr const char* kUniversityEventId = "university_choice";
constexpr const char* kNailongEventId = "sacred_nailong";

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
        return "精英";
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
        return sf::Color(185, 94, 190);
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

sf::Color mapLineColor(bool active)
{
    if (active)
    {
        return sf::Color(225, 198, 118);
    }

    return sf::Color(82, 86, 94);
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
} // namespace

Game::Game()
    : window(sf::VideoMode({kWindowWidth, kWindowHeight}), "Slay the Spire Clone"),
      fontLoaded(false),
      eventSystem(eventDatabase),
      scene(SceneType::Map),
      handledResult(BattleResult::Active),
      relicHealing(0)
{
    fontLoaded = loadFont();
    window.setFramerateLimit(60);

    if (fontLoaded)
    {
        battleView.setFont(font);
    }

    if (!eventView.loadFont("assets/fonts/simhei.ttf"))
    {
        lastError = eventView.getLastError();
        std::cerr << lastError << std::endl;
    }

    if (!eventDatabase.loadFromFile(kEventDataPath))
    {
        lastError = "加载事件数据库失败: " + eventDatabase.getLastError();
        std::cerr << lastError << std::endl;
    }

    mapIconsLoaded = loadMapIconTextures();
    if (!mapIconsLoaded)
    {
        std::cerr << lastError << std::endl;
    }

    MapGenerator generator;
    mapNodes = generator.generateMap(6);
    if (!mapNodes.empty())
    {
        mapNodes.front().type = MapNodeType::Event;
    }
    if (mapNodes.size() > 1)
    {
        mapNodes[1].type = MapNodeType::Battle;
    }
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

    if (const auto* key = event.getIf<sf::Event::KeyPressed>())
    {
        if (scene == SceneType::Event && eventView.handleAnyInput(eventSystem, state))
        {
            return;
        }

        if (key->code == sf::Keyboard::Key::Escape)
        {
            window.close();
            return;
        }

        if (scene == SceneType::Battle && key->code == sf::Keyboard::Key::R &&
            combat.getResult() != BattleResult::Active)
        {
            if (combat.getResult() == BattleResult::Defeat)
            {
                state.currentHealth = state.maxHealth;
            }
            showMap();
            return;
        }
    }

    if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>())
    {
        if (scene == SceneType::Event)
        {
            eventView.handleMouseMove(window.mapPixelToCoords(mouseMoved->position),
                                      eventSystem);
        }
        return;
    }

    if (const auto* mouse = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mouse->button != sf::Mouse::Button::Left)
        {
            return;
        }

        const sf::Vector2f mousePosition = window.mapPixelToCoords(mouse->position);
        if (scene == SceneType::Map)
        {
            handleMapMouseClick(mousePosition);
        }
        else if (scene == SceneType::Event)
        {
            eventView.handleMouseClick(mousePosition, eventSystem, state);
        }
        else if (scene == SceneType::Battle &&
                 combat.getResult() == BattleResult::Active)
        {
            battleView.handleMouseClick(mousePosition, combat);
            handleBattleResult();
        }
    }
}

void Game::handleMapMouseClick(sf::Vector2f mousePosition)
{
    for (const MapNodeButton& button : layoutMapNodes())
    {
        if (!button.bounds.contains(mousePosition))
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

        if (node->type == MapNodeType::Battle || node->type == MapNodeType::Elite ||
            node->type == MapNodeType::Boss)
        {
            startBattle();
            return;
        }

        statusMessage = mapNodeTypeName(node->type) + "节点暂未接入，已沿当前道路前进。";
        return;
    }
}

void Game::update(float deltaSeconds)
{
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
        combat.update();
        handleBattleResult();
    }
}

void Game::render()
{
    window.clear(sf::Color(35, 38, 42));

    switch (scene)
    {
    case SceneType::Map:
        drawMapScene();
        break;
    case SceneType::Event:
        eventView.draw(window, eventSystem, state);
        break;
    case SceneType::Battle:
        battleView.draw(window, combat);
        drawResultOverlay();
        break;
    case SceneType::GameOver:
        drawGameOver();
        break;
    }

    window.display();
}

void Game::startBattle()
{
    relicSystem.beginBattle();
    handledResult = BattleResult::Active;
    relicHealing = 0;
    statusMessage.clear();
    combat.startBattle(state.currentHealth);
    scene = SceneType::Battle;
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
    scene = SceneType::Event;
    window.setTitle("Slay the Spire Clone - 事件");
    return true;
}

void Game::showMap()
{
    scene = SceneType::Map;
    statusMessage = "已返回地图。";
    window.setTitle("Slay the Spire Clone - 地图");
}

void Game::showGameOver()
{
    scene = SceneType::GameOver;
    window.setTitle("Slay the Spire Clone - 游戏结束");
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
        state.currentHealth = 0;
        statusMessage = "战斗失败";
    }
}

void Game::drawMapScene()
{
    sf::RectangleShape background({static_cast<float>(kWindowWidth),
                                   static_cast<float>(kWindowHeight)});
    background.setFillColor(sf::Color(28, 31, 37));
    window.draw(background);

    if (!fontLoaded)
    {
        return;
    }

    sf::Text title = makeText("地图", 42, sf::Color(235, 229, 207));
    title.setPosition({70.0f, 48.0f});
    window.draw(title);

    sf::Text hint = makeText("从最底层开始选择路线，之后只能沿连线向上前进。",
                             22, sf::Color(210, 199, 174));
    hint.setPosition({70.0f, 108.0f});
    window.draw(hint);

    sf::Text status = makeText("当前生命: " + std::to_string(state.currentHealth) +
                                   "/" + std::to_string(state.maxHealth) +
                                   "    金币: " + std::to_string(state.gold),
                               24, sf::Color(242, 210, 105));
    status.setPosition({70.0f, 154.0f});
    window.draw(status);

    if (!statusMessage.empty())
    {
        sf::Text message = makeText(statusMessage, 20, sf::Color(224, 218, 200));
        message.setPosition({70.0f, 196.0f});
        window.draw(message);
    }

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
            const bool activeLine = node.id == state.currentNodeId ||
                                    state.currentNodeId == -1 ||
                                    std::find(node.nextNodeIds.begin(), node.nextNodeIds.end(),
                                              state.currentNodeId) != node.nextNodeIds.end();
            sf::VertexArray line(sf::PrimitiveType::Lines, 2);
            line[0].position = center;
            line[0].color = mapLineColor(activeLine);
            line[1].position = targetCenter;
            line[1].color = mapLineColor(activeLine);
            window.draw(line);
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
        const sf::Texture* texture = getMapNodeTexture(node->type);
        if (texture != nullptr)
        {
            sf::Sprite icon(*texture);
            const sf::FloatRect localBounds = icon.getLocalBounds();
            const float scale = std::min(button.bounds.size.x / localBounds.size.x,
                                         button.bounds.size.y / localBounds.size.y);
            icon.setScale({scale, scale});
            icon.setPosition({button.bounds.position.x +
                                  (button.bounds.size.x - localBounds.size.x * scale) / 2.0f,
                              button.bounds.position.y +
                                  (button.bounds.size.y - localBounds.size.y * scale) / 2.0f});
            icon.setColor(sf::Color(255, 255, 255,
                                    static_cast<std::uint8_t>(selectable || selected ? 255 : 95)));
            window.draw(icon);
        }
        else
        {
            sf::CircleShape circle(button.bounds.size.x / 2.0f, 48);
            circle.setPosition(button.bounds.position);
            circle.setFillColor(mapNodeColor(node->type));
            circle.setOutlineThickness(selected ? 5.0f : 3.0f);
            circle.setOutlineColor(sf::Color(238, 221, 170));
            window.draw(circle);
        }

        if (selectable || selected)
        {
            sf::CircleShape outline(button.bounds.size.x / 2.0f, 48);
            outline.setPosition(button.bounds.position);
            outline.setFillColor(sf::Color::Transparent);
            outline.setOutlineThickness(selected ? 5.0f : 3.0f);
            outline.setOutlineColor(selected ? sf::Color(246, 216, 114)
                                             : sf::Color(224, 221, 205));
            window.draw(outline);
        }

        sf::Text label = makeText(mapNodeTypeName(node->type), 16,
                                  selectable || selected ? sf::Color(246, 242, 226)
                                                         : sf::Color(128, 128, 128));
        const sf::FloatRect bounds = label.getLocalBounds();
        label.setPosition({button.bounds.position.x +
                               (button.bounds.size.x - bounds.size.x) / 2.0f -
                               bounds.position.x,
                           button.bounds.position.y +
                               button.bounds.size.y + 8.0f});
        label.setOutlineThickness(2.0f);
        label.setOutlineColor(sf::Color(18, 18, 18, 210));
        window.draw(label);
    }
}

void Game::drawResultOverlay()
{
    if (combat.getResult() == BattleResult::Active)
    {
        return;
    }

    sf::RectangleShape overlay({560.0f, 230.0f});
    overlay.setPosition({360.0f, 220.0f});
    overlay.setFillColor(sf::Color(22, 24, 28, 235));
    overlay.setOutlineColor(sf::Color(230, 174, 72));
    overlay.setOutlineThickness(3.0f);
    window.draw(overlay);

    if (!fontLoaded)
    {
        return;
    }

    sf::Text title = makeText(statusMessage, 40, sf::Color(245, 220, 150));
    title.setPosition({510.0f, 250.0f});
    window.draw(title);

    std::string detail;
    if (combat.getResult() == BattleResult::Victory)
    {
        detail = "燃烧之血回复 " + std::to_string(relicHealing) +
                 " 点生命  当前生命 " + std::to_string(state.currentHealth) +
                 "/" + std::to_string(state.maxHealth);
    }
    else
    {
        detail = "本次挑战结束";
    }

    sf::Text detailText = makeText(detail, 22, sf::Color(235, 229, 207));
    detailText.setPosition({430.0f, 325.0f});
    window.draw(detailText);

    sf::Text restart = makeText("按 R 返回地图", 22, sf::Color(210, 210, 210));
    restart.setPosition({545.0f, 390.0f});
    window.draw(restart);
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

bool Game::loadMapIconTextures()
{
    struct TextureLoadItem
    {
        sf::Texture* texture;
        const char* filePath;
    };

    const std::array<TextureLoadItem, 5> items = {{
        {&battleNodeTexture, "assets/images/map/node_normal.png"},
        {&eliteNodeTexture, "assets/images/map/node_elite.png"},
        {&restNodeTexture, "assets/images/map/node_rest.png"},
        {&shopNodeTexture, "assets/images/map/node_shop.png"},
        {&eventNodeTexture, "assets/images/map/node_event.png"},
    }};

    for (const TextureLoadItem& item : items)
    {
        if (!item.texture->loadFromFile(item.filePath))
        {
            lastError = std::string("无法加载地图图标: ") + item.filePath;
            return false;
        }
    }

    return true;
}

const sf::Texture* Game::getMapNodeTexture(MapNodeType type) const
{
    if (!mapIconsLoaded)
    {
        return nullptr;
    }

    switch (type)
    {
    case MapNodeType::Battle:
        return &battleNodeTexture;
    case MapNodeType::Elite:
    case MapNodeType::Boss:
        return &eliteNodeTexture;
    case MapNodeType::Rest:
        return &restNodeTexture;
    case MapNodeType::Shop:
        return &shopNodeTexture;
    case MapNodeType::Event:
        return &eventNodeTexture;
    }

    return nullptr;
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

    constexpr float nodeSize = 86.0f;
    constexpr float top = 260.0f;
    constexpr float bottom = 640.0f;
    const float rowGap = maxRow == 0 ? 0.0f : (bottom - top) / static_cast<float>(maxRow);

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

        const float totalWidth = static_cast<float>(rowNodeCount - 1) * 180.0f;
        const float x = static_cast<float>(kWindowWidth) / 2.0f - totalWidth / 2.0f +
                        static_cast<float>(node.column) * 180.0f - nodeSize / 2.0f;
        const float y = bottom - static_cast<float>(node.row) * rowGap - nodeSize / 2.0f;
        buttons.push_back({node.id, sf::FloatRect({x, y}, {nodeSize, nodeSize})});
    }

    return buttons;
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
