#include "ui/MainMenuView.hpp"

#include "ui/UiHelpers.hpp"

namespace
{
sf::FloatRect makeStartBounds()
{
    return {{500.0f, 496.0f}, {280.0f, 64.0f}};
}

sf::FloatRect makeLoadBounds()
{
    return {{456.0f, 586.0f}, {368.0f, 64.0f}};
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

void MainMenuView::handleMouseMove(sf::Vector2f position)
{
    if (UiHelpers::contains(startButtonBounds(), position))
    {
        hoveredAction_ = Action::Start;
    }
    else if (UiHelpers::contains(loadButtonBounds(), position))
    {
        hoveredAction_ = Action::Load;
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

    if (UiHelpers::contains(loadButtonBounds(), position))
    {
        return Action::Load;
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
            hoveredAction_ == Action::Load ? loadButtonBounds() :
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
    UiHelpers::drawButton(window, *font_, loadButtonBounds(), "LOAD GAME", true,
                          hoveredAction_ == Action::Load);
}

sf::FloatRect MainMenuView::startButtonBounds() const
{
    return makeStartBounds();
}

sf::FloatRect MainMenuView::loadButtonBounds() const
{
    return makeLoadBounds();
}
