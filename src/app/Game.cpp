#include "app/Game.hpp"

#include "card/CardDatabase.hpp"
#include "ui/CardView.hpp"
#include "ui/UiHelpers.hpp"

#include <array>
#include <filesystem>
#include <stdexcept>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace
{
constexpr unsigned int kWindowWidth = 1280;
constexpr unsigned int kWindowHeight = 720;

std::filesystem::path executableDirectory()
{
#ifdef _WIN32
    std::wstring buffer(32768, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(),
                                            static_cast<DWORD>(buffer.size()));
    if (length > 0 && length < buffer.size())
    {
        buffer.resize(length);
        return std::filesystem::path(buffer).parent_path();
    }
#endif
    return std::filesystem::current_path();
}

bool loadFontFromCandidates(sf::Font& font)
{
    const std::array<std::filesystem::path, 4> candidates = {
        executableDirectory() / "assets/fonts/simhei.ttf",
        std::filesystem::path("assets/fonts/simhei.ttf"),
        std::filesystem::path("Debug/assets/fonts/simhei.ttf"),
        std::filesystem::path("../assets/fonts/simhei.ttf")};
    for (const auto& path : candidates)
    {
        if (font.openFromFile(path)) return true;
    }
    return false;
}

bool loadTextureFromCandidates(sf::Texture& texture)
{
    const std::array<std::filesystem::path, 8> candidates = {
        executableDirectory() / "assets/images/backgrounds/dungeon.png",
        executableDirectory() / "assets/images/event/dungeon_background.png",
        std::filesystem::path("assets/images/backgrounds/dungeon.png"),
        std::filesystem::path("assets/images/event/dungeon_background.png"),
        std::filesystem::path("Debug/assets/images/backgrounds/dungeon.png"),
        std::filesystem::path("Debug/assets/images/event/dungeon_background.png"),
        std::filesystem::path("../assets/images/backgrounds/dungeon.png"),
        std::filesystem::path("../assets/images/event/dungeon_background.png")};
    for (const auto& path : candidates)
    {
        if (texture.loadFromFile(path)) return true;
    }
    return false;
}
}

Game::Game()
    : window_(sf::VideoMode({kWindowWidth, kWindowHeight}), "Spire Road - Act One")
{
    window_.setFramerateLimit(60);
    fontLoaded_ = loadResources();
}

void Game::run()
{
    while (window_.isOpen())
    {
        while (const auto event = window_.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window_.close();
            }
            else if (const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>())
            {
                if (mouse->button == sf::Mouse::Button::Left)
                {
                    const sf::Vector2f position = window_.mapPixelToCoords(mouse->position);
                    if (scene_ == SceneType::Map &&
                        !(position.x >= 1035.0f && position.x <= 1225.0f &&
                          position.y >= 12.0f && position.y <= 58.0f))
                    {
                        mapView_.beginPointer(position);
                    }
                    else handleClick(position);
                }
            }
            else if (const auto* mouse = event->getIf<sf::Event::MouseMoved>())
            {
                if (scene_ == SceneType::Map)
                {
                    mapView_.updatePointer(window_.mapPixelToCoords(mouse->position));
                }
            }
            else if (const auto* mouse = event->getIf<sf::Event::MouseButtonReleased>())
            {
                if (scene_ == SceneType::Map && mouse->button == sf::Mouse::Button::Left)
                {
                    const int nodeId = mapView_.endPointer(
                        window_.mapPixelToCoords(mouse->position), mapState_);
                    if (nodeId >= 0) enterMapNode(nodeId);
                }
            }
            else if (const auto* wheel = event->getIf<sf::Event::MouseWheelScrolled>())
            {
                if (scene_ == SceneType::Map) mapView_.scroll(wheel->delta);
            }
            else if (const auto* key = event->getIf<sf::Event::KeyPressed>())
            {
                if (key->code == sf::Keyboard::Key::Escape && scene_ == SceneType::Map)
                {
                    returnToMenu();
                }
            }
        }

        if (scene_ == SceneType::Battle)
        {
            combat_.update();
            handleBattleResult();
        }
        draw();
    }
}

void Game::handleClick(sf::Vector2f position)
{
    if (scene_ == SceneType::MainMenu)
    {
        handleMenuClick(position);
        return;
    }
    if (scene_ == SceneType::DeckView)
    {
        handleDeckClick(position);
        return;
    }
    if (position.x >= 1035.0f && position.x <= 1225.0f &&
        position.y >= 12.0f && position.y <= 58.0f)
    {
        deckReturnScene_ = scene_;
        deckUpgradeMode_ = false;
        scene_ = SceneType::DeckView;
        return;
    }
    else if (scene_ == SceneType::Battle)
    {
        battleView_.handleMouseClick(position, combat_);
        handleBattleResult();
    }
    else
    {
        handleRoomClick(position);
    }
}

void Game::handleMenuClick(sf::Vector2f position)
{
    switch (mainMenuView_.handleMouseClick(position))
    {
    case MainMenuView::Action::Start:
        startNewRun();
        break;
    case MainMenuView::Action::Quit:
        window_.close();
        break;
    case MainMenuView::Action::None:
        break;
    }
}

void Game::handleRoomClick(sf::Vector2f position)
{
    const int action = roomView_.handleMouseClick(position, currentActions());
    if (action >= 0) handleRoomAction(action);
}

void Game::handleRoomAction(int actionIndex)
{
    switch (scene_)
    {
    case SceneType::Intro:
        if (actionIndex == 0) state_.gainGold(50);
        if (actionIndex == 1) relicSystem_.obtainRelic(state_, "anchor");
        scene_ = SceneType::Map;
        break;
    case SceneType::CardReward:
        if (actionIndex >= 0 && actionIndex < static_cast<int>(rewardCards_.size()))
        {
            state_.addCard(rewardCards_[static_cast<std::size_t>(actionIndex)].id);
        }
        finishCurrentNode();
        break;
    case SceneType::Event:
        if (actionIndex == 0)
        {
            state_.loseHealth(6);
            state_.gainGold(75);
        }
        else if (actionIndex == 1)
        {
            state_.heal(12);
        }
        if (state_.currentHealth <= 0)
        {
            scene_ = SceneType::GameOver;
            roomEyebrow_ = "挑战失败";
            roomTitle_ = "你倒在了事件房间";
            roomDescription_ = "尖塔不会宽恕鲁莽的选择。";
        }
        else
        {
            finishCurrentNode();
        }
        break;
    case SceneType::Rest:
        if (actionIndex == 0) state_.heal(state_.maxHealth * 3 / 10);
        if (actionIndex == 1)
        {
            deckReturnScene_ = SceneType::Rest;
            deckUpgradeMode_ = true;
            scene_ = SceneType::DeckView;
            return;
        }
        finishCurrentNode();
        break;
    case SceneType::Shop:
        if (actionIndex == 0 && state_.spendGold(50)) state_.addCard("iron_wave");
        if (actionIndex == 1 && state_.spendGold(75)) state_.removeFirstCard("strike");
        finishCurrentNode();
        break;
    case SceneType::Treasure:
        finishCurrentNode();
        break;
    case SceneType::ActResult:
    case SceneType::GameOver:
        if (actionIndex == 0) startNewRun();
        else if (actionIndex == 1) returnToMenu();
        else window_.close();
        break;
    case SceneType::DeckView:
        break;
    default:
        break;
    }
}

void Game::draw()
{
    window_.clear(sf::Color(20, 20, 23));
    switch (scene_)
    {
    case SceneType::MainMenu:
        mainMenuView_.draw(window_);
        break;
    case SceneType::Map:
        mapView_.draw(window_, mapState_, state_);
        break;
    case SceneType::Battle:
        battleView_.draw(window_, combat_);
        break;
    case SceneType::DeckView:
        drawDeckView();
        break;
    default:
        drawRoom();
        break;
    }
    if (scene_ != SceneType::MainMenu && scene_ != SceneType::DeckView)
    {
        drawDeckButton();
    }
    window_.display();
}

void Game::drawRoom()
{
    roomView_.draw(window_, state_, roomEyebrow_, roomTitle_, roomDescription_, currentActions());
}

void Game::startNewRun()
{
    state_.reset();
    mapState_.reset(mapGenerator_.generateActOne(state_.seed));
    mapView_.resetScroll();
    battleNumber_ = 0;
    lastRelicHealing_ = 0;
    roomEyebrow_ = "来自鲸鱼的低语";
    roomTitle_ = "又一位挑战者";
    roomDescription_ = "在踏入第一幕前，选择一份启程赠礼。";
    scene_ = SceneType::Intro;
}

void Game::enterMapNode(int nodeId)
{
    if (!mapState_.chooseNode(nodeId)) return;
    const MapNode* node = mapState_.getCurrentNode();
    if (node == nullptr) return;

    switch (node->type)
    {
    case MapNodeType::Battle:
    case MapNodeType::Elite:
    case MapNodeType::Boss:
        startCurrentBattle();
        break;
    case MapNodeType::Unknown:
        roomEyebrow_ = "未知房间";
        roomTitle_ = "被遗忘的祭坛";
        roomDescription_ = "石台上散落着金币，旁边的泉水仍泛着微光。";
        scene_ = SceneType::Event;
        break;
    case MapNodeType::Rest:
        roomEyebrow_ = "篝火";
        roomTitle_ = "片刻喘息";
        roomDescription_ = "火焰温暖而安静。你只能选择一项行动。";
        scene_ = SceneType::Rest;
        break;
    case MapNodeType::Shop:
        roomEyebrow_ = "商店";
        roomTitle_ = "商人的货架";
        roomDescription_ = "花费金币强化牌组，或直接离开。";
        scene_ = SceneType::Shop;
        break;
    case MapNodeType::Treasure:
        if (state_.relicIds.size() % 2 == 1) relicSystem_.obtainRelic(state_, "vajra");
        else relicSystem_.obtainRelic(state_, "lantern");
        roomEyebrow_ = "宝箱";
        roomTitle_ = state_.relicIds.back() == "vajra" ? "获得遗物：金刚杵" : "获得遗物：灯笼";
        roomDescription_ = state_.relicIds.back() == "vajra"
                               ? "之后的战斗开始时获得 1 点力量。"
                               : "之后每场战斗的第一回合获得 1 点能量。";
        scene_ = SceneType::Treasure;
        break;
    }
}

void Game::startCurrentBattle()
{
    const MapNode* node = mapState_.getCurrentNode();
    if (node == nullptr) return;
    ++battleNumber_;
    const int floor = node->row + 1;
    EncounterDefinition encounter;
    if (node->type == MapNodeType::Elite)
    {
        encounter = {"乐加维林", 55 + floor, 10};
    }
    else if (node->type == MapNodeType::Boss)
    {
        encounter = {"史莱姆老大", 85, 12};
    }
    else
    {
        const std::array<std::string, 4> names{"邪教徒", "下颚虫", "酸液史莱姆", "真菌兽"};
        encounter = {names[static_cast<std::size_t>(battleNumber_ % names.size())],
                     30 + floor * 2, 5 + floor / 4};
    }

    relicSystem_.beginBattle();
    const RelicBattleStartModifiers modifiers = relicSystem_.applyBattleStart(state_);
    combat_.startBattle(state_.currentHealth, state_.seed + battleNumber_, buildCombatDeck(),
                        encounter, modifiers.block, modifiers.strength, modifiers.energy,
                        modifiers.drawCards, state_.maxHealth);
    scene_ = SceneType::Battle;
}

void Game::finishCurrentNode()
{
    mapState_.completeCurrentNode();
    if (mapState_.isActComplete())
    {
        roomEyebrow_ = "第一幕完成";
        roomTitle_ = "你穿过了高塔的底层";
        roomDescription_ = "生命、金币、牌组和遗物均已保留。第一幕旅程至此结算。";
        scene_ = SceneType::ActResult;
    }
    else
    {
        scene_ = SceneType::Map;
    }
}

void Game::handleBattleResult()
{
    if (combat_.getResult() == BattleResult::Active) return;
    if (combat_.getResult() == BattleResult::Defeat)
    {
        state_.currentHealth = 0;
        roomEyebrow_ = "挑战失败";
        roomTitle_ = "铁甲战士倒下了";
        roomDescription_ = "你可以立即开始新游戏，或返回主菜单。";
        scene_ = SceneType::GameOver;
        return;
    }

    state_.currentHealth = combat_.getPlayer().getCurrentHealth();
    lastRelicHealing_ = relicSystem_.applyBattleVictory(state_);
    const MapNode* node = mapState_.getCurrentNode();
    const int goldReward = node != nullptr && node->type == MapNodeType::Elite ? 35 : 20;
    state_.gainGold(goldReward);
    prepareCardReward();
    roomEyebrow_ = "战斗胜利";
    roomTitle_ = "选择一张卡牌奖励";
    roomDescription_ = "获得 " + std::to_string(goldReward) + " 金币。燃烧之血回复 " +
                       std::to_string(lastRelicHealing_) + " 点生命。";
    scene_ = SceneType::CardReward;
}

void Game::prepareCardReward()
{
    static const std::array<std::array<const char*, 3>, 3> choices{{
        {{"cleave", "iron_wave", "shrug_it_off"}},
        {{"pommel_strike", "twin_strike", "clothesline"}},
        {{"anger", "true_grit", "flex"}}}};
    const auto& ids = choices[static_cast<std::size_t>(battleNumber_ % choices.size())];
    rewardCards_.clear();
    for (const char* id : ids) rewardCards_.push_back(CardDatabase::createById(id));
}

void Game::returnToMenu()
{
    scene_ = SceneType::MainMenu;
}

void Game::drawDeckView()
{
    sf::RectangleShape background({1280.0f, 720.0f});
    background.setFillColor(sf::Color(24, 25, 29));
    window_.draw(background);

    sf::RectangleShape header({1280.0f, 78.0f});
    header.setFillColor(sf::Color(39, 37, 40));
    window_.draw(header);

    if (fontLoaded_)
    {
        UiHelpers::drawText(window_, font_, deckUpgradeMode_ ? "选择一张牌进行升级" : "牌组", 32,
                            {42.0f, 20.0f}, sf::Color(239, 211, 151));
        UiHelpers::drawText(window_, font_, std::to_string(state_.deck.size()) + " 张牌", 20,
                            {270.0f, 27.0f}, sf::Color(201, 195, 184));
        UiHelpers::drawButton(window_, font_, {{1035.0f, 12.0f}, {190.0f, 46.0f}},
                              deckUpgradeMode_ ? "返回篝火" : "返回", true, false);
    }

    constexpr std::size_t kColumns = 5;
    const float cardWidth = CardView::getCardSize().x;
    const float cardHeight = CardView::getCardSize().y;
    const float gap = 18.0f;
    const float totalWidth = kColumns * cardWidth + (kColumns - 1) * gap;
    const float startX = (1280.0f - totalWidth) / 2.0f;
    for (std::size_t index = 0; index < state_.deck.size(); ++index)
    {
        Card card;
        try
        {
            card = CardDatabase::createFromInstance(state_.deck[index]);
        }
        catch (const std::invalid_argument&)
        {
            continue;
        }
        CardView view;
        if (fontLoaded_) view.setFont(font_);
        const std::size_t row = index / kColumns;
        const std::size_t column = index % kColumns;
        view.setPosition({startX + column * (cardWidth + gap),
                          105.0f + row * (cardHeight + 22.0f)});
        view.draw(window_, card);

        if (deckUpgradeMode_ && state_.deck[index].upgraded)
        {
            sf::RectangleShape disabled({cardWidth, cardHeight});
            disabled.setPosition({startX + column * (cardWidth + gap),
                                  105.0f + row * (cardHeight + 22.0f)});
            disabled.setFillColor(sf::Color(30, 30, 30, 135));
            window_.draw(disabled);
            if (fontLoaded_)
            {
                UiHelpers::drawCenteredText(window_, font_, "已升级", 18,
                                            disabled.getGlobalBounds(), sf::Color(235, 220, 180));
            }
        }
    }
}

void Game::drawDeckButton()
{
    if (!fontLoaded_) return;
    UiHelpers::drawButton(window_, font_, {{1035.0f, 12.0f}, {190.0f, 46.0f}},
                          "牌组 " + std::to_string(state_.deck.size()), true, false);
}

void Game::handleDeckClick(sf::Vector2f position)
{
    if (position.x >= 1035.0f && position.x <= 1225.0f &&
        position.y >= 12.0f && position.y <= 58.0f)
    {
        scene_ = deckReturnScene_;
        deckUpgradeMode_ = false;
        return;
    }

    if (!deckUpgradeMode_) return;
    const float cardWidth = CardView::getCardSize().x;
    const float cardHeight = CardView::getCardSize().y;
    const float gap = 18.0f;
    const std::size_t columns = 5;
    const float totalWidth = columns * cardWidth + (columns - 1) * gap;
    const float startX = (1280.0f - totalWidth) / 2.0f;
    for (std::size_t index = 0; index < state_.deck.size(); ++index)
    {
        const std::size_t row = index / columns;
        const std::size_t column = index % columns;
        const sf::FloatRect bounds{{startX + column * (cardWidth + gap),
                                    105.0f + row * (cardHeight + 22.0f)},
                                   {cardWidth, cardHeight}};
        if (bounds.contains(position) && state_.upgradeCardAt(index))
        {
            deckUpgradeMode_ = false;
            finishCurrentNode();
            return;
        }
    }
}

std::vector<Card> Game::buildCombatDeck() const
{
    std::vector<Card> result;
    result.reserve(state_.deck.size());
    for (const CardInstance& instance : state_.deck)
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

std::vector<RoomAction> Game::currentActions() const
{
    switch (scene_)
    {
    case SceneType::Intro:
        return {{"获得 50 金币", "从 99 金币开始变为 149"},
                {"获得遗物：锚", "每场战斗开始获得 10 格挡"}};
    case SceneType::CardReward:
    {
        std::vector<RoomAction> actions;
        for (const Card& card : rewardCards_) actions.push_back({card.name, card.description});
        actions.push_back({"跳过", "不添加卡牌"});
        return actions;
    }
    case SceneType::Event:
        return {{"献上 6 点生命", "获得 75 金币"},
                {"饮用泉水", "回复 12 点生命"}, {"离开", "不发生任何事"}};
    case SceneType::Rest:
        return {{"休息", "回复最大生命的 30%"},
                {"锻造", "从牌库中选择一张未升级的牌"}};
    case SceneType::Shop:
        return {{"购买铁斩波", "50 金币", state_.gold >= 50},
                {"移除一张打击", "75 金币", state_.gold >= 75},
                {"离开", "保留金币"}};
    case SceneType::Treasure:
        return {{"收下遗物并返回地图", "遗物效果将在后续战斗生效"}};
    case SceneType::ActResult:
        return {{"再来一局", "重新生成第一幕"}, {"返回主菜单", ""}, {"结束游戏", ""}};
    case SceneType::GameOver:
        return {{"重新开始", "建立新的第一幕存档"}, {"返回主菜单", ""}, {"结束游戏", ""}};
    default:
        return {};
    }
}

bool Game::loadResources()
{
    const bool fontLoaded = loadFontFromCandidates(font_);
    const bool textureLoaded = loadTextureFromCandidates(dungeonTexture_);
    if (fontLoaded)
    {
        mainMenuView_.setFont(font_);
        mapView_.setFont(font_);
        battleView_.setFont(font_);
        roomView_.setFont(font_);
    }
    if (textureLoaded)
    {
        mainMenuView_.setBackground(dungeonTexture_);
        battleView_.setBackground(dungeonTexture_);
        roomView_.setBackground(dungeonTexture_);
    }
    // 没有背景图时各视图会使用纯色背景，不能影响文字和卡牌内容显示。
    return fontLoaded;
}
