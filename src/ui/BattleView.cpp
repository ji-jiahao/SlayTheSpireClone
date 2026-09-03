#include "ui/BattleView.hpp"

#include "ui/CardView.hpp"
#include "ui/UiHelpers.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <string>

namespace
{
// 战斗界面按固定 1280x720 逻辑分辨率排版，与其它界面保持一致。
constexpr float kWindowWidth = 1280.0f;
constexpr float kWindowHeight = 720.0f;

constexpr float kHandGap = 20.0f;
constexpr float kHandY = 470.0f;
constexpr float kHoverTransitionSeconds = 0.15f;
constexpr float kHoverLift = -20.0f;
constexpr float kHoverScale = 1.15f;
constexpr float kSelectionLift = -15.0f;
constexpr float kSelectionScale = 1.10f;
constexpr float kSelectionTimeoutSeconds = 5.0f;
constexpr float kPlayArcHeight = 72.0f;
constexpr float kPlayScaleEnd = 0.84f;
constexpr const char* kHoverCardSoundPath = "assets/sounds/card_select.mp3";
constexpr const char* kAttackCardSoundPath = "assets/sounds/card_attack.mp3";
constexpr const char* kDefenseCardSoundPath = "assets/sounds/card_defense.mp3";

sf::Vector2f playerFocusPoint()
{
    return {223.0f, 313.0f};
}

sf::Vector2f enemyFocusPoint()
{
    return {1025.0f, 310.0f};
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

void drawShadow(sf::RenderTarget& target, sf::Vector2f center, sf::Vector2f size,
                std::uint8_t alpha)
{
    sf::CircleShape shadow(1.0f, 40);
    shadow.setOrigin({1.0f, 1.0f});
    shadow.setScale(size);
    shadow.setPosition(center);
    shadow.setFillColor(sf::Color(0, 0, 0, alpha));
    target.draw(shadow);
}
} // namespace

BattleView::BattleView()
    : font_(nullptr),
      background_(nullptr),
      hoverCardSoundLoaded_(false),
      attackCardSoundLoaded_(false),
      defenseCardSoundLoaded_(false)
{
    hoverTextureReady_ = hoveredCardTexture_.resize({160, 220}) &&
                        fadingCardTexture_.resize({160, 220});

    hoverPanelBackground_.setFillColor(sf::Color(27, 25, 30, 236));
    hoverPanelBackground_.setOutlineThickness(0.0f);
    hoverPanelOutline_.setFillColor(sf::Color::Transparent);
    hoverPanelOutline_.setOutlineColor(sf::Color(244, 216, 150, 120));
    hoverPanelOutline_.setOutlineThickness(3.0f);

    hoverCostCircle_.setRadius(15.0f);
    hoverCostCircle_.setPointCount(32);
    hoverCostCircle_.setFillColor(sf::Color(244, 232, 198));
    hoverCostCircle_.setOutlineColor(sf::Color(52, 42, 30));
    hoverCostCircle_.setOutlineThickness(2.0f);
    hoverCostCircle_.setOrigin({15.0f, 15.0f});

    loadCardSounds();
}

void BattleView::setFont(const sf::Font& font)
{
    font_ = &font;

    hoverNameText_.emplace(font, UiHelpers::toSfString(""), 22);
    hoverNameText_->setFillColor(sf::Color(246, 239, 224));

    hoverTypeText_.emplace(font, UiHelpers::toSfString(""), 16);
    hoverTypeText_->setFillColor(sf::Color(218, 208, 190));

    hoverCostText_.emplace(font, UiHelpers::toSfString(""), 18);
    hoverCostText_->setFillColor(sf::Color(44, 34, 24));

    hoverDescriptionText_.emplace(font, UiHelpers::toSfString(""), 15);
    hoverDescriptionText_->setFillColor(sf::Color(230, 222, 208));

    if (hoveredCard_.active)
    {
        updateHoverCardTexture(hoveredCardTexture_, hoveredCard_.card);
        updateHoverPanel(hoveredCard_.card, hoveredCard_.bounds);
    }

    if (fadingCard_.active)
    {
        updateHoverCardTexture(fadingCardTexture_, fadingCard_.card);
    }
}

void BattleView::setBackground(const sf::Texture& texture)
{
    background_ = &texture;
}

void BattleView::reset()
{
    handState_ = HandState::Idle;
    hoveredCard_ = {};
    fadingCard_ = {};
    selectedCard_ = {};
    selectedTargetHovered_ = false;
    activePlays_.clear();
    activeBursts_.clear();
    selectionTimer_ = 0.0f;
    hoverPanelVisible_ = false;
}

void BattleView::update(float deltaSeconds)
{
    updateActiveVisuals(deltaSeconds);
}

void BattleView::handleMouseMove(sf::Vector2f mousePosition, const CombatSystem& combat)
{
    if (combat.getResult() != BattleResult::Active)
    {
        clearHoverVisual();
        clearTargetSelection();
        return;
    }

    if (handState_ == HandState::SelectingTarget)
    {
        updateSelectionTargetHover(mousePosition);
        return;
    }

    const std::vector<Card>& hand = combat.getHandCards();
    const std::vector<BattleHover::HandCardLayout> layouts =
        BattleHover::layoutHandCards(hand, CardView::getCardSize(), kWindowWidth, kHandY,
                                     kHandGap);
    const int handIndex = BattleHover::pickHoveredCardIndex(mousePosition, layouts);

    if (handIndex == hoveredCard_.handIndex)
    {
        return;
    }

    beginHoverVisual(hand, handIndex,
                     handIndex >= 0
                         ? layouts[static_cast<std::size_t>(handIndex)].bounds
                         : sf::FloatRect());
}

void BattleView::handleMouseClick(sf::Vector2f mousePosition, CombatSystem& combat)
{
    clearHoverVisual();

    if (combat.getResult() != BattleResult::Active)
    {
        return;
    }

    if (handState_ == HandState::SelectingTarget)
    {
        if (getSelectionTargetBounds(selectedCard_.targetKind).contains(mousePosition))
        {
            const Card playedCard = selectedCard_.card;
            const sf::Vector2f startPos = selectedCard_.bounds.position;
            const BattleTargetKind targetKind = selectedCard_.targetKind;

            if (combat.playCard(selectedCard_.handIndex))
            {
                startPlayAnimation(playedCard, startPos, targetKind);
                playCardSound(playedCard);
            }

            clearTargetSelection();
            return;
        }

        return;
    }

    const std::vector<Card>& hand = combat.getHandCards();
    const std::vector<BattleHover::HandCardLayout> layouts =
        BattleHover::layoutHandCards(hand, CardView::getCardSize(), kWindowWidth, kHandY,
                                     kHandGap);
    for (const BattleHover::HandCardLayout& layout : layouts)
    {
        if (!layout.bounds.contains(mousePosition))
        {
            continue;
        }

        const Card card = hand[static_cast<std::size_t>(layout.handIndex)];
        if (card.cost < 0)
        {
            return;
        }

        if (combat.getPlayer().getCurrentEnergy() < card.cost)
        {
            return;
        }

        if (BattleCast::requiresTargetSelection(card))
        {
            beginTargetSelection(hand, layout.handIndex, layout.bounds);
            updateSelectionTargetHover(mousePosition);
        }
        else if (combat.playCard(layout.handIndex))
        {
            startPlayAnimation(card, layout.bounds.position, BattleCast::resolveTargetKind(card));
            playCardSound(card);
        }
        return;
    }

    if (getEndTurnButtonBounds().contains(mousePosition))
    {
        combat.endPlayerTurn();
    }
}

bool BattleView::handleKeyPress(sf::Keyboard::Key key, CombatSystem& combat)
{
    (void)combat;
    if (handState_ != HandState::SelectingTarget)
    {
        return false;
    }

    if (key == sf::Keyboard::Key::Escape)
    {
        clearTargetSelection();
        return true;
    }

    return false;
}

sf::FloatRect BattleView::getEndTurnButtonBounds() const
{
    return {{kWindowWidth - 170.0f, 620.0f}, {140.0f, 60.0f}};
}

bool BattleView::isVisualLocked() const
{
    return handState_ != HandState::Idle || !activePlays_.empty() || !activeBursts_.empty();
}

void BattleView::beginHoverVisual(const std::vector<Card>& hand, int handIndex,
                                  const sf::FloatRect& bounds)
{
    if (handIndex < 0)
    {
        if (hoveredCard_.active)
        {
            fadingCard_ = hoveredCard_;
            fadingCard_.active = true;
            if (fadingCard_.progress <= 0.0f)
            {
                fadingCard_.progress = 1.0f;
            }
        }

        hoveredCard_.active = false;
        hoveredCard_.handIndex = -1;
        hoveredCard_.progress = 0.0f;
        hoverPanelVisible_ = false;
        return;
    }

    if (hoveredCard_.active)
    {
        fadingCard_ = hoveredCard_;
        fadingCard_.active = true;
        if (fadingCard_.progress <= 0.0f)
        {
            fadingCard_.progress = 1.0f;
        }
    }

    hoveredCard_.handIndex = handIndex;
    hoveredCard_.card = hand[static_cast<std::size_t>(handIndex)];
    hoveredCard_.bounds = bounds;
    hoveredCard_.progress = 0.0f;
    hoveredCard_.active = true;

    updateHoverCardTexture(hoveredCardTexture_, hoveredCard_.card);
    updateHoverPanel(hoveredCard_.card, hoveredCard_.bounds);
    playHoverSound();
}

void BattleView::clearHoverVisual()
{
    if (hoveredCard_.active)
    {
        fadingCard_ = hoveredCard_;
        fadingCard_.active = true;
        if (fadingCard_.progress <= 0.0f)
        {
            fadingCard_.progress = 1.0f;
        }
    }

    hoveredCard_.active = false;
    hoveredCard_.handIndex = -1;
    hoveredCard_.progress = 0.0f;
    hoverPanelVisible_ = false;
}

void BattleView::beginTargetSelection(const std::vector<Card>& hand, int handIndex,
                                      const sf::FloatRect& bounds)
{
    clearHoverVisual();

    selectedCard_.handIndex = handIndex;
    selectedCard_.card = hand[static_cast<std::size_t>(handIndex)];
    selectedCard_.bounds = bounds;
    selectedCard_.targetKind = BattleCast::resolveTargetKind(selectedCard_.card);
    selectedCard_.progress = 0.0f;
    selectedCard_.active = true;
    handState_ = HandState::SelectingTarget;
    selectedTargetHovered_ = false;
    selectionTimer_ = 0.0f;
}

void BattleView::clearTargetSelection()
{
    selectedCard_.active = false;
    selectedCard_.handIndex = -1;
    selectedCard_.progress = 0.0f;
    selectedTargetHovered_ = false;
    selectionTimer_ = 0.0f;
    handState_ = activePlays_.empty() ? HandState::Idle : HandState::Playing;
}

void BattleView::startPlayAnimation(const Card& card, sf::Vector2f startPos,
                                    BattleTargetKind targetKind)
{
    PlayAnim anim;
    anim.card = card;
    anim.startPos = {startPos.x, startPos.y};
    anim.targetPos = getTargetFocusPoint(targetKind);
    activePlays_.push_back(anim);
    handState_ = HandState::Playing;
}

void BattleView::updateActiveVisuals(float deltaSeconds)
{
    const float hoverStep = std::min(1.0f, deltaSeconds / kHoverTransitionSeconds);

    if (hoveredCard_.active)
    {
        hoveredCard_.progress = std::min(1.0f, hoveredCard_.progress + hoverStep);
    }

    if (fadingCard_.active)
    {
        fadingCard_.progress = std::max(0.0f, fadingCard_.progress - hoverStep);
        if (fadingCard_.progress <= 0.0f)
        {
            fadingCard_.active = false;
            fadingCard_.handIndex = -1;
        }
    }

    if (selectedCard_.active)
    {
        selectedCard_.progress = std::min(1.0f, selectedCard_.progress + hoverStep);
        selectionTimer_ += deltaSeconds;
        if (selectionTimer_ >= kSelectionTimeoutSeconds)
        {
            clearTargetSelection();
        }
    }

    for (PlayAnim& anim : activePlays_)
    {
        if (anim.finished)
        {
            continue;
        }

        anim.progress = std::min(1.0f, anim.progress + deltaSeconds / anim.duration);
        if (anim.progress >= 1.0f)
        {
            anim.finished = true;
            activeBursts_.push_back({anim.targetPos});
        }
    }

    activePlays_.erase(
        std::remove_if(activePlays_.begin(), activePlays_.end(),
                       [](const PlayAnim& anim) { return anim.finished; }),
        activePlays_.end());

    for (HitBurst& burst : activeBursts_)
    {
        burst.progress = std::min(1.0f, burst.progress + deltaSeconds / burst.duration);
    }

    activeBursts_.erase(
        std::remove_if(activeBursts_.begin(), activeBursts_.end(),
                       [](const HitBurst& burst) { return burst.progress >= 1.0f; }),
        activeBursts_.end());

    if (handState_ == HandState::Playing && activePlays_.empty() && !selectedCard_.active)
    {
        handState_ = HandState::Idle;
    }

    if (handState_ == HandState::SelectingTarget && !selectedCard_.active)
    {
        handState_ = HandState::Idle;
    }
}

void BattleView::updateHoverCardTexture(sf::RenderTexture& texture, const Card& card) const
{
    if (!hoverTextureReady_)
    {
        return;
    }

    texture.clear(sf::Color::Transparent);
    CardView cardView;
    if (font_ != nullptr)
    {
        cardView.setFont(*font_);
    }
    cardView.setPosition({0.0f, 0.0f});
    cardView.draw(texture, card);
    texture.display();
}

void BattleView::updateHoverPanel(const Card& card, const sf::FloatRect& bounds)
{
    if (font_ == nullptr)
    {
        hoverPanelVisible_ = false;
        return;
    }

    constexpr float kPanelWidth = 284.0f;
    constexpr float kSidePadding = 16.0f;
    constexpr float kTopPadding = 12.0f;
    constexpr float kLineHeight = 20.0f;

    const std::vector<std::string> wrappedLines =
        UiHelpers::wrapText(*font_, card.description, 15, kPanelWidth - 32.0f);
    const std::string description = joinLines(wrappedLines);
    const float panelHeight = 112.0f + static_cast<float>(wrappedLines.size()) * kLineHeight;

    hoverPanelSize_ = {kPanelWidth, panelHeight};
    hoverPanelPosition_ = BattleHover::computeTooltipPosition(
        bounds, hoverPanelSize_, {kWindowWidth, kWindowHeight});

    hoverPanelOutline_.setSize(hoverPanelSize_);
    hoverPanelOutline_.setPosition(hoverPanelPosition_);

    hoverPanelBackground_.setSize(hoverPanelSize_);
    hoverPanelBackground_.setPosition(hoverPanelPosition_);

    if (!hoverNameText_.has_value() || !hoverTypeText_.has_value() ||
        !hoverCostText_.has_value() || !hoverDescriptionText_.has_value())
    {
        return;
    }

    hoverNameText_->setString(UiHelpers::toSfString(card.name));
    hoverNameText_->setPosition({hoverPanelPosition_.x + kSidePadding,
                                 hoverPanelPosition_.y + kTopPadding});

    hoverTypeText_->setString(UiHelpers::toSfString("类型 " + cardTypeLabel(card.type)));
    hoverTypeText_->setPosition({hoverPanelPosition_.x + kSidePadding,
                                 hoverPanelPosition_.y + 40.0f});

    const std::string costLabel = card.cost < 0 ? "X" : std::to_string(card.cost);
    hoverCostText_->setString(UiHelpers::toSfString(costLabel));
    hoverCostCircle_.setPosition({hoverPanelPosition_.x + hoverPanelSize_.x - 30.0f,
                                  hoverPanelPosition_.y + 26.0f});
    const sf::FloatRect costBounds = hoverCostText_->getLocalBounds();
    hoverCostText_->setPosition({hoverPanelPosition_.x + hoverPanelSize_.x - 30.0f -
                                     costBounds.position.x - costBounds.size.x / 2.0f,
                                 hoverPanelPosition_.y + 26.0f - costBounds.position.y -
                                     costBounds.size.y / 2.0f - 2.0f});

    hoverDescriptionText_->setString(UiHelpers::toSfString(description));
    hoverDescriptionText_->setPosition({hoverPanelPosition_.x + kSidePadding,
                                        hoverPanelPosition_.y + 66.0f});

    hoverPanelVisible_ = true;
}

void BattleView::playHoverSound()
{
    if (!hoverCardSoundLoaded_)
    {
        return;
    }

    hoverCardSound_.stop();
    hoverCardSound_.play();
}

std::string BattleView::cardTypeLabel(CardType type) const
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

sf::FloatRect BattleView::getSelectionTargetBounds(BattleTargetKind targetKind) const
{
    switch (targetKind)
    {
    case BattleTargetKind::Self:
        return {{155.0f, 245.0f}, {136.0f, 136.0f}};
    case BattleTargetKind::Enemy:
        return {{950.0f, 235.0f}, {150.0f, 150.0f}};
    }

    return {{0.0f, 0.0f}, {0.0f, 0.0f}};
}

sf::Vector2f BattleView::getTargetFocusPoint(BattleTargetKind targetKind) const
{
    return targetKind == BattleTargetKind::Self ? playerFocusPoint() : enemyFocusPoint();
}

void BattleView::updateSelectionTargetHover(sf::Vector2f mousePosition)
{
    selectedTargetHovered_ = getSelectionTargetBounds(selectedCard_.targetKind).contains(mousePosition);
}

void BattleView::drawHoverVisual(sf::RenderTarget& target, const HoverCardVisual& visual,
                                 const sf::RenderTexture& texture, bool isHovered) const
{
    if (!hoverTextureReady_)
    {
        return;
    }

    if (!visual.active && visual.progress <= 0.0f)
    {
        return;
    }

    const float eased = BattleHover::easeOutCubic(visual.progress);
    const float scale = 1.0f + (kHoverScale - 1.0f) * eased;
    const float lift = kHoverLift * eased;
    const sf::Vector2f cardCenter{visual.bounds.position.x + visual.bounds.size.x / 2.0f,
                                  visual.bounds.position.y + visual.bounds.size.y / 2.0f +
                                      lift};

    drawShadow(target, {cardCenter.x, visual.bounds.position.y + visual.bounds.size.y +
                                     20.0f + lift * 0.2f},
               {visual.bounds.size.x * (0.48f + 0.12f * eased),
                visual.bounds.size.y * (0.09f + 0.05f * eased)},
               static_cast<std::uint8_t>(84.0f + 40.0f * eased));

    if (isHovered)
    {
        sf::RectangleShape glow({visual.bounds.size.x * scale + 10.0f,
                                 visual.bounds.size.y * scale + 10.0f});
        glow.setOrigin({glow.getSize().x / 2.0f, glow.getSize().y / 2.0f});
        glow.setPosition(cardCenter);
        glow.setFillColor(sf::Color::Transparent);
        glow.setOutlineColor(sf::Color(255, 245, 220,
                                       static_cast<std::uint8_t>(95.0f * eased)));
        glow.setOutlineThickness(3.0f);
        target.draw(glow);
    }

    sf::Sprite sprite(texture.getTexture());
    const sf::FloatRect localBounds = sprite.getLocalBounds();
    sprite.setOrigin({localBounds.position.x + localBounds.size.x / 2.0f,
                      localBounds.position.y + localBounds.size.y / 2.0f});
    sprite.setPosition(cardCenter);
    sprite.setScale({scale, scale});
    target.draw(sprite);

    if (isHovered && hoverPanelVisible_)
    {
        drawHoverPanel(target);
    }
}

void BattleView::drawHandCard(sf::RenderTarget& target, const Card& card,
                              sf::Vector2f position, float scale, float rotation) const
{
    CardView cardView;
    if (font_ != nullptr)
    {
        cardView.setFont(*font_);
    }
    cardView.setPosition(position);
    cardView.setScale(scale);
    cardView.setRotation(rotation);
    cardView.draw(target, card);
}

void BattleView::drawHoverPanel(sf::RenderTarget& target) const
{
    if (!hoverPanelVisible_)
    {
        return;
    }

    target.draw(hoverPanelBackground_);
    target.draw(hoverPanelOutline_);
    if (!hoverNameText_.has_value() || !hoverTypeText_.has_value() ||
        !hoverCostText_.has_value() || !hoverDescriptionText_.has_value())
    {
        return;
    }

    target.draw(*hoverNameText_);
    target.draw(*hoverTypeText_);
    target.draw(hoverCostCircle_);
    target.draw(*hoverCostText_);
    target.draw(*hoverDescriptionText_);
}

void BattleView::drawTargetHighlight(sf::RenderTarget& target) const
{
    if (handState_ != HandState::SelectingTarget || !selectedCard_.active ||
        !selectedTargetHovered_)
    {
        return;
    }

    const sf::Vector2f center = getTargetFocusPoint(selectedCard_.targetKind);
    const bool enemyTarget = selectedCard_.targetKind == BattleTargetKind::Enemy;
    const sf::Vector2f size = enemyTarget ? sf::Vector2f{220.0f, 220.0f}
                                          : sf::Vector2f{210.0f, 210.0f};

    sf::RectangleShape glow(size);
    glow.setOrigin({size.x / 2.0f, size.y / 2.0f});
    glow.setPosition(center);
    glow.setFillColor(sf::Color(96, 220, 128, 38));
    glow.setOutlineColor(sf::Color(136, 255, 175, 160));
    glow.setOutlineThickness(4.0f);
    target.draw(glow);

    sf::CircleShape ring(enemyTarget ? 88.0f : 80.0f, 48);
    ring.setOrigin({ring.getRadius(), ring.getRadius()});
    ring.setPosition(center);
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineColor(sf::Color(188, 255, 206, 175));
    ring.setOutlineThickness(5.0f);
    target.draw(ring);
}

void BattleView::drawTargetSelectionOverlay(sf::RenderWindow& window) const
{
    drawTargetHighlight(window);
}

void BattleView::loadCardSounds()
{
    hoverCardSoundLoaded_ = hoverCardSound_.openFromFile(kHoverCardSoundPath);
    if (hoverCardSoundLoaded_)
    {
        hoverCardSound_.setVolume(90.0f);
        hoverCardSound_.setLooping(false);
    }

    attackCardSoundLoaded_ = attackCardSound_.openFromFile(kAttackCardSoundPath);
    if (attackCardSoundLoaded_)
    {
        attackCardSound_.setVolume(100.0f);
        attackCardSound_.setLooping(false);
    }

    defenseCardSoundLoaded_ = defenseCardSound_.openFromFile(kDefenseCardSoundPath);
    if (defenseCardSoundLoaded_)
    {
        defenseCardSound_.setVolume(100.0f);
        defenseCardSound_.setLooping(false);
    }
}

void BattleView::playCardSound(const Card& card)
{
    if (card.type == CardType::Attack && attackCardSoundLoaded_)
    {
        defenseCardSound_.stop();
        attackCardSound_.stop();
        attackCardSound_.play();
        return;
    }

    if (card.type == CardType::Skill && defenseCardSoundLoaded_)
    {
        attackCardSound_.stop();
        defenseCardSound_.stop();
        defenseCardSound_.play();
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

    sf::CircleShape enemyBody(75.0f, 7);
    enemyBody.setPosition({950.0f, 235.0f});
    enemyBody.setFillColor(sf::Color(58, 50, 70, 235));
    enemyBody.setOutlineColor(sf::Color(183, 95, 84));
    enemyBody.setOutlineThickness(4.0f);
    window.draw(enemyBody);

    drawPlayerPanel(window, combat.getPlayer());
    drawEnemyPanel(window, combat.getEnemy());
    drawHand(window, combat.getHandCards());
    drawEndTurnButton(window);

    for (const PlayAnim& anim : activePlays_)
    {
        drawPlayAnim(window, anim);
    }

    for (const HitBurst& burst : activeBursts_)
    {
        drawHitBurst(window, burst);
    }

    if (handState_ == HandState::SelectingTarget && selectedCard_.active)
    {
        drawTargetSelectionOverlay(window);
    }

    if (font_ != nullptr)
    {
        sf::Text piles =
            UiHelpers::makeText(*font_,
                                "抽牌 " + std::to_string(combat.getDeck().getDrawPile().size()) +
                                    "   弃牌 " +
                                    std::to_string(combat.getDeck().getDiscardPile().size()) +
                                    "   消耗 " +
                                    std::to_string(combat.getDeck().getExhaustPile().size()),
                                17, sf::Color(225, 220, 210));
        piles.setPosition({35.0f, 675.0f});
        window.draw(piles);
    }
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
        sf::Text title = UiHelpers::makeText(*font_, "玩家", 24, sf::Color(235, 229, 207));
        title.setPosition({58.0f, 52.0f});
        window.draw(title);

        sf::Text hpText =
            UiHelpers::makeText(*font_, "HP " + std::to_string(player.getCurrentHealth()) +
                                             "/" + std::to_string(player.getMaxHealth()),
                                18, sf::Color(222, 210, 190));
        hpText.setPosition({58.0f, 92.0f});
        window.draw(hpText);

        sf::Text energyText =
            UiHelpers::makeText(*font_, "能量 " + std::to_string(player.getCurrentEnergy()) +
                                             "/" + std::to_string(player.getMaxEnergy()),
                                18, sf::Color(240, 200, 120));
        energyText.setPosition({250.0f, 92.0f});
        window.draw(energyText);

        sf::Text blockText =
            UiHelpers::makeText(*font_, "格挡 " + std::to_string(player.getBlock()), 18,
                                sf::Color(140, 180, 230));
        blockText.setPosition({58.0f, 140.0f});
        window.draw(blockText);

        const std::string status = "力量 " + std::to_string(player.getStrength()) +
                                   "  虚弱 " + std::to_string(player.getWeak()) +
                                   "  易伤 " + std::to_string(player.getVulnerable());
        sf::Text statusText = UiHelpers::makeText(*font_, status, 15,
                                                   sf::Color(190, 190, 200));
        statusText.setPosition({150.0f, 143.0f});
        window.draw(statusText);
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
        sf::Text title =
            UiHelpers::makeText(*font_, enemy.getName(), 24, sf::Color(235, 229, 207));
        title.setPosition({kWindowWidth - 402.0f, 52.0f});
        window.draw(title);

        sf::Text hpText =
            UiHelpers::makeText(*font_, "HP " + std::to_string(enemy.getCurrentHealth()) +
                                             "/" + std::to_string(enemy.getMaxHealth()),
                                18, sf::Color(222, 210, 190));
        hpText.setPosition({kWindowWidth - 402.0f, 92.0f});
        window.draw(hpText);

        sf::Text intentText =
            UiHelpers::makeText(*font_, "意图 " + std::to_string(enemy.getIntentDamage()), 18,
                                sf::Color(230, 120, 110));
        intentText.setPosition({kWindowWidth - 402.0f, 140.0f});
        window.draw(intentText);

        const std::string status = "力量 " + std::to_string(enemy.getStrength()) +
                                   "  虚弱 " + std::to_string(enemy.getWeak()) +
                                   "  易伤 " + std::to_string(enemy.getVulnerable());
        sf::Text statusText = UiHelpers::makeText(*font_, status, 15,
                                                   sf::Color(190, 190, 200));
        statusText.setPosition({kWindowWidth - 285.0f, 143.0f});
        window.draw(statusText);
    }

    const float hpRatio = maxHealth > 0.0f ? currentHealth / maxHealth : 0.0f;
    drawBar(window, {kWindowWidth - 402.0f, 122.0f}, {180.0f, 16.0f}, hpRatio,
            sf::Color(196, 70, 60), sf::Color(60, 40, 40));
}

void BattleView::drawHand(sf::RenderWindow& window, const std::vector<Card>& hand) const
{
    const std::vector<BattleHover::HandCardLayout> layouts =
        BattleHover::layoutHandCards(hand, CardView::getCardSize(), kWindowWidth, kHandY,
                                     kHandGap);

    for (const BattleHover::HandCardLayout& layout : layouts)
    {
        if ((hoveredCard_.active && layout.handIndex == hoveredCard_.handIndex) ||
            (fadingCard_.active && layout.handIndex == fadingCard_.handIndex) ||
            (selectedCard_.active && layout.handIndex == selectedCard_.handIndex))
        {
            continue;
        }

        drawHandCard(window, hand[static_cast<std::size_t>(layout.handIndex)],
                     layout.bounds.position, 1.0f, 0.0f);
    }

    if (fadingCard_.active && fadingCard_.progress > 0.0f)
    {
        drawHoverVisual(window, fadingCard_, fadingCardTexture_, false);
    }

    if (hoveredCard_.active && hoveredCard_.progress > 0.0f)
    {
        drawHoverVisual(window, hoveredCard_, hoveredCardTexture_, true);
    }

    if (selectedCard_.active)
    {
        const float eased = BattleHover::easeOutCubic(selectedCard_.progress);
        const float scale = 1.0f + (kSelectionScale - 1.0f) * eased;
        const float lift = kSelectionLift * eased;
        const sf::Vector2f liftedPosition{selectedCard_.bounds.position.x,
                                          selectedCard_.bounds.position.y + lift};
        drawShadow(window,
                   {liftedPosition.x + selectedCard_.bounds.size.x / 2.0f,
                    liftedPosition.y + selectedCard_.bounds.size.y + 16.0f},
                   {selectedCard_.bounds.size.x * (0.46f + 0.08f * eased),
                    selectedCard_.bounds.size.y * (0.09f + 0.04f * eased)},
                   static_cast<std::uint8_t>(80.0f + 40.0f * eased));
        drawHandCard(window, selectedCard_.card, liftedPosition, scale, 0.0f);
    }
}

void BattleView::drawPlayAnim(sf::RenderTarget& target, const PlayAnim& anim) const
{
    const float eased = BattleCast::easeInOutQuad(anim.progress);
    const sf::Vector2f center = BattleCast::lerp(anim.startPos + sf::Vector2f{80.0f, 110.0f},
                                                 anim.targetPos, eased);
    const float arc = std::sin(anim.progress * 3.14159265f) * kPlayArcHeight;
    const sf::Vector2f offset{0.0f, -arc};
    const float scale = 1.0f + (kPlayScaleEnd - 1.0f) * eased;
    const float rotation = anim.rotationStart + (anim.rotationEnd - anim.rotationStart) * eased;
    const sf::Vector2f topLeft{center.x + offset.x - CardView::getCardSize().x * scale / 2.0f,
                               center.y + offset.y - CardView::getCardSize().y * scale / 2.0f};

    drawShadow(target, {center.x + offset.x, center.y + offset.y + 20.0f},
               {CardView::getCardSize().x * (0.46f + 0.06f * eased),
                CardView::getCardSize().y * (0.09f + 0.04f * eased)},
               static_cast<std::uint8_t>(86.0f + 42.0f * eased));
    drawHandCard(target, anim.card, topLeft, scale, rotation);
}

void BattleView::drawHitBurst(sf::RenderTarget& target, const HitBurst& burst) const
{
    const float eased = BattleCast::easeInOutQuad(burst.progress);
    const float fade = 1.0f - eased;

    sf::CircleShape flash(20.0f + 26.0f * eased, 40);
    flash.setOrigin({flash.getRadius(), flash.getRadius()});
    flash.setPosition(burst.position);
    flash.setFillColor(sf::Color(255, 248, 225, static_cast<std::uint8_t>(170.0f * fade)));
    target.draw(flash);

    sf::CircleShape ring(28.0f + 44.0f * eased, 40);
    ring.setOrigin({ring.getRadius(), ring.getRadius()});
    ring.setPosition(burst.position);
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineColor(sf::Color(255, 240, 180, static_cast<std::uint8_t>(200.0f * fade)));
    ring.setOutlineThickness(4.0f);
    target.draw(ring);

    static constexpr std::array<sf::Vector2f, 6> directions = {
        sf::Vector2f{1.0f, 0.0f}, sf::Vector2f{0.5f, 0.87f}, sf::Vector2f{-0.5f, 0.87f},
        sf::Vector2f{-1.0f, 0.0f}, sf::Vector2f{-0.5f, -0.87f}, sf::Vector2f{0.5f, -0.87f},
    };

    for (std::size_t index = 0; index < directions.size(); ++index)
    {
        sf::RectangleShape shard({18.0f, 3.0f});
        shard.setOrigin({9.0f, 1.5f});
        shard.setPosition({burst.position.x + directions[index].x * (16.0f + 26.0f * eased),
                           burst.position.y + directions[index].y * (16.0f + 26.0f * eased)});
        shard.setRotation(sf::degrees(std::atan2(directions[index].y, directions[index].x)));
        shard.setFillColor(sf::Color(255, 220, 140, static_cast<std::uint8_t>(150.0f * fade)));
        target.draw(shard);
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
        sf::Text label = UiHelpers::makeText(*font_, "结束回合", 22, sf::Color(24, 19, 14));
        const sf::FloatRect textBounds = label.getLocalBounds();
        label.setPosition({bounds.position.x + (bounds.size.x - textBounds.size.x) / 2.0f -
                               textBounds.position.x,
                           bounds.position.y + (bounds.size.y - textBounds.size.y) / 2.0f -
                               textBounds.position.y - 2.0f});
        window.draw(label);
    }
}
