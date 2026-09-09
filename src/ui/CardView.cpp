#include "ui/CardView.hpp"
#include "ui/CardPresentation.hpp"
#include "ui/UiHelpers.hpp"

#include <string>
#include <unordered_map>

namespace
{
constexpr float kCardWidth = 160.0f;
constexpr float kCardHeight = 220.0f;
constexpr float kOutlineThickness = 4.0f;
constexpr const char* kCardArtDirectory = "assets/images/cards/pixel_v2/";
constexpr const char* kStarterCardArtPath = "assets/images/cards/starter_placeholder.png";
constexpr const char* kUncommonCardArtPath = "assets/images/cards/uncommon_placeholder.png";
constexpr const char* kRareCardArtPath = "assets/images/cards/rare_placeholder.png";

struct CardArtCache
{
    // 卡面按卡牌 ID 缓存，确保同名但不同逻辑的卡牌也能正确区分。
    std::unordered_map<std::string, sf::Texture> cards;
    sf::Texture starter;
    sf::Texture uncommon;
    sf::Texture rare;
    bool starterLoaded = false;
    bool uncommonLoaded = false;
    bool rareLoaded = false;
};

CardArtCache& cardArtCache()
{
    static CardArtCache cache;
    return cache;
}

const sf::Texture* textureForCard(const Card& card)
{
    CardArtCache& cache = cardArtCache();
    const auto cardIt = cache.cards.find(card.id);
    if (cardIt != cache.cards.end())
    {
        return cardIt->second.getSize().x > 0 ? &cardIt->second : nullptr;
    }

    // 只在卡牌真正出现在画面上时读取对应资源，并缓存失败结果，避免每帧重复访问磁盘。
    sf::Texture texture;
    const bool cardLoaded =
        texture.loadFromFile(std::string(kCardArtDirectory) + card.id + ".png");
    (void)cardLoaded;
    const auto [it, inserted] = cache.cards.emplace(card.id, std::move(texture));
    (void)inserted;
    if (it->second.getSize().x > 0)
    {
        return &it->second;
    }

    sf::Texture* fallback = nullptr;
    const char* fallbackPath = nullptr;
    bool* fallbackLoaded = nullptr;
    switch (card.rarity)
    {
    case CardRarity::Starter:
    case CardRarity::Common:
        fallback = &cache.starter;
        fallbackPath = kStarterCardArtPath;
        fallbackLoaded = &cache.starterLoaded;
        break;
    case CardRarity::Uncommon:
        fallback = &cache.uncommon;
        fallbackPath = kUncommonCardArtPath;
        fallbackLoaded = &cache.uncommonLoaded;
        break;
    case CardRarity::Rare:
        fallback = &cache.rare;
        fallbackPath = kRareCardArtPath;
        fallbackLoaded = &cache.rareLoaded;
        break;
    case CardRarity::Status:
    case CardRarity::Curse:
        // 状态牌和诅咒牌使用 CardView 的文字/几何卡面，不能套用普通牌占位图。
        return nullptr;
    }

    if (!*fallbackLoaded)
    {
        *fallbackLoaded = true;
        const bool loaded = fallback->loadFromFile(fallbackPath);
        (void)loaded;
    }

    return fallback->getSize().x > 0 ? fallback : nullptr;
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
    if (const sf::Texture* texture = textureForCard(card); texture != nullptr)
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

        // Cover only the printed orb; artwork and cost share the same transform.
        if (font_ != nullptr)
        {
            sf::RenderStates states;
            states.transform.translate(position_ + sf::Vector2f{kCardWidth / 2, kCardHeight / 2});
            states.transform.rotate(sf::degrees(rotation_));
            states.transform.scale({scale_, scale_});
            states.transform.translate({-kCardWidth / 2, -kCardHeight / 2});
            const float radius = 17.0f;
            sf::CircleShape circle(radius, 32);
            circle.setOrigin({radius, radius});
            circle.setPosition({22.0f, 24.0f});
            circle.setFillColor(colorForType(card.type));
            circle.setOutlineThickness(2.0f);
            circle.setOutlineColor(sf::Color(39,31,28));
            target.draw(circle, states);
            circle.setRadius(14.5f);
            circle.setOrigin({14.5f,14.5f});
            circle.setOutlineThickness(1.0f);
            circle.setOutlineColor(sf::Color(242,214,160));
            target.draw(circle, states);
            sf::Text costText = UiHelpers::makeText(*font_, CardPresentation::costLabel(card.cost), 23, sf::Color(255,246,220));
            costText.setStyle(sf::Text::Bold);
            costText.setOutlineThickness(1.0f);
            costText.setOutlineColor(sf::Color(39,31,28));
            const auto bounds = costText.getLocalBounds();
            costText.setPosition(circle.getPosition() - sf::Vector2f{bounds.position.x + bounds.size.x / 2.0f,
                                                                       bounds.position.y + bounds.size.y / 2.0f});
            target.draw(costText, states);
        }
        return;
    }

    sf::RenderStates states;
    const sf::Vector2f center = position_ + sf::Vector2f{kCardWidth / 2, kCardHeight / 2};
    states.transform.translate(center).rotate(sf::degrees(rotation_)).scale({scale_, scale_}).translate(-center);
    sf::RectangleShape body({kCardWidth, kCardHeight});
    body.setPosition(position_);
    body.setFillColor(colorForType(card.type));
    body.setOutlineColor(sf::Color(40, 32, 26));
    body.setOutlineThickness(kOutlineThickness);
    target.draw(body, states);

    if (font_ == nullptr)
    {
        return;
    }

    // 费用：左上角圆形。
    if (card.cost >= -1)
    {
        const float radius = 22.0f;
        sf::CircleShape costCircle(radius, 24);
        costCircle.setOrigin({radius, radius});
        costCircle.setPosition({position_.x + radius + 6.0f, position_.y + radius + 6.0f});
        costCircle.setFillColor(sf::Color(246, 240, 224));
        target.draw(costCircle, states);

        const std::string costLabel = CardPresentation::costLabel(card.cost);
        sf::Text costText = UiHelpers::makeText(*font_, costLabel, 22,
                                                sf::Color(40, 32, 26));
        const sf::FloatRect costBounds = costText.getLocalBounds();
        costText.setPosition(
            {position_.x + radius + 6.0f - costBounds.size.x / 2.0f - costBounds.position.x,
             position_.y + radius + 6.0f - costBounds.size.y / 2.0f - costBounds.position.y - 2.0f});
        target.draw(costText, states);
    }

    // 卡名：顶部居中。
    sf::Text nameText = UiHelpers::makeText(*font_, card.name, 20, sf::Color(250, 246, 236));
    const sf::FloatRect nameBounds = nameText.getLocalBounds();
    nameText.setPosition({position_.x + (kCardWidth - nameBounds.size.x) / 2.0f -
                              nameBounds.position.x,
                          position_.y + 10.0f});
    target.draw(nameText, states);

    // 描述面板。
    sf::RectangleShape panel({kCardWidth - 24.0f, kCardHeight - 96.0f});
    panel.setPosition({position_.x + 12.0f, position_.y + 48.0f});
    panel.setFillColor(sf::Color(246, 240, 224));
    target.draw(panel, states);

    const std::vector<std::string> lines =
        UiHelpers::wrapText(*font_, card.description, 16, kCardWidth - 48.0f);
    float lineY = position_.y + 60.0f;
    for (const std::string& line : lines)
    {
        sf::Text descText = UiHelpers::makeText(*font_, line, 16, sf::Color(40, 34, 28));
        descText.setPosition({position_.x + 24.0f, lineY});
        target.draw(descText, states);
        lineY += 22.0f;
    }

    // 伤害 / 格挡数值：底部。
    if (card.damage > 0)
    {
        sf::Text damageText =
            UiHelpers::makeText(*font_, "DMG " + std::to_string(card.damage), 18,
                                sf::Color(250, 246, 236));
        damageText.setPosition({position_.x + 14.0f, position_.y + kCardHeight - 34.0f});
        target.draw(damageText, states);
    }

    if (card.block > 0)
    {
        sf::Text blockText =
            UiHelpers::makeText(*font_, "BLK " + std::to_string(card.block), 18,
                                sf::Color(250, 246, 236));
        const sf::FloatRect blockBounds = blockText.getLocalBounds();
        blockText.setPosition({position_.x + kCardWidth - 14.0f - blockBounds.size.x,
                               position_.y + kCardHeight - 34.0f});
        target.draw(blockText, states);
    }
}
