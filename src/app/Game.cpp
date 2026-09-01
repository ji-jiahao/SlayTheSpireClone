#include "app/Game.hpp"

#include <SFML/Window.hpp>

#include <iostream>

namespace
{
const char* kDemoEventId = "sacred_nailong";
const char* kFontPath = "assets/fonts/simhei.ttf";
const char* kEventDataPath = "assets/data/events.json";

sf::String toSfString(const std::string& text)
{
    return sf::String::fromUtf8(text.begin(), text.end());
}
} // namespace

Game::Game()
    : window_(sf::VideoMode({1280, 720}), "SlayTheSpireClone - 事件系统"),
      eventSystem_(eventDatabase_),
      scene_(SceneType::Event),
      ready_(true)
{
    window_.setFramerateLimit(60);

    if (!font_.openFromFile(kFontPath))
    {
        ready_ = false;
        lastError_ = std::string("无法加载字体: ") + kFontPath;
        std::cerr << lastError_ << std::endl;
        return;
    }

    if (!eventView_.loadFont(kFontPath))
    {
        ready_ = false;
        lastError_ = eventView_.getLastError();
        std::cerr << lastError_ << std::endl;
        return;
    }

    if (!eventDatabase_.loadFromFile(kEventDataPath))
    {
        ready_ = false;
        lastError_ = eventDatabase_.getLastError();
        std::cerr << "加载事件数据库失败: " << lastError_ << std::endl;
        return;
    }

    startEventIfAvailable();
}

void Game::run()
{
    if (!ready_)
    {
        return;
    }

    while (window_.isOpen())
    {
        handleEvents();
        update(clock_.restart().asSeconds());
        render();
    }
}

void Game::handleEvents()
{
    while (const auto event = window_.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
        {
            window_.close();
            break;
        }

        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
        {
            if (keyPressed->code == sf::Keyboard::Key::Escape)
            {
                window_.close();
                break;
            }
        }

        if (scene_ != SceneType::Event)
        {
            continue;
        }

        if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved>())
        {
            eventView_.handleMouseMove(
                {static_cast<float>(mouseMoved->position.x),
                 static_cast<float>(mouseMoved->position.y)},
                eventSystem_);
        }

        if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>())
        {
            if (mousePressed->button != sf::Mouse::Button::Left)
            {
                continue;
            }

            eventView_.handleMouseClick(
                {static_cast<float>(mousePressed->position.x),
                 static_cast<float>(mousePressed->position.y)},
                eventSystem_, gameState_);
        }
    }
}

void Game::update(float deltaSeconds)
{
    if (scene_ != SceneType::Event)
    {
        return;
    }

    eventView_.update(deltaSeconds);
    if (!eventView_.shouldReturnToMap())
    {
        return;
    }

    eventView_.clearReturnToMapRequest();
    if (gameState_.isDead())
    {
        showGameOver();
    }
    else
    {
        showMap();
    }
}

void Game::render()
{
    window_.clear();

    switch (scene_)
    {
    case SceneType::Event:
        eventView_.draw(window_, eventSystem_, gameState_);
        break;
    case SceneType::Map:
        drawMapPlaceholder();
        break;
    case SceneType::GameOver:
        drawGameOver();
        break;
    }

    window_.display();
}

void Game::startEventIfAvailable()
{
    if (gameState_.hasVisitedEvent(kDemoEventId))
    {
        showMap();
        return;
    }

    if (!eventSystem_.startEvent(kDemoEventId))
    {
        ready_ = false;
        lastError_ = eventSystem_.getLastError();
        std::cerr << "启动事件失败: " << lastError_ << std::endl;
        return;
    }

    if (!eventView_.prepareEvent(eventSystem_.getCurrentEvent()))
    {
        ready_ = false;
        lastError_ = eventView_.getLastError();
        std::cerr << lastError_ << std::endl;
        return;
    }

    eventView_.enterCurrentState(eventSystem_);
    scene_ = SceneType::Event;
}

void Game::showMap()
{
    scene_ = SceneType::Map;
    window_.setTitle("SlayTheSpireClone - 地图");
}

void Game::showGameOver()
{
    scene_ = SceneType::GameOver;
    window_.setTitle("SlayTheSpireClone - 游戏结束");
}

void Game::drawMapPlaceholder()
{
    sf::RectangleShape background({1280.0f, 720.0f});
    background.setFillColor(sf::Color(28, 31, 37));
    window_.draw(background);

    sf::Text title = makeText("地图", 46, sf::Color(235, 229, 207));
    title.setPosition({80.0f, 70.0f});
    window_.draw(title);

    sf::Text message = makeText("事件已关闭，已返回地图。该事件已记录为访问过，不会重复出现。",
                                24, sf::Color(210, 199, 174));
    message.setPosition({80.0f, 150.0f});
    window_.draw(message);

    sf::Text status = makeText("当前生命: " + std::to_string(gameState_.currentHealth) +
                                   "/" + std::to_string(gameState_.maxHealth) +
                                   "    金币: " + std::to_string(gameState_.gold),
                               24, sf::Color(242, 210, 105));
    status.setPosition({80.0f, 210.0f});
    window_.draw(status);
}

void Game::drawGameOver()
{
    sf::RectangleShape background({1280.0f, 720.0f});
    background.setFillColor(sf::Color(34, 18, 20));
    window_.draw(background);

    sf::Text title = makeText("游戏结束", 54, sf::Color(238, 221, 210));
    title.setPosition({80.0f, 80.0f});
    window_.draw(title);

    sf::Text message = makeText("生命值降为 0，已进入正常死亡流程占位界面。",
                                26, sf::Color(222, 183, 166));
    message.setPosition({80.0f, 170.0f});
    window_.draw(message);
}

sf::Text Game::makeText(const std::string& text, unsigned int size,
                        sf::Color color) const
{
    sf::Text drawableText(font_, toSfString(text), size);
    drawableText.setFillColor(color);
    return drawableText;
}
