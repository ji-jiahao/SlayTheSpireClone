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

void drawStatusBadge(sf::RenderWindow& window, const sf::Font& font,
                     sf::Vector2f position, const std::string& label, int value,
                     sf::Color color)
{
    sf::CircleShape badge(13.0f, 20);
    badge.setOrigin({13.0f, 13.0f});
    badge.setPosition(position);
    badge.setFillColor(color);
    badge.setOutlineColor(sf::Color(238, 232, 214));
    badge.setOutlineThickness(1.5f);
    window.draw(badge);

    sf::Text labelText = makeText(font, label, 13, sf::Color(250, 246, 236));
    const sf::FloatRect labelBounds = labelText.getLocalBounds();
    labelText.setPosition({position.x - labelBounds.size.x / 2.0f - labelBounds.position.x,
                           position.y - labelBounds.size.y / 2.0f - labelBounds.position.y - 1.0f});
    window.draw(labelText);

    sf::Text valueText = makeText(font, std::to_string(value), 14, sf::Color(220, 215, 204));
    valueText.setPosition({position.x + 18.0f, position.y - 10.0f});
    window.draw(valueText);
}

sf::Color intentColor(EnemyIntentType type)
{
    switch (type)
    {
    case EnemyIntentType::Attack:
    case EnemyIntentType::AttackDefend:
        return sf::Color(178, 65, 54);
    case EnemyIntentType::DefendBuff:
    case EnemyIntentType::Buff:
        return sf::Color(173, 118, 42);
    case EnemyIntentType::Debuff:
        return sf::Color(104, 78, 151);
    case EnemyIntentType::Status:
    case EnemyIntentType::Composite:
        return sf::Color(74, 126, 116);
    case EnemyIntentType::Split:
        return sf::Color(151, 80, 120);
    default:
        return sf::Color(92, 99, 112);
    }
}

std::string intentSymbol(EnemyIntentType type)
{
    switch (type)
    {
    case EnemyIntentType::Attack: return "攻";
    case EnemyIntentType::AttackDefend: return "攻";
    case EnemyIntentType::DefendBuff: return "盾";
    case EnemyIntentType::Buff: return "强";
    case EnemyIntentType::Debuff: return "弱";
    case EnemyIntentType::Status: return "牌";
    case EnemyIntentType::Sleep: return "眠";
    case EnemyIntentType::Stunned: return "晕";
    case EnemyIntentType::Preparing: return "蓄";
    case EnemyIntentType::Split: return "分";
    case EnemyIntentType::Composite: return "复";
    }
    return "?";
}

void drawIntentBadge(sf::RenderWindow& window, const sf::Font& font,
                     sf::Vector2f center, EnemyIntentType type)
{
    sf::CircleShape badge(24.0f, 24);
    badge.setOrigin({24.0f, 24.0f});
    badge.setPosition(center);
    badge.setFillColor(intentColor(type));
    badge.setOutlineColor(sf::Color(240, 216, 160));
    badge.setOutlineThickness(2.0f);
    window.draw(badge);

    sf::Text symbol = makeText(font, intentSymbol(type), 21, sf::Color(250, 246, 236));
    const sf::FloatRect bounds = symbol.getLocalBounds();
    symbol.setPosition({center.x - bounds.size.x / 2.0f - bounds.position.x,
                        center.y - bounds.size.y / 2.0f - bounds.position.y - 2.0f});
    window.draw(symbol);
}
} // namespace

BattleView::BattleView() : font_(nullptr), background_(nullptr) {}

void BattleView::setFont(const sf::Font& font)
{
    font_ = &font;
}

void BattleView::setBackground(const sf::Texture& texture)
{
    background_ = &texture;
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
    if (background_ != nullptr)
    {
        sf::Sprite background(*background_);
        background.setScale({kWindowWidth / background_->getSize().x,
                             kWindowHeight / background_->getSize().y});
        window.draw(background);
        sf::RectangleShape veil({kWindowWidth, kWindowHeight});
        veil.setFillColor(sf::Color(12, 13, 16, 85));
        window.draw(veil);
    }
    else
    {
        sf::RectangleShape background({kWindowWidth, kWindowHeight});
        background.setFillColor(sf::Color(35, 38, 42));
        window.draw(background);
    }

    sf::CircleShape playerBody(68.0f, 24);
    playerBody.setPosition({155.0f, 245.0f});
    playerBody.setFillColor(sf::Color(128, 48, 42, 225));
    playerBody.setOutlineColor(sf::Color(224, 174, 104));
    playerBody.setOutlineThickness(4.0f);
    window.draw(playerBody);

    if (combat.getEnemy().getArchetype() == EnemyArchetype::SlimePair)
    {
        for (int index = 0; index < 2; ++index)
        {
            sf::CircleShape slime(58.0f, 22);
            slime.setPosition({900.0f + index * 128.0f, 270.0f - index * 18.0f});
            slime.setFillColor(index == 0 ? sf::Color(87, 148, 89, 235)
                                          : sf::Color(120, 94, 151, 235));
            slime.setOutlineColor(sf::Color(210, 176, 112));
            slime.setOutlineThickness(4.0f);
            window.draw(slime);
        }
    }
    else
    {
        sf::CircleShape enemyBody(75.0f, 7);
        enemyBody.setPosition({950.0f, 235.0f});
        enemyBody.setFillColor(sf::Color(58, 50, 70, 235));
        enemyBody.setOutlineColor(sf::Color(183, 95, 84));
        enemyBody.setOutlineThickness(4.0f);
        window.draw(enemyBody);
    }

    drawPlayerPanel(window, combat.getPlayer());
    drawEnemyPanel(window, combat.getEnemy(), combat.getEnemyIntentDamage());
    drawHand(window, combat.getHandCards());
    drawEndTurnButton(window);

    if (font_ != nullptr)
    {
        sf::Text piles = makeText(*font_,
            "抽牌 " + std::to_string(combat.getDeck().getDrawPile().size()) +
            "   弃牌 " + std::to_string(combat.getDeck().getDiscardPile().size()) +
            "   消耗 " + std::to_string(combat.getDeck().getExhaustPile().size()),
            17, sf::Color(225, 220, 210));
        piles.setPosition({35.0f, 675.0f});
        window.draw(piles);
    }
}

void BattleView::drawPlayerPanel(sf::RenderWindow& window, const Player& player) const
{
    sf::RectangleShape panel({400.0f, 180.0f});
    panel.setPosition({30.0f, 24.0f});
    panel.setFillColor(sf::Color(48, 52, 58));
    panel.setOutlineColor(sf::Color(40, 32, 26));
    panel.setOutlineThickness(2.0f);
    window.draw(panel);

    const float maxHealth = static_cast<float>(player.getMaxHealth());
    const float currentHealth = static_cast<float>(player.getCurrentHealth());

    if (font_ != nullptr)
    {
        sf::Text title = makeText(*font_, "玩家", 24, sf::Color(235, 229, 207));
        title.setPosition({48.0f, 36.0f});
        window.draw(title);

        sf::Text hpText =
            makeText(*font_, "HP " + std::to_string(player.getCurrentHealth()) + "/" +
                                 std::to_string(player.getMaxHealth()),
                     18, sf::Color(222, 210, 190));
        hpText.setPosition({48.0f, 76.0f});
        window.draw(hpText);

        sf::Text energyText =
            makeText(*font_, "能量 " + std::to_string(player.getCurrentEnergy()) + "/" +
                                 std::to_string(player.getMaxEnergy()),
                     18, sf::Color(240, 200, 120));
        energyText.setPosition({250.0f, 76.0f});
        window.draw(energyText);

        sf::Text blockText =
            makeText(*font_, "格挡 " + std::to_string(player.getBlock()), 18,
                     sf::Color(140, 180, 230));
        blockText.setPosition({48.0f, 124.0f});
        window.draw(blockText);

        drawStatusBadge(window, *font_, {135.0f, 165.0f}, "力", player.getStrength(),
                        sf::Color(170, 83, 55));
        drawStatusBadge(window, *font_, {201.0f, 165.0f}, "弱", player.getWeak(),
                        sf::Color(85, 116, 155));
        drawStatusBadge(window, *font_, {267.0f, 165.0f}, "易", player.getVulnerable(),
                        sf::Color(157, 91, 57));
        drawStatusBadge(window, *font_, {333.0f, 165.0f}, "脆", player.getFrail(),
                        sf::Color(112, 101, 75));
        drawStatusBadge(window, *font_, {69.0f, 165.0f}, "敏", player.getDexterity(),
                        sf::Color(72, 130, 103));
    }

    const float hpRatio = maxHealth > 0.0f ? currentHealth / maxHealth : 0.0f;
    drawBar(window, {48.0f, 106.0f}, {180.0f, 16.0f}, hpRatio,
            sf::Color(196, 70, 60), sf::Color(60, 40, 40));
}

void BattleView::drawEnemyPanel(sf::RenderWindow& window, const Enemy& enemy,
                                int calculatedIntentDamage) const
{
    sf::RectangleShape panel({430.0f, 205.0f});
    panel.setPosition({kWindowWidth - 450.0f, 12.0f});
    panel.setFillColor(sf::Color(48, 52, 58));
    panel.setOutlineColor(sf::Color(40, 32, 26));
    panel.setOutlineThickness(2.0f);
    window.draw(panel);

    const float maxHealth = static_cast<float>(enemy.getMaxHealth());
    const float currentHealth = static_cast<float>(enemy.getCurrentHealth());

    if (font_ != nullptr)
    {
        sf::Text title = makeText(*font_, enemy.getName(), 24, sf::Color(235, 229, 207));
        title.setPosition({kWindowWidth - 432.0f, 25.0f});
        window.draw(title);

        sf::Text hpText = makeText(
            *font_, "HP " + std::to_string(enemy.getCurrentHealth()) + "/" +
                        std::to_string(enemy.getMaxHealth()),
            18, sf::Color(222, 210, 190));
        hpText.setPosition({kWindowWidth - 432.0f, 64.0f});
        window.draw(hpText);

        if (enemy.getBlock() > 0)
        {
            sf::Text blockText = makeText(*font_, "格挡 " + std::to_string(enemy.getBlock()),
                                          16, sf::Color(140, 180, 230));
            blockText.setPosition({kWindowWidth - 230.0f, 65.0f});
            window.draw(blockText);
        }

        const EnemyIntent& intent = enemy.getIntent();
        drawIntentBadge(window, *font_, {kWindowWidth - 408.0f, 137.0f}, intent.type);
        std::string intentLabel = intent.name;
        if (intent.damage > 0)
        {
            intentLabel += "  " + std::to_string(calculatedIntentDamage);
            if (intent.hits > 1) intentLabel += " x " + std::to_string(intent.hits);
        }
        sf::Text intentText = makeText(*font_, intentLabel, 18, sf::Color(240, 216, 160));
        intentText.setPosition({kWindowWidth - 368.0f, 112.0f});
        window.draw(intentText);

        sf::Text intentDetail = makeText(*font_, intent.description, 14, sf::Color(205, 201, 191));
        intentDetail.setPosition({kWindowWidth - 368.0f, 140.0f});
        window.draw(intentDetail);

        const std::string power = enemy.getPowerDescription();
        if (!power.empty())
        {
            sf::Text powerText = makeText(*font_, power, 13, sf::Color(181, 167, 213));
            powerText.setPosition({kWindowWidth - 432.0f, 181.0f});
            window.draw(powerText);
        }

        drawStatusBadge(window, *font_, {kWindowWidth - 190.0f, 91.0f}, "力",
                        enemy.getStrength(), sf::Color(170, 83, 55));
        drawStatusBadge(window, *font_, {kWindowWidth - 125.0f, 91.0f}, "弱",
                        enemy.getWeak(), sf::Color(85, 116, 155));
        drawStatusBadge(window, *font_, {kWindowWidth - 60.0f, 91.0f}, "易",
                        enemy.getVulnerable(), sf::Color(157, 91, 57));
    }

    const float hpRatio = maxHealth > 0.0f ? currentHealth / maxHealth : 0.0f;
    drawBar(window, {kWindowWidth - 432.0f, 91.0f}, {180.0f, 16.0f}, hpRatio,
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
