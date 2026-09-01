#include "ui/RoomView.hpp"

#include "ui/UiHelpers.hpp"

void RoomView::setFont(const sf::Font& font) { font_ = &font; }
void RoomView::setBackground(const sf::Texture& texture) { background_ = &texture; }

sf::FloatRect RoomView::actionBounds(std::size_t index, std::size_t count) const
{
    const float width = count <= 1 ? 420.0f : 300.0f;
    const float gap = 18.0f;
    const float total = static_cast<float>(count) * width + static_cast<float>(count - 1) * gap;
    return {{(1280.0f - total) / 2.0f + static_cast<float>(index) * (width + gap), 520.0f},
            {width, 105.0f}};
}

int RoomView::handleMouseClick(sf::Vector2f position,
                               const std::vector<RoomAction>& actions) const
{
    for (std::size_t index = 0; index < actions.size(); ++index)
    {
        if (actions[index].enabled && UiHelpers::contains(actionBounds(index, actions.size()), position))
        {
            return static_cast<int>(index);
        }
    }
    return -1;
}

void RoomView::draw(sf::RenderWindow& window, const GameState& state,
                    const std::string& eyebrow, const std::string& title,
                    const std::string& description,
                    const std::vector<RoomAction>& actions) const
{
    if (background_ != nullptr)
    {
        sf::Sprite background(*background_);
        background.setScale({1280.0f / background_->getSize().x, 720.0f / background_->getSize().y});
        window.draw(background);
    }

    sf::RectangleShape veil({1280.0f, 720.0f});
    veil.setFillColor(sf::Color(10, 11, 14, 145));
    window.draw(veil);

    sf::RectangleShape topBar({1280.0f, 66.0f});
    topBar.setFillColor(sf::Color(22, 22, 25, 220));
    window.draw(topBar);

    sf::RectangleShape content({920.0f, 330.0f});
    content.setPosition({180.0f, 120.0f});
    content.setFillColor(sf::Color(29, 29, 32, 225));
    content.setOutlineColor(sf::Color(175, 143, 87));
    content.setOutlineThickness(2.0f);
    window.draw(content);
    if (font_ == nullptr) return;

    UiHelpers::drawText(window, *font_, "HP " + std::to_string(state.currentHealth) + "/" +
                                               std::to_string(state.maxHealth),
                        20, {38.0f, 19.0f}, sf::Color(227, 112, 98));
    UiHelpers::drawText(window, *font_, "金币 " + std::to_string(state.gold), 20,
                        {230.0f, 19.0f}, sf::Color(238, 202, 98));
    UiHelpers::drawText(window, *font_, "牌组 " + std::to_string(state.deck.size()), 20,
                        {380.0f, 19.0f}, sf::Color(190, 204, 215));

    UiHelpers::drawCenteredText(window, *font_, eyebrow, 18,
                                {{250.0f, 145.0f}, {780.0f, 32.0f}},
                                sf::Color(190, 160, 105));
    UiHelpers::drawCenteredText(window, *font_, title, 40,
                                {{250.0f, 185.0f}, {780.0f, 65.0f}},
                                sf::Color(244, 232, 208));
    UiHelpers::drawCenteredText(window, *font_, description, 22,
                                {{250.0f, 285.0f}, {780.0f, 100.0f}},
                                sf::Color(205, 202, 196));

    for (std::size_t index = 0; index < actions.size(); ++index)
    {
        const sf::FloatRect bounds = actionBounds(index, actions.size());
        UiHelpers::drawButton(window, *font_, bounds, actions[index].label,
                              actions[index].enabled, index == 0);
        if (!actions[index].detail.empty())
        {
            UiHelpers::drawCenteredText(window, *font_, actions[index].detail, 15,
                                        {{bounds.position.x + 8.0f, bounds.position.y + 65.0f},
                                         {bounds.size.x - 16.0f, 30.0f}},
                                        actions[index].enabled ? sf::Color(205, 200, 190)
                                                               : sf::Color(125, 125, 130));
        }
    }
}
