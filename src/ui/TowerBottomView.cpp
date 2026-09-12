#include "ui/TowerBottomView.hpp"
#include "ui/UiHelpers.hpp"

#include <algorithm>
#include <cmath>

bool TowerBottomView::loadResources()
{
    const bool loaded = background_.loadFromFile("assets/images/event/torch_stone_event_background.png") &&
                        character_.loadFromFile("assets/images/event/fufu.png");
    character_.setSmooth(true);
    return loaded;
}

void TowerBottomView::reset()
{
    timer_ = 0.0f;
}

void TowerBottomView::update(float deltaSeconds)
{
    if (std::isfinite(deltaSeconds) && deltaSeconds > 0.0f)
        timer_ += deltaSeconds;
}

bool TowerBottomView::isReady() const
{
    return timer_ >= 2.4f;
}

sf::FloatRect TowerBottomView::optionBounds(int index)
{
    return {{735.0f, 334.0f + index * 88.0f}, {455.0f, 68.0f}};
}

int TowerBottomView::choiceAt(sf::Vector2f position) const
{
    if (!isReady()) return -1;
    for (int index = 0; index < 3; ++index)
        if (optionBounds(index).contains(position)) return index;
    return -1;
}

void TowerBottomView::draw(sf::RenderWindow& window, const sf::Font& font) const
{
    sf::Sprite background(background_);
    const auto size = background_.getSize();
    const float scale = std::max(1280.0f / size.x, 720.0f / size.y);
    background.setScale({scale, scale});
    background.setPosition({(1280.0f - size.x * scale) / 2.0f,
                            (720.0f - size.y * scale) / 2.0f});
    window.draw(background);
    sf::RectangleShape shade({1280.0f, 720.0f});
    shade.setFillColor(sf::Color(0, 0, 0, 85));
    window.draw(shade);

    const sf::Color gold(238, 194, 82);
    UiHelpers::drawCenteredText(window, font, "塔底", 36, {{0, 40}, {1280, 60}}, gold);
    const float frameEnds[] = {0.06f, 0.12f, 0.18f, 0.24f, 0.27f, 0.33f,
                               0.39f, 0.45f, 0.51f, 0.57f, 0.63f, 0.69f};
    const float frameTime = std::fmod(timer_, 0.69f);
    int frame = 0;
    while (frame < 11 && frameTime >= frameEnds[frame]) ++frame;
    sf::Sprite character(character_, {{frame % 4 * 600, frame / 4 * 600}, {600, 600}});
    character.setScale({0.88f, 0.88f});
    character.setPosition({172.0f, 142.0f});
    window.draw(character);

    sf::RectangleShape bubble({455.0f, 148.0f});
    bubble.setPosition({735.0f, 156.0f});
    bubble.setFillColor(sf::Color(245, 242, 232));
    bubble.setOutlineColor(gold);
    bubble.setOutlineThickness(2.0f);
    window.draw(bubble);
    sf::ConvexShape tail(3);
    tail.setPoint(0, {735.0f, 210.0f});
    tail.setPoint(1, {707.0f, 240.0f});
    tail.setPoint(2, {735.0f, 234.0f});
    tail.setFillColor(sf::Color(245, 242, 232));
    window.draw(tail);
    const auto lines = UiHelpers::wrapText(font,
        "在塔底你遇见了热心的fufu，fufu愿意给你一个Buff。", 26, 395.0f);
    float y = 180.0f;
    for (const auto& line : lines)
    {
        UiHelpers::drawText(window, font, line, 26, {765.0f, y}, sf::Color(32, 28, 24));
        y += 34.0f;
    }
    const char* labels[] = {"每通过一关后恢复8点生命值", "立即获得一张随机卡牌",
                            "每关开始时获得2点敏捷属性加成"};
    const auto mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    for (int index = 0; index < 3; ++index)
        UiHelpers::drawButton(window, font, optionBounds(index), labels[index], true,
                              choiceAt(mouse) == index);
    UiHelpers::drawCenteredText(window, font, "选择一份祝福，开始攀塔", 20,
                                {{735, 602}, {455, 36}}, sf::Color(224, 211, 181));

    if (!isReady())
    {
        const float reveal = std::clamp((timer_ - 1.1f) / 1.3f, 0.0f, 1.0f);
        shade.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(255 * (1 - reveal))));
        window.draw(shade);
        const float titleAlpha = std::min(std::clamp(timer_ / 0.4f, 0.0f, 1.0f), 1 - reveal);
        UiHelpers::drawCenteredText(window, font, "塔底", 64, {{0, 280}, {1280, 160}},
            sf::Color(238, 194, 82, static_cast<std::uint8_t>(255 * titleAlpha)));
    }
}
