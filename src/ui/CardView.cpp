#include "ui/CardView.hpp"
#include "ui/UiHelpers.hpp"

#include <array>
#include <string>
#include <vector>

namespace
{
constexpr float kCardWidth = 160.0f;
constexpr float kCardHeight = 220.0f;
constexpr float kOutlineThickness = 4.0f;
constexpr const char* kStarterCardArtPath = "assets/images/cards/starter_placeholder.png";
constexpr const char* kUncommonCardArtPath = "assets/images/cards/uncommon_placeholder.png";
constexpr const char* kRareCardArtPath = "assets/images/cards/rare_placeholder.png";

struct CardArtCache
{
    sf::Texture starter;
    sf::Texture uncommon;
    sf::Texture rare;
    bool loaded = false;
};

CardArtCache& cardArtCache()
{
    static CardArtCache cache;
    if (!cache.loaded)
    {
        const bool starterLoaded = cache.starter.loadFromFile(kStarterCardArtPath);
        const bool uncommonLoaded = cache.uncommon.loadFromFile(kUncommonCardArtPath);
        const bool rareLoaded = cache.rare.loadFromFile(kRareCardArtPath);
        (void)starterLoaded;
        (void)uncommonLoaded;
        (void)rareLoaded;
        cache.loaded = true;
    }
    return cache;
}

const sf::Texture* textureForRarity(CardRarity rarity)
{
    const CardArtCache& cache = cardArtCache();
    switch (rarity)
    {
    case CardRarity::Starter:
    case CardRarity::Common:
        return cache.starter.getSize().x > 0 ? &cache.starter : nullptr;
    case CardRarity::Uncommon:
        return cache.uncommon.getSize().x > 0 ? &cache.uncommon : nullptr;
    case CardRarity::Rare:
        return cache.rare.getSize().x > 0 ? &cache.rare : nullptr;
    case CardRarity::Status:
    case CardRarity::Curse:
        return cache.starter.getSize().x > 0 ? &cache.starter : nullptr;
    }

    return nullptr;
}
} // namespace

CardView::CardView()
    : position_(0.0f, 0.0f),
      scale_(1.0f),
      rotation_(0.0f),
      font_(nullptr)
{
}

void CardView::setFont(const sf::Font& font)
{
    font_ = &font;
}

void CardView::setPosition(sf::Vector2f position)
{
    position_ = position;
}

void CardView::setScale(float scale)
{
    scale_ = scale;
}

void CardView::setRotation(float rotationDegrees)
{
    rotation_ = rotationDegrees;
}

sf::Vector2f CardView::getCardSize()
{
    return {kCardWidth, kCardHeight};
}

sf::FloatRect CardView::getBounds() const
{
    return {position_, {kCardWidth, kCardHeight}};
}

sf::Color CardView::colorForType(CardType type) const
{
    switch (type)
    {
    case CardType::Attack:
        return sf::Color(196, 84, 70);
    case CardType::Skill:
        return sf::Color(78, 128, 186);
    case CardType::Power:
        return sf::Color(186, 148, 62);
    }

    return sf::Color(150, 150, 150);
}

void CardView::draw(sf::RenderTarget& target, const Card& card) const
{
    if (const sf::Texture* texture = textureForRarity(card.rarity); texture != nullptr)
    {
        sf::Sprite sprite(*texture);
        const sf::Vector2u textureSize = texture->getSize();
        sprite.setOrigin({static_cast<float>(textureSize.x) / 2.0f,
                          static_cast<float>(textureSize.y) / 2.0f});
        sprite.setPosition({position_.x + kCardWidth / 2.0f, position_.y + kCardHeight / 2.0f});
        sprite.setScale({(kCardWidth / static_cast<float>(textureSize.x)) * scale_,
                         (kCardHeight / static_cast<float>(textureSize.y)) * scale_});
        sprite.setRotation(sf::degrees(rotation_));
        target.draw(sprite);
        return;
    }

    sf::RectangleShape body({kCardWidth, kCardHeight});
    body.setPosition(position_);
    body.setFillColor(colorForType(card.type));
    body.setOutlineColor(sf::Color(40, 32, 26));
    body.setOutlineThickness(kOutlineThickness);
    target.draw(body);

    if (font_ == nullptr)
    {
        return;
    }

    // 费用：左上角圆形。
    if (card.cost > 0)
    {
        const float radius = 22.0f;
        sf::CircleShape costCircle(radius, 24);
        costCircle.setOrigin({radius, radius});
        costCircle.setPosition({position_.x + radius + 6.0f, position_.y + radius + 6.0f});
        costCircle.setFillColor(sf::Color(246, 240, 224));
        target.draw(costCircle);

        sf::Text costText = UiHelpers::makeText(*font_, std::to_string(card.cost), 22,
                                                sf::Color(40, 32, 26));
        const sf::FloatRect costBounds = costText.getLocalBounds();
        costText.setPosition(
            {position_.x + radius + 6.0f - costBounds.size.x / 2.0f - costBounds.position.x,
             position_.y + radius + 6.0f - costBounds.size.y / 2.0f - costBounds.position.y - 2.0f});
        target.draw(costText);
    }

    // 卡名：顶部居中。
    sf::Text nameText = UiHelpers::makeText(*font_, card.name, 20, sf::Color(250, 246, 236));
    const sf::FloatRect nameBounds = nameText.getLocalBounds();
    nameText.setPosition({position_.x + (kCardWidth - nameBounds.size.x) / 2.0f -
                              nameBounds.position.x,
                          position_.y + 10.0f});
    target.draw(nameText);

    // 描述面板。
    sf::RectangleShape panel({kCardWidth - 24.0f, kCardHeight - 96.0f});
    panel.setPosition({position_.x + 12.0f, position_.y + 48.0f});
    panel.setFillColor(sf::Color(246, 240, 224));
    target.draw(panel);

    const std::vector<std::string> lines =
        UiHelpers::wrapText(*font_, card.description, 16, kCardWidth - 48.0f);
    float lineY = position_.y + 60.0f;
    for (const std::string& line : lines)
    {
        sf::Text descText = UiHelpers::makeText(*font_, line, 16, sf::Color(40, 34, 28));
        descText.setPosition({position_.x + 24.0f, lineY});
        target.draw(descText);
        lineY += 22.0f;
    }

    // 伤害 / 格挡数值：底部。
    if (card.damage > 0)
    {
        sf::Text damageText =
            UiHelpers::makeText(*font_, "DMG " + std::to_string(card.damage), 18,
                                sf::Color(250, 246, 236));
        damageText.setPosition({position_.x + 14.0f, position_.y + kCardHeight - 34.0f});
        target.draw(damageText);
    }

    if (card.block > 0)
    {
        sf::Text blockText =
            UiHelpers::makeText(*font_, "BLK " + std::to_string(card.block), 18,
                                sf::Color(250, 246, 236));
        const sf::FloatRect blockBounds = blockText.getLocalBounds();
        blockText.setPosition({position_.x + kCardWidth - 14.0f - blockBounds.size.x,
                               position_.y + kCardHeight - 34.0f});
        target.draw(blockText);
    }
}
