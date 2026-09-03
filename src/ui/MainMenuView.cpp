#include "ui/MainMenuView.hpp"

#include "ui/UiHelpers.hpp"

#include <algorithm>
#include <cstdint>

namespace
{
constexpr float kFadeDurationSeconds = 3.0f;

sf::FloatRect makeStartBounds()
{
    return {{500.0f, 496.0f}, {280.0f, 64.0f}};
}

}

void MainMenuView::setFont(const sf::Font& font)
{
    font_ = &font;
}

void MainMenuView::setBackground(const sf::Texture& texture)
{
    background_ = &texture;
}

void MainMenuView::resetFade()
{
    fadeTimer_ = 0.0f;
}

void MainMenuView::update(float deltaSeconds)
{
    fadeTimer_ = std::min(kFadeDurationSeconds, fadeTimer_ + deltaSeconds);
}

void MainMenuView::handleMouseMove(sf::Vector2f position)
{
    if (UiHelpers::contains(startButtonBounds(), position))
    {
        hoveredAction_ = Action::Start;
    }
    else
    {
        hoveredAction_ = Action::None;
    }
}

MainMenuView::Action MainMenuView::handleMouseClick(sf::Vector2f position) const
{
    if (UiHelpers::contains(startButtonBounds(), position))
    {
        return Action::Start;
    }

    return Action::None;
}

void MainMenuView::draw(sf::RenderWindow& window) const
{
    if (background_ != nullptr)
    {
        sf::Sprite background(*background_);
        background.setScale({1280.0f / background_->getSize().x,
                             720.0f / background_->getSize().y});
        window.draw(background);

        const sf::FloatRect hoveredBounds =
            hoveredAction_ == Action::Start ? startButtonBounds() :
            sf::FloatRect();
        if (hoveredAction_ != Action::None)
        {
            sf::RectangleShape highlight(hoveredBounds.size);
            highlight.setPosition(hoveredBounds.position);
            highlight.setFillColor(sf::Color(230, 174, 72, 22));
            highlight.setOutlineColor(sf::Color(245, 220, 150, 170));
            highlight.setOutlineThickness(3.0f);
            window.draw(highlight);
        }
        const float progress = std::clamp(fadeTimer_ / kFadeDurationSeconds, 0.0f, 1.0f);
        const auto alpha = static_cast<std::uint8_t>((1.0f - progress) * 220.0f);
        sf::RectangleShape fadeOverlay({1280.0f, 720.0f});
        fadeOverlay.setFillColor(sf::Color(0, 0, 0, alpha));
        window.draw(fadeOverlay);
        return;
    }

    sf::RectangleShape background({1280.0f, 720.0f});
    background.setFillColor(sf::Color(30, 31, 34));
    window.draw(background);

    sf::RectangleShape veil({1280.0f, 720.0f});
    veil.setFillColor(sf::Color(12, 13, 16, 125));
    window.draw(veil);

    if (font_ == nullptr)
    {
        return;
    }

    UiHelpers::drawCenteredText(window, *font_, "东南苦行塔", 68,
                                {{270.0f, 95.0f}, {740.0f, 100.0f}},
                                sf::Color(238, 203, 125));
    UiHelpers::drawButton(window, *font_, startButtonBounds(), "START", true,
                          hoveredAction_ == Action::Start);

    const float progress = std::clamp(fadeTimer_ / kFadeDurationSeconds, 0.0f, 1.0f);
    const auto alpha = static_cast<std::uint8_t>((1.0f - progress) * 220.0f);
    sf::RectangleShape fadeOverlay({1280.0f, 720.0f});
    fadeOverlay.setFillColor(sf::Color(0, 0, 0, alpha));
    window.draw(fadeOverlay);
}

sf::FloatRect MainMenuView::startButtonBounds() const
{
    return makeStartBounds();
}
