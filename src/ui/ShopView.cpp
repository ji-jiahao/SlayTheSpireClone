#include "ui/ShopView.hpp"

#include "card/CardDatabase.hpp"
#include "ui/BattleHover.hpp"
#include "ui/CardView.hpp"
#include "ui/UiHelpers.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <exception>
#include <filesystem>

namespace
{
constexpr float kWindowWidth = 1280.0f;
constexpr float kWindowHeight = 720.0f;
constexpr float kShopCardScale = 0.72f;
constexpr float kDeckCardScale = 0.46f;
constexpr float kDialogueFadeInSeconds = 0.45f;
constexpr float kDialogueVisibleSeconds = 3.0f;
constexpr float kDialogueFadeOutSeconds = 0.65f;
constexpr float kDialogueCycleSeconds =
    kDialogueFadeInSeconds + kDialogueVisibleSeconds + kDialogueFadeOutSeconds;
constexpr float kDialoguePauseSeconds = 0.8f;
constexpr float kMerchantFrameSeconds = 0.09f;

std::string joinLines(const std::vector<std::string>& lines)
{
    std::string result;
    for (std::size_t index = 0; index < lines.size(); ++index)
    {
        if (index > 0)
        {
            result += '\n';
        }
        result += lines[index];
    }
    return result;
}

sf::Color priceColor(bool affordable)
{
    return affordable ? sf::Color(114, 218, 118) : sf::Color(220, 78, 70);
}
} // namespace

void ShopView::setFont(const sf::Font& font)
{
    font_ = &font;

    cardTooltipBackground_.setFillColor(sf::Color(27, 25, 30, 242));
    cardTooltipOutline_.setFillColor(sf::Color::Transparent);
    cardTooltipOutline_.setOutlineColor(sf::Color(244, 216, 150, 130));
    cardTooltipOutline_.setOutlineThickness(3.0f);
    cardTooltipCostCircle_.setRadius(15.0f);
    cardTooltipCostCircle_.setPointCount(32);
    cardTooltipCostCircle_.setOrigin({15.0f, 15.0f});
    cardTooltipCostCircle_.setFillColor(sf::Color(244, 232, 198));
    cardTooltipCostCircle_.setOutlineColor(sf::Color(52, 42, 30));
    cardTooltipCostCircle_.setOutlineThickness(2.0f);

    cardTooltipNameText_.emplace(font, UiHelpers::toSfString(""), 22);
    cardTooltipNameText_->setFillColor(sf::Color(246, 239, 224));
    cardTooltipTypeText_.emplace(font, UiHelpers::toSfString(""), 16);
    cardTooltipTypeText_->setFillColor(sf::Color(218, 208, 190));
    cardTooltipCostText_.emplace(font, UiHelpers::toSfString(""), 18);
    cardTooltipCostText_->setFillColor(sf::Color(44, 34, 24));
    cardTooltipDescriptionText_.emplace(font, UiHelpers::toSfString(""), 15);
    cardTooltipDescriptionText_->setFillColor(sf::Color(230, 222, 208));
}

void ShopView::setBackground(const sf::Texture* texture)
{
    background_ = texture;
}

bool ShopView::loadMerchantAnimation(const std::string& framesDirectory)
{
    merchantFrames_.clear();
    merchantFrameIndex_ = 0;
    merchantFrameTimer_ = 0.0f;

    std::vector<std::filesystem::path> framePaths;
    if (std::filesystem::exists(framesDirectory))
    {
        for (const std::filesystem::directory_entry& entry :
             std::filesystem::directory_iterator(framesDirectory))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".png")
            {
                framePaths.push_back(entry.path());
            }
        }
    }

    std::sort(framePaths.begin(), framePaths.end());
    for (const std::filesystem::path& path : framePaths)
    {
        sf::Texture texture;
        if (texture.loadFromFile(path.string()))
        {
            merchantFrames_.push_back(std::move(texture));
        }
    }

    merchantLoaded_ = !merchantFrames_.empty();
    return merchantLoaded_;
}

void ShopView::resetDialogue(unsigned int seed)
{
    dialogueRandomEngine_.seed(seed);
    dialogueTimer_ = 0.0f;
    dialoguePauseTimer_ = 0.0f;
    chooseNextDialogue();
}

void ShopView::update(float deltaSeconds)
{
    if (merchantLoaded_ && merchantFrames_.size() > 1)
    {
        merchantFrameTimer_ += deltaSeconds;
        while (merchantFrameTimer_ >= kMerchantFrameSeconds)
        {
            merchantFrameTimer_ -= kMerchantFrameSeconds;
            merchantFrameIndex_ = (merchantFrameIndex_ + 1) % merchantFrames_.size();
        }
    }

    if (dialoguePauseTimer_ > 0.0f)
    {
        dialoguePauseTimer_ = std::max(0.0f, dialoguePauseTimer_ - deltaSeconds);
        if (dialoguePauseTimer_ <= 0.0f)
        {
            dialogueTimer_ = 0.0f;
            chooseNextDialogue();
        }
        return;
    }

    dialogueTimer_ += deltaSeconds;
    if (dialogueTimer_ >= kDialogueCycleSeconds)
    {
        dialogueTimer_ = kDialogueCycleSeconds;
        dialoguePauseTimer_ = kDialoguePauseSeconds;
    }
}

void ShopView::handleMouseMove(sf::Vector2f position, const ShopSystem& shop,
                               const GameState& state, bool removingCard)
{
    hoveredAction_ = handleMouseClick(position, shop, state, removingCard);
    clearCardTooltip();

    if (removingCard || hoveredAction_.type != ShopActionType::BuyCard)
    {
        return;
    }

    const auto& offers = shop.getCardOffers();
    if (hoveredAction_.index >= offers.size() ||
        offers[hoveredAction_.index].sold)
    {
        return;
    }

    updateCardTooltip(offers[hoveredAction_.index].card,
                      cardBounds(hoveredAction_.index));
}

ShopAction ShopView::handleMouseClick(sf::Vector2f position, const ShopSystem& shop,
                                      const GameState& state, bool removingCard) const
{
    if (removingCard)
    {
        if (UiHelpers::contains(cancelRemoveButtonBounds(), position))
        {
            return {ShopActionType::CancelRemove, 0};
        }

        for (std::size_t index = 0; index < state.deck.size(); ++index)
        {
            if (UiHelpers::contains(deckCardBounds(index), position))
            {
                return {ShopActionType::RemoveCard, index};
            }
        }

        return {};
    }

    const auto& cardOffers = shop.getCardOffers();
    for (std::size_t index = 0; index < cardOffers.size(); ++index)
    {
        if (UiHelpers::contains(cardBounds(index), position))
        {
            return {ShopActionType::BuyCard, index};
        }
    }

    if (UiHelpers::contains(removeButtonBounds(), position))
    {
        return {ShopActionType::OpenRemove, 0};
    }

    if (UiHelpers::contains(leaveButtonBounds(), position))
    {
        return {ShopActionType::Leave, 0};
    }

    return {};
}

void ShopView::draw(sf::RenderWindow& window, const ShopSystem& shop,
                    const GameState& state, bool removingCard,
                    const std::string& message) const
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

        sf::RectangleShape veil({kWindowWidth, kWindowHeight});
        veil.setFillColor(sf::Color(12, 10, 9, 78));
        window.draw(veil);
    }
    else
    {
        sf::RectangleShape background({kWindowWidth, kWindowHeight});
        background.setFillColor(sf::Color(28, 27, 31));
        window.draw(background);
    }

    drawMerchant(window);

    if (font_ == nullptr)
    {
        return;
    }

    drawDialogueBubble(window);

    UiHelpers::drawText(window, *font_, "商店", 40, {60.0f, 52.0f},
                        sf::Color(239, 205, 118));
    UiHelpers::drawText(window, *font_, "金币 " + std::to_string(state.gold), 24,
                        {60.0f, 108.0f}, sf::Color(238, 202, 98));
    if (!merchantLoaded_)
    {
        UiHelpers::drawCenteredText(window, *font_, "商人位置", 28,
                                    {{80.0f, 330.0f}, {205.0f, 54.0f}},
                                    sf::Color(214, 205, 185));
    }

    if (removingCard)
    {
        UiHelpers::drawCenteredText(window, *font_, "选择一张牌删除", 30,
                                    {{360.0f, 52.0f}, {540.0f, 46.0f}},
                                    sf::Color(242, 226, 180));
        UiHelpers::drawText(window, *font_,
                            "删牌费用 " + std::to_string(shop.getRemoveCardPrice()),
                            22, {930.0f, 64.0f}, sf::Color(238, 202, 98));

        for (std::size_t index = 0; index < state.deck.size(); ++index)
        {
            Card card;
            try
            {
                card = CardDatabase::createFromInstance(state.deck[index]);
            }
            catch (const std::exception&)
            {
                continue;
            }

            const sf::FloatRect bounds = deckCardBounds(index);
            CardView cardView;
            cardView.setFont(*font_);
            cardView.setPosition({0.0f, 0.0f});
            sf::RenderTexture texture;
            if (!texture.resize({160, 220}))
            {
                continue;
            }
            texture.clear(sf::Color::Transparent);
            cardView.draw(texture, card);
            texture.display();
            sf::Sprite sprite(texture.getTexture());
            sprite.setPosition(bounds.position);
            sprite.setScale({kDeckCardScale, kDeckCardScale});
            window.draw(sprite);

            if (hoveredAction_.type == ShopActionType::RemoveCard &&
                hoveredAction_.index == index)
            {
                sf::RectangleShape outline(bounds.size);
                outline.setPosition(bounds.position);
                outline.setFillColor(sf::Color::Transparent);
                outline.setOutlineColor(sf::Color(244, 220, 150));
                outline.setOutlineThickness(3.0f);
                window.draw(outline);
            }
        }

        UiHelpers::drawButton(window, *font_, cancelRemoveButtonBounds(), "取消",
                              true, hoveredAction_.type == ShopActionType::CancelRemove);
    }
    else
    {
        UiHelpers::drawText(window, *font_, "卡牌", 28, {385.0f, 58.0f},
                            sf::Color(230, 220, 190));
        for (std::size_t index = 0; index < shop.getCardOffers().size(); ++index)
        {
            const ShopCardOffer& offer = shop.getCardOffers()[index];
            const sf::FloatRect bounds = cardBounds(index);

            CardView cardView;
            cardView.setFont(*font_);
            cardView.setPosition({0.0f, 0.0f});
            sf::RenderTexture texture;
            if (!texture.resize({160, 220}))
            {
                continue;
            }
            texture.clear(sf::Color::Transparent);
            cardView.draw(texture, offer.card);
            texture.display();
            sf::Sprite sprite(texture.getTexture());
            sprite.setPosition(bounds.position);
            sprite.setScale({kShopCardScale, kShopCardScale});
            sprite.setColor(offer.sold ? sf::Color(120, 120, 120, 130) : sf::Color::White);
            window.draw(sprite);

            if (hoveredAction_.type == ShopActionType::BuyCard &&
                hoveredAction_.index == index)
            {
                sf::RectangleShape outline(bounds.size);
                outline.setPosition(bounds.position);
                outline.setFillColor(sf::Color::Transparent);
                outline.setOutlineColor(sf::Color(244, 220, 150));
                outline.setOutlineThickness(3.0f);
                window.draw(outline);
            }

            UiHelpers::drawCenteredText(window, *font_,
                                        offer.sold ? "已售出" :
                                            std::to_string(offer.price) + " 金币",
                                        17,
                                        {{bounds.position.x, bounds.position.y + bounds.size.y + 6.0f},
                                         {bounds.size.x, 28.0f}},
                                        offer.sold ? sf::Color(160, 160, 160)
                                                   : priceColor(state.gold >= offer.price));
        }

        UiHelpers::drawButton(window, *font_, removeButtonBounds(),
                              "删除卡牌 " + std::to_string(shop.getRemoveCardPrice()),
                              !state.deck.empty() && state.gold >= shop.getRemoveCardPrice(),
                              hoveredAction_.type == ShopActionType::OpenRemove);
        UiHelpers::drawButton(window, *font_, leaveButtonBounds(), "离开",
                              true, hoveredAction_.type == ShopActionType::Leave);
    }

    drawCardTooltip(window);

    if (!message.empty())
    {
        UiHelpers::drawCenteredText(window, *font_, message, 20,
                                    {{350.0f, 665.0f}, {820.0f, 34.0f}},
                                    sf::Color(224, 216, 198));
    }
}

void ShopView::updateCardTooltip(const Card& card, const sf::FloatRect& bounds)
{
    if (font_ == nullptr)
    {
        return;
    }

    constexpr float kPanelWidth = 284.0f;
    constexpr float kSidePadding = 16.0f;
    constexpr float kLineHeight = 20.0f;
    const std::vector<std::string> wrappedLines =
        UiHelpers::wrapText(*font_, card.description, 15, kPanelWidth - 32.0f);

    cardTooltipSize_ = {
        kPanelWidth, 112.0f + static_cast<float>(wrappedLines.size()) * kLineHeight};
    cardTooltipPosition_ = BattleHover::computeTooltipPosition(
        bounds, cardTooltipSize_, {kWindowWidth, kWindowHeight});

    cardTooltipBackground_.setSize(cardTooltipSize_);
    cardTooltipBackground_.setPosition(cardTooltipPosition_);
    cardTooltipOutline_.setSize(cardTooltipSize_);
    cardTooltipOutline_.setPosition(cardTooltipPosition_);

    if (!cardTooltipNameText_.has_value() || !cardTooltipTypeText_.has_value() ||
        !cardTooltipCostText_.has_value() || !cardTooltipDescriptionText_.has_value())
    {
        return;
    }

    cardTooltipNameText_->setString(UiHelpers::toSfString(card.name));
    cardTooltipNameText_->setPosition({cardTooltipPosition_.x + kSidePadding,
                                       cardTooltipPosition_.y + 12.0f});
    cardTooltipTypeText_->setString(
        UiHelpers::toSfString("类型 " + cardTypeLabel(card.type)));
    cardTooltipTypeText_->setPosition({cardTooltipPosition_.x + kSidePadding,
                                       cardTooltipPosition_.y + 40.0f});

    cardTooltipCostText_->setString(UiHelpers::toSfString(
        card.cost < 0 ? "X" : std::to_string(card.cost)));
    cardTooltipCostCircle_.setPosition(
        {cardTooltipPosition_.x + cardTooltipSize_.x - 30.0f,
         cardTooltipPosition_.y + 26.0f});
    const sf::FloatRect costBounds = cardTooltipCostText_->getLocalBounds();
    cardTooltipCostText_->setPosition(
        {cardTooltipPosition_.x + cardTooltipSize_.x - 30.0f -
             costBounds.position.x - costBounds.size.x / 2.0f,
         cardTooltipPosition_.y + 26.0f - costBounds.position.y -
             costBounds.size.y / 2.0f - 2.0f});

    cardTooltipDescriptionText_->setString(
        UiHelpers::toSfString(joinLines(wrappedLines)));
    cardTooltipDescriptionText_->setPosition({cardTooltipPosition_.x + kSidePadding,
                                               cardTooltipPosition_.y + 66.0f});
    cardTooltipVisible_ = true;
}

void ShopView::clearCardTooltip()
{
    cardTooltipVisible_ = false;
}

void ShopView::drawCardTooltip(sf::RenderWindow& window) const
{
    if (!cardTooltipVisible_ || font_ == nullptr)
    {
        return;
    }

    window.draw(cardTooltipBackground_);
    window.draw(cardTooltipOutline_);
    if (!cardTooltipNameText_.has_value() || !cardTooltipTypeText_.has_value() ||
        !cardTooltipCostText_.has_value() || !cardTooltipDescriptionText_.has_value())
    {
        return;
    }

    window.draw(*cardTooltipNameText_);
    window.draw(*cardTooltipTypeText_);
    window.draw(cardTooltipCostCircle_);
    window.draw(*cardTooltipCostText_);
    window.draw(*cardTooltipDescriptionText_);
}

std::string ShopView::cardTypeLabel(CardType type) const
{
    switch (type)
    {
    case CardType::Attack:
        return "攻击";
    case CardType::Skill:
        return "技能";
    case CardType::Power:
        return "能力";
    }

    return "未知";
}

sf::FloatRect ShopView::cardBounds(std::size_t index) const
{
    const float width = 160.0f * kShopCardScale;
    const float height = 220.0f * kShopCardScale;
    const float x = 350.0f + static_cast<float>(index % 4) * 150.0f;
    const float y = 112.0f + static_cast<float>(index / 4) * 245.0f;
    return {{x, y}, {width, height}};
}

void ShopView::chooseNextDialogue()
{
    static const std::array<std::string, 2> lines = {
        "不来点什么？",
        "超实惠！"
    };

    std::uniform_int_distribution<std::size_t> distribution(0, lines.size() - 1);
    dialogueText_ = lines[distribution(dialogueRandomEngine_)];
}

float ShopView::dialogueAlpha() const
{
    if (dialoguePauseTimer_ > 0.0f)
    {
        return 0.0f;
    }

    if (dialogueTimer_ < kDialogueFadeInSeconds)
    {
        return std::clamp(dialogueTimer_ / kDialogueFadeInSeconds, 0.0f, 1.0f);
    }

    if (dialogueTimer_ < kDialogueFadeInSeconds + kDialogueVisibleSeconds)
    {
        return 1.0f;
    }

    const float fadeOutTime =
        dialogueTimer_ - kDialogueFadeInSeconds - kDialogueVisibleSeconds;
    return std::clamp(1.0f - fadeOutTime / kDialogueFadeOutSeconds, 0.0f, 1.0f);
}

void ShopView::drawMerchant(sf::RenderWindow& window) const
{
    if (merchantLoaded_)
    {
        const sf::Texture& texture = merchantFrames_[merchantFrameIndex_ % merchantFrames_.size()];
        sf::Sprite merchant(texture);
        const sf::FloatRect bounds = merchant.getLocalBounds();
        const float scale = std::min(260.0f / bounds.size.x, 440.0f / bounds.size.y);
        merchant.setScale({scale, scale});
        merchant.setPosition({60.0f, 185.0f});
        window.draw(merchant);
        return;
    }

    sf::RectangleShape merchant({245.0f, 430.0f});
    merchant.setPosition({60.0f, 160.0f});
    merchant.setFillColor(sf::Color(54, 48, 57));
    merchant.setOutlineColor(sf::Color(186, 154, 88));
    merchant.setOutlineThickness(3.0f);
    window.draw(merchant);
}

void ShopView::drawDialogueBubble(sf::RenderWindow& window) const
{
    if (font_ == nullptr)
    {
        return;
    }

    const float alphaRatio = dialogueAlpha();
    if (alphaRatio <= 0.0f)
    {
        return;
    }

    const auto alpha = static_cast<std::uint8_t>(std::round(255.0f * alphaRatio));
    sf::FloatRect bubbleBounds({66.0f, 96.0f}, {280.0f, 72.0f});
    sf::RectangleShape bubble(bubbleBounds.size);
    bubble.setPosition(bubbleBounds.position);
    bubble.setFillColor(sf::Color(246, 242, 232, alpha));
    bubble.setOutlineColor(sf::Color(38, 30, 22, alpha));
    bubble.setOutlineThickness(3.0f);
    window.draw(bubble);

    sf::CircleShape tail(13.0f, 3);
    tail.setFillColor(sf::Color(246, 242, 232, alpha));
    tail.setPosition({142.0f, 157.0f});
    tail.setRotation(sf::degrees(30.0f));
    window.draw(tail);

    sf::Text line = UiHelpers::makeText(*font_, dialogueText_, 24,
                                        sf::Color(24, 20, 16, alpha));
    const sf::FloatRect textBounds = line.getLocalBounds();
    line.setPosition({bubbleBounds.position.x +
                          (bubbleBounds.size.x - textBounds.size.x) / 2.0f -
                          textBounds.position.x,
                      bubbleBounds.position.y +
                          (bubbleBounds.size.y - textBounds.size.y) / 2.0f -
                          textBounds.position.y - 2.0f});
    window.draw(line);
}

sf::FloatRect ShopView::removeButtonBounds() const
{
    return {{950.0f, 485.0f}, {210.0f, 64.0f}};
}

sf::FloatRect ShopView::leaveButtonBounds() const
{
    return {{950.0f, 592.0f}, {210.0f, 64.0f}};
}

sf::FloatRect ShopView::cancelRemoveButtonBounds() const
{
    return {{970.0f, 600.0f}, {180.0f, 58.0f}};
}

sf::FloatRect ShopView::deckCardBounds(std::size_t index) const
{
    const float width = 160.0f * kDeckCardScale;
    const float height = 220.0f * kDeckCardScale;
    const float x = 360.0f + static_cast<float>(index % 6) * 96.0f;
    const float y = 128.0f + static_cast<float>(index / 6) * 140.0f;
    return {{x, y}, {width, height}};
}
