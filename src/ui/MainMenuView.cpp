#include "ui/MainMenuView.hpp"

#include "ui/UiHelpers.hpp"

namespace
{
const sf::FloatRect kStartButton{{500.0f, 445.0f}, {280.0f, 64.0f}};
const sf::FloatRect kQuitButton{{500.0f, 525.0f}, {280.0f, 64.0f}};
}

void MainMenuView::setFont(const sf::Font& font) { font_ = &font; }
void MainMenuView::setBackground(const sf::Texture& texture) { background_ = &texture; }

MainMenuView::Action MainMenuView::handleMouseClick(sf::Vector2f position) const
{
    if (UiHelpers::contains(kStartButton, position)) return Action::Start;
    if (UiHelpers::contains(kQuitButton, position)) return Action::Quit;
    return Action::None;
}

void MainMenuView::draw(sf::RenderWindow& window) const
{
    if (background_ != nullptr)
    {
        sf::Sprite background(*background_);
        background.setScale({1280.0f / background_->getSize().x, 720.0f / background_->getSize().y});
        window.draw(background);
    }
    else
    {
        sf::RectangleShape background({1280.0f, 720.0f});
        background.setFillColor(sf::Color(30, 31, 34));
        window.draw(background);
    }

    sf::RectangleShape veil({1280.0f, 720.0f});
    veil.setFillColor(sf::Color(12, 13, 16, 125));
    window.draw(veil);
    if (font_ == nullptr) return;

    UiHelpers::drawCenteredText(window, *font_, "尖塔之路", 68,
                                {{270.0f, 95.0f}, {740.0f, 100.0f}},
                                sf::Color(238, 203, 125));
    UiHelpers::drawCenteredText(window, *font_, "铁甲战士 · 第一幕", 28,
                                {{390.0f, 195.0f}, {500.0f, 50.0f}},
                                sf::Color(222, 216, 201));
    UiHelpers::drawButton(window, *font_, kStartButton, "开始游戏", true, true);
    UiHelpers::drawButton(window, *font_, kQuitButton, "结束游戏");
}
