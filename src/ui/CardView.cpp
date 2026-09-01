#include "ui/CardView.hpp"

#include <sstream>
#include <string>
#include <vector>

namespace
{
constexpr float kCardWidth = 160.0f;
constexpr float kCardHeight = 220.0f;
constexpr float kOutlineThickness = 4.0f;

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

// 按最大宽度把文本拆成多行，用于卡牌描述。
std::vector<std::string> wrapText(const sf::Font& font, const std::string& text,
                                  unsigned int characterSize, float maxWidth)
{
    std::vector<std::string> lines;
    std::istringstream words(text);
    std::string word;
    std::string line;

    sf::Text measure(font, "", characterSize);

    while (words >> word)
    {
        const std::string candidate = line.empty() ? word : line + " " + word;
        measure.setString(toSfString(candidate));
        if (!line.empty() && measure.getLocalBounds().size.x > maxWidth)
        {
            lines.push_back(line);
            line = word;
        }
        else
        {
            line = candidate;
        }
    }

    if (!line.empty())
    {
        lines.push_back(line);
    }

    return lines;
}
} // namespace

CardView::CardView() : position_(0.0f, 0.0f), font_(nullptr) {}

void CardView::setFont(const sf::Font& font)
{
    font_ = &font;
}

void CardView::setPosition(sf::Vector2f position)
{
    position_ = position;
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

void CardView::draw(sf::RenderWindow& window, const Card& card) const
{
    sf::RectangleShape body({kCardWidth, kCardHeight});
    body.setPosition(position_);
    body.setFillColor(colorForType(card.type));
    body.setOutlineColor(sf::Color(40, 32, 26));
    body.setOutlineThickness(kOutlineThickness);
    window.draw(body);

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
        window.draw(costCircle);

        sf::Text costText = makeText(*font_, std::to_string(card.cost), 22,
                                     sf::Color(40, 32, 26));
        const sf::FloatRect costBounds = costText.getLocalBounds();
        costText.setPosition(
            {position_.x + radius + 6.0f - costBounds.size.x / 2.0f - costBounds.position.x,
             position_.y + radius + 6.0f - costBounds.size.y / 2.0f - costBounds.position.y - 2.0f});
        window.draw(costText);
    }

    // 卡名：顶部居中。
    sf::Text nameText = makeText(*font_, card.name, 20, sf::Color(250, 246, 236));
    const sf::FloatRect nameBounds = nameText.getLocalBounds();
    nameText.setPosition({position_.x + (kCardWidth - nameBounds.size.x) / 2.0f -
                              nameBounds.position.x,
                          position_.y + 10.0f});
    window.draw(nameText);

    // 描述面板。
    sf::RectangleShape panel({kCardWidth - 24.0f, kCardHeight - 96.0f});
    panel.setPosition({position_.x + 12.0f, position_.y + 48.0f});
    panel.setFillColor(sf::Color(246, 240, 224));
    window.draw(panel);

    const std::vector<std::string> lines =
        wrapText(*font_, card.description, 16, kCardWidth - 48.0f);
    float lineY = position_.y + 60.0f;
    for (const std::string& line : lines)
    {
        sf::Text descText = makeText(*font_, line, 16, sf::Color(40, 34, 28));
        descText.setPosition({position_.x + 24.0f, lineY});
        window.draw(descText);
        lineY += 22.0f;
    }

    // 伤害 / 格挡数值：底部。
    if (card.damage > 0)
    {
        sf::Text damageText = makeText(*font_, "DMG " + std::to_string(card.damage), 18,
                                       sf::Color(250, 246, 236));
        damageText.setPosition({position_.x + 14.0f, position_.y + kCardHeight - 34.0f});
        window.draw(damageText);
    }

    if (card.block > 0)
    {
        sf::Text blockText = makeText(*font_, "BLK " + std::to_string(card.block), 18,
                                      sf::Color(250, 246, 236));
        const sf::FloatRect blockBounds = blockText.getLocalBounds();
        blockText.setPosition({position_.x + kCardWidth - 14.0f - blockBounds.size.x,
                               position_.y + kCardHeight - 34.0f});
        window.draw(blockText);
    }
}
