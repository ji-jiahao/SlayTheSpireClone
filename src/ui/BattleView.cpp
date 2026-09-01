#include "ui/BattleView.hpp"

#include "ui/CardView.hpp"

#include <algorithm>
#include <string>

namespace
{
// 战斗界面按固定 1280x720 逻辑分辨率排版，与其它界面保持一致。
constexpr float kWindowWidth = 1280.0f;
constexpr float kWindowHeight = 720.0f;

constexpr float kHandGap = 20.0f;
constexpr float kHandY = 470.0f;

sf::String toSfString(const std::string& text)
{
    return sf::String::fromUtf8(text.begin(), text.end());
}

sf::Text makeText(const sf::Font& font, const std::string& text,
                  unsigned int characterSize, sf::Color color)
{
    sf::Text drawableText(font, toSfString(text), characterSize);
    drawableText.setFillColor(color);
    return drawableText;
}

void drawBar(sf::RenderWindow& window, sf::Vector2f position, sf::Vector2f size,
             float fillRatio, sf::Color fillColor, sf::Color backgroundColor)
{
    sf::RectangleShape background(size);
    background.setPosition(position);
    background.setFillColor(backgroundColor);
    window.draw(background);

    if (fillRatio > 0.0f)
    {
        const float clamped = std::clamp(fillRatio, 0.0f, 1.0f);
        sf::RectangleShape fill({size.x * clamped, size.y});
        fill.setPosition(position);
        fill.setFillColor(fillColor);
        window.draw(fill);
    }

    sf::RectangleShape outline(size);
    outline.setPosition(position);
    outline.setFillColor(sf::Color::Transparent);
    outline.setOutlineColor(sf::Color(40, 32, 26));
    outline.setOutlineThickness(2.0f);
    window.draw(outline);
}
} // namespace

BattleView::BattleView() : font_(nullptr) {}

void BattleView::setFont(const sf::Font& font)
{
    font_ = &font;
}

sf::FloatRect BattleView::getEndTurnButtonBounds() const
{
    return {{kWindowWidth - 170.0f, 620.0f}, {140.0f, 60.0f}};
}

std::vector<BattleView::HandCardLayout> BattleView::layoutHand(
    const std::vector<Card>& hand) const
{
    const std::size_t count = hand.size();
    const sf::Vector2f cardSize = CardView::getCardSize();

    std::vector<HandCardLayout> layouts;
    layouts.reserve(count);

    if (count == 0)
    {
        return layouts;
    }

    const float totalWidth =
        static_cast<float>(count) * cardSize.x + static_cast<float>(count - 1) * kHandGap;
    const float startX = (kWindowWidth - totalWidth) / 2.0f;

    for (std::size_t index = 0; index < count; ++index)
    {
        HandCardLayout layout;
        layout.handIndex = static_cast<int>(index);
        layout.bounds = {
            {startX + static_cast<float>(index) * (cardSize.x + kHandGap), kHandY},
            cardSize};
        layouts.push_back(layout);
    }

    return layouts;
}

void BattleView::handleMouseClick(sf::Vector2f mousePosition, CombatSystem& combat)
{
    for (const HandCardLayout& layout : layoutHand(combat.getHandCards()))
    {
        if (layout.bounds.contains(mousePosition))
        {
            combat.playCard(layout.handIndex);
            return;
        }
    }

    if (getEndTurnButtonBounds().contains(mousePosition))
    {
        combat.endPlayerTurn();
    }
}

void BattleView::draw(sf::RenderWindow& window, const CombatSystem& combat) const
{
    sf::RectangleShape background({kWindowWidth, kWindowHeight});
    background.setFillColor(sf::Color(35, 38, 42));
    window.draw(background);

    drawPlayerPanel(window, combat.getPlayer());
    drawEnemyPanel(window, combat.getEnemy());
    drawHand(window, combat.getHandCards());
    drawEndTurnButton(window);
}

void BattleView::drawPlayerPanel(sf::RenderWindow& window, const Player& player) const
{
    sf::RectangleShape panel({380.0f, 150.0f});
    panel.setPosition({40.0f, 40.0f});
    panel.setFillColor(sf::Color(48, 52, 58));
    panel.setOutlineColor(sf::Color(40, 32, 26));
    panel.setOutlineThickness(2.0f);
    window.draw(panel);

    const float maxHealth = static_cast<float>(player.getMaxHealth());
    const float currentHealth = static_cast<float>(player.getCurrentHealth());

    if (font_ != nullptr)
    {
        sf::Text title = makeText(*font_, "玩家", 24, sf::Color(235, 229, 207));
        title.setPosition({58.0f, 52.0f});
        window.draw(title);

        sf::Text hpText =
            makeText(*font_, "HP " + std::to_string(player.getCurrentHealth()) + "/" +
                                 std::to_string(player.getMaxHealth()),
                     18, sf::Color(222, 210, 190));
        hpText.setPosition({58.0f, 92.0f});
        window.draw(hpText);

        sf::Text energyText =
            makeText(*font_, "能量 " + std::to_string(player.getCurrentEnergy()) + "/" +
                                 std::to_string(player.getMaxEnergy()),
                     18, sf::Color(240, 200, 120));
        energyText.setPosition({250.0f, 92.0f});
        window.draw(energyText);

        sf::Text blockText =
            makeText(*font_, "格挡 " + std::to_string(player.getBlock()), 18,
                     sf::Color(140, 180, 230));
        blockText.setPosition({58.0f, 140.0f});
        window.draw(blockText);
    }

    const float hpRatio = maxHealth > 0.0f ? currentHealth / maxHealth : 0.0f;
    drawBar(window, {58.0f, 122.0f}, {180.0f, 16.0f}, hpRatio,
            sf::Color(196, 70, 60), sf::Color(60, 40, 40));
}

void BattleView::drawEnemyPanel(sf::RenderWindow& window, const Enemy& enemy) const
{
    sf::RectangleShape panel({380.0f, 150.0f});
    panel.setPosition({kWindowWidth - 420.0f, 40.0f});
    panel.setFillColor(sf::Color(48, 52, 58));
    panel.setOutlineColor(sf::Color(40, 32, 26));
    panel.setOutlineThickness(2.0f);
    window.draw(panel);

    const float maxHealth = static_cast<float>(enemy.getMaxHealth());
    const float currentHealth = static_cast<float>(enemy.getCurrentHealth());

    if (font_ != nullptr)
    {
        sf::Text title = makeText(*font_, enemy.getName(), 24, sf::Color(235, 229, 207));
        title.setPosition({kWindowWidth - 402.0f, 52.0f});
        window.draw(title);

        sf::Text hpText = makeText(
            *font_, "HP " + std::to_string(enemy.getCurrentHealth()) + "/" +
                        std::to_string(enemy.getMaxHealth()),
            18, sf::Color(222, 210, 190));
        hpText.setPosition({kWindowWidth - 402.0f, 92.0f});
        window.draw(hpText);

        sf::Text intentText =
            makeText(*font_, "意图 " + std::to_string(enemy.getIntentDamage()), 18,
                     sf::Color(230, 120, 110));
        intentText.setPosition({kWindowWidth - 402.0f, 140.0f});
        window.draw(intentText);
    }

    const float hpRatio = maxHealth > 0.0f ? currentHealth / maxHealth : 0.0f;
    drawBar(window, {kWindowWidth - 402.0f, 122.0f}, {180.0f, 16.0f}, hpRatio,
            sf::Color(196, 70, 60), sf::Color(60, 40, 40));
}

void BattleView::drawHand(sf::RenderWindow& window, const std::vector<Card>& hand) const
{
    const std::vector<HandCardLayout> layouts = layoutHand(hand);
    for (const HandCardLayout& layout : layouts)
    {
        CardView cardView;
        if (font_ != nullptr)
        {
            cardView.setFont(*font_);
        }
        cardView.setPosition(layout.bounds.position);
        cardView.draw(window, hand[static_cast<std::size_t>(layout.handIndex)]);
    }
}

void BattleView::drawEndTurnButton(sf::RenderWindow& window) const
{
    const sf::FloatRect bounds = getEndTurnButtonBounds();

    sf::RectangleShape button({bounds.size.x, bounds.size.y});
    button.setPosition(bounds.position);
    button.setFillColor(sf::Color(230, 174, 72));
    button.setOutlineColor(sf::Color(36, 28, 18));
    button.setOutlineThickness(3.0f);
    window.draw(button);

    if (font_ != nullptr)
    {
        sf::Text label = makeText(*font_, "结束回合", 22, sf::Color(24, 19, 14));
        const sf::FloatRect textBounds = label.getLocalBounds();
        label.setPosition({bounds.position.x + (bounds.size.x - textBounds.size.x) / 2.0f -
                               textBounds.position.x,
                           bounds.position.y + (bounds.size.y - textBounds.size.y) / 2.0f -
                               textBounds.position.y - 2.0f});
        window.draw(label);
    }
}
