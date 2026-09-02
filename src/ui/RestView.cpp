#include "ui/RestView.hpp"

#include "ui/UiHelpers.hpp"

#include <algorithm>

namespace
{
constexpr float kWindowWidth = 1280.0f;
constexpr float kWindowHeight = 720.0f;
}

void RestView::setBackground(const sf::Texture* texture)
{
    background_ = texture;
}

void RestView::setFont(const sf::Font& font)
{
    font_ = &font;
}

void RestView::handleMouseMove(sf::Vector2f position)
{
    if (UiHelpers::contains(restButtonBounds(), position))
    {
        hoveredAction_ = Action::Rest;
    }
    else if (UiHelpers::contains(leaveButtonBounds(), position))
    {
        hoveredAction_ = Action::Leave;
    }
    else
    {
        hoveredAction_ = Action::None;
    }
}

RestView::Action RestView::handleMouseClick(sf::Vector2f position) const
{
    if (UiHelpers::contains(restButtonBounds(), position))
    {
        return Action::Rest;
    }

    if (UiHelpers::contains(leaveButtonBounds(), position))
    {
        return Action::Leave;
    }

    return Action::None;
}

void RestView::draw(sf::RenderWindow& window, const GameState& state,
                    int healAmount, bool rested, const std::string& message) const
{
    if (background_ != nullptr)
    {
        sf::Sprite background(*background_);
        const sf::FloatRect bounds = background.getLocalBounds();
        const float scale = std::max(kWindowWidth / bounds.size.x,
                                     kWindowHeight / bounds.size.y);
        background.setScale({scale, scale});
        background.setPosition({(kWindowWidth - bounds.size.x * scale) / 2.0f,
                                (kWindowHeight - bounds.size.y * scale) / 2.0f});
        window.draw(background);
    }
    else
    {
        sf::RectangleShape background({kWindowWidth, kWindowHeight});
        background.setFillColor(sf::Color(30, 27, 25));
        window.draw(background);

        sf::CircleShape fire(70.0f, 36);
        fire.setOrigin({70.0f, 70.0f});
        fire.setPosition({kWindowWidth * 0.5f, 330.0f});
        fire.setFillColor(sf::Color(210, 78, 42));
        fire.setOutlineColor(sf::Color(246, 194, 96));
        fire.setOutlineThickness(8.0f);
        window.draw(fire);
    }

    sf::RectangleShape veil({kWindowWidth, kWindowHeight});
    veil.setFillColor(sf::Color(18, 14, 12, 80));
    window.draw(veil);

    if (font_ == nullptr)
    {
        return;
    }

    UiHelpers::drawCenteredText(window, *font_, "篝火", 44,
                                {{390.0f, 70.0f}, {500.0f, 64.0f}},
                                sf::Color(244, 220, 145));
    UiHelpers::drawCenteredText(window, *font_, "生命 " +
                                    std::to_string(state.currentHealth) + "/" +
                                    std::to_string(state.maxHealth),
                                24, {{390.0f, 145.0f}, {500.0f, 42.0f}},
                                sf::Color(230, 160, 140));

    const std::string detail = rested ? "已经休息过，可以离开。"
                                      : "休息并回复 " + std::to_string(healAmount) + " 点生命。";
    UiHelpers::drawCenteredText(window, *font_, detail, 24,
                                {{310.0f, 420.0f}, {660.0f, 48.0f}},
                                sf::Color(224, 216, 198));

    if (!message.empty())
    {
        UiHelpers::drawCenteredText(window, *font_, message, 20,
                                    {{300.0f, 480.0f}, {680.0f, 38.0f}},
                                    sf::Color(190, 220, 170));
    }

    UiHelpers::drawButton(window, *font_, restButtonBounds(), "休息",
                          !rested, hoveredAction_ == Action::Rest);
    UiHelpers::drawButton(window, *font_, leaveButtonBounds(), "离开",
                          true, hoveredAction_ == Action::Leave);
}

sf::FloatRect RestView::restButtonBounds() const
{
    return {{400.0f, 555.0f}, {210.0f, 64.0f}};
}

sf::FloatRect RestView::leaveButtonBounds() const
{
    return {{670.0f, 555.0f}, {210.0f, 64.0f}};
}
