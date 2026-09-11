#include "ui/BattleView.hpp"

#include "ui/CardView.hpp"
#include "ui/CardPresentation.hpp"
#include "ui/UiHelpers.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <string>
#include <utility>

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
constexpr const char* kHoverCardSoundPath = "assets/sounds/card_select.mp3";
constexpr const char* kAttackCardSoundPath = "assets/sounds/card_attack.mp3";
constexpr const char* kDefenseCardSoundPath = "assets/sounds/card_defense.mp3";
constexpr const char* kEndTurnSoundPath = "assets/sounds/end_turn.mp3";
constexpr float kPlayerFrameSeconds = 0.10f;
constexpr float kEnemyFrameSeconds = 0.10f;
constexpr float kHitFlashDurationSeconds = 0.18f;

// 受击闪烁所用的片段着色器：把当前纹理采样成白色剪影（保留 alpha），
// flash_alpha 控制白色透明度，配合计时器实现“白色图像”的闪烁间隔。
constexpr const char* kHitFlashFragmentShader = R"(
uniform sampler2D texture;
uniform float flash_alpha;

void main()
{
    vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);
    gl_FragColor = vec4(1.0, 1.0, 1.0, pixel.a * flash_alpha);
}
)";

sf::Vector2f playerFocusPoint()
{
    return {223.0f, 313.0f};
}

sf::Vector2f enemyFocusPoint()
{
    return {1025.0f, 345.0f};
}

std::vector<BattleHover::HandCardLayout> handLayouts(const std::vector<Card>& hand)
{
    // Reserve the left edge for piles and the right edge for the end-turn button.
    auto layouts = BattleHover::layoutHandCards(hand, CardView::getCardSize(),
                                                kWindowWidth, kHandY, kHandGap);
    if (layouts.size() > 5)
    {
        const float step = 730.0f / static_cast<float>(layouts.size() - 1);
        for (std::size_t i = 0; i < layouts.size(); ++i)
            layouts[i].bounds.position.x = 190.0f + step * static_cast<float>(i);
    }
    return layouts;
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

std::string intentCategory(const EnemyIntent& intent)
{
    std::string category;
    if (intent.damage > 0)
    {
        category = "攻击";
    }
    if (intent.weak > 0 || intent.vulnerable > 0 || intent.frail > 0 ||
        intent.slimed > 0)
    {
        if (!category.empty())
        {
            category += "/";
        }
        category += "施加负面状态";
    }
    if (intent.block > 0 || intent.strength > 0 || intent.dexterity > 0 ||
        intent.type == EnemyIntentType::Sleep ||
        intent.type == EnemyIntentType::Preparing || category.empty())
    {
        if (!category.empty())
        {
            category += "/";
        }
        category += "防御";
    }
    return category;
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
      defenseCardSoundLoaded_(false),
      endTurnSoundLoaded_(false)
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
    loadPlayerVisuals();
    loadEnemyVisuals();
    loadHitFlashShader();
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
    hud_.reset();
    handState_ = HandState::Idle;
    hoveredCard_ = {};
    fadingCard_ = {};
    selectedCard_ = {};
    selectedTargetHovered_ = false;
    activePlays_.clear();
    activeBursts_.clear();
    hoverPanelVisible_ = false;
    endTurnHovered_ = false;
    playerAnimationTimer_ = 0.0f;
    playerAnimationFrame_ = 0;
    playerFlashTimer_ = 0.0f;
    enemyFlashTimer_ = 0.0f;
    lastPlayerHealth_ = 0;
    lastEnemyHealth_ = 0;
    healthSnapshotValid_ = false;
}

void BattleView::update(float deltaSeconds, CombatSystem& combat)
{
    if (!std::isfinite(deltaSeconds)) return;
    deltaSeconds = std::max(0.0f, deltaSeconds);
    updateActiveVisuals(deltaSeconds, combat);
    // Refresh cached hover artwork even when the mouse stays still as costs change.
    auto refresh = [&](HoverCardVisual& visual, sf::RenderTexture& texture, bool tooltip) {
        if (!visual.active || visual.handIndex < 0 ||
            static_cast<std::size_t>(visual.handIndex) >= combat.getHandCards().size()) return;
        const auto& original = combat.getHandCards()[visual.handIndex];
        if (original.id != visual.card.id) return;
        const auto display = CardPresentation::forCombat(original, combat);
        if (display.cost != visual.card.cost || display.name != visual.card.name ||
            display.description != visual.card.description)
        {
            visual.card = display;
            updateHoverCardTexture(texture, display);
            if (tooltip) updateHoverPanel(display, visual.bounds);
        }
    };
    refresh(hoveredCard_, hoveredCardTexture_, true);
    refresh(fadingCard_, fadingCardTexture_, false);
    if (selectedCard_.active && selectedCard_.handIndex >= 0 &&
        static_cast<std::size_t>(selectedCard_.handIndex) < combat.getHandCards().size())
        selectedCard_.card = CardPresentation::forCombat(combat.getHandCards()[selectedCard_.handIndex], combat);
    playerAnimationTimer_ += std::max(0.0f, deltaSeconds);
    while (playerAnimationTimer_ >= kPlayerFrameSeconds && !playerFrames_.empty())
    {
        playerAnimationTimer_ -= kPlayerFrameSeconds;
        ++playerAnimationFrame_;
    }
    enemyAnimationTimer_ += std::max(0.0f, deltaSeconds);
    while (enemyAnimationTimer_ >= kEnemyFrameSeconds)
    {
        enemyAnimationTimer_ -= kEnemyFrameSeconds;
        ++enemyAnimationFrame_;
    }
    updateDamageFlashes(deltaSeconds, combat);
}

void BattleView::handleMouseMove(sf::Vector2f mousePosition, const CombatSystem& combat)
{
    mousePosition_ = mousePosition;
    if (hasPendingPlay()) return;
    if (hud_.handleMouseMove(mousePosition) || combat.hasPendingDiscardChoice())
    {
        clearHoverVisual();
        endTurnHovered_ = false;
        return;
    }
    if (combat.getResult() != BattleResult::Active)
    {
        clearHoverVisual();
        clearTargetSelection();
        endTurnHovered_ = false;
        return;
    }

    if (handState_ == HandState::SelectingTarget)
    {
        updateSelectionTargetHover(mousePosition);
        endTurnHovered_ = false;
        return;
    }

    endTurnHovered_ = getEndTurnButtonBounds().contains(mousePosition);

    const auto hand = CardPresentation::handForCombat(combat);
    const auto layouts = handLayouts(hand);
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
    if (hasPendingPlay()) return;
    const float hoveredProgress = hoveredCard_.active ? hoveredCard_.progress : 0.0f;
    const int hoveredIndex = hoveredCard_.handIndex;
    clearHoverVisual();

    if (hud_.handleMouseClick(mousePosition, combat))
    {
        clearTargetSelection();
        endTurnHovered_ = false;
        return;
    }

    if (combat.getResult() != BattleResult::Active)
    {
        return;
    }

    if (handState_ == HandState::SelectingTarget)
    {
        if (getSelectionTargetBounds(selectedCard_.targetKind).contains(mousePosition))
        {
            const Card playedCard = CardPresentation::forCombat(
                combat.getHandCards()[selectedCard_.handIndex], combat);
            const float eased = BattleHover::easeOutCubic(selectedCard_.progress);
            startPlayAnimation(playedCard,
                               selectedCard_.bounds.position + sf::Vector2f{0.0f, kSelectionLift * eased},
                               1.0f + (kSelectionScale - 1.0f) * eased,
                               selectedCard_.handIndex, combat);
            clearTargetSelection();
            return;
        }

        return;
    }

    const std::vector<Card>& hand = combat.getHandCards();
    const auto layouts = handLayouts(hand);
    const int clickedIndex = BattleHover::pickHoveredCardIndex(mousePosition, layouts);
    if (clickedIndex >= 0)
    {
        const auto& layout = layouts[static_cast<std::size_t>(clickedIndex)];
        const Card card = hand[static_cast<std::size_t>(layout.handIndex)];
        const Card displayCard = CardPresentation::forCombat(card, combat);
        const int cardCost = combat.getPlayableCardCost(card);
        if (cardCost < 0)
        {
            return;
        }

        if (combat.getPlayer().getCurrentEnergy() < cardCost)
        {
            return;
        }

        if (BattleCast::requiresTargetSelection(card))
        {
            beginTargetSelection(CardPresentation::handForCombat(combat), layout.handIndex, layout.bounds);
            updateSelectionTargetHover(mousePosition);
        }
        else
        {
            const float eased = hoveredIndex == clickedIndex
                                    ? BattleHover::easeOutCubic(hoveredProgress) : 0.0f;
            startPlayAnimation(displayCard, layout.bounds.position + sf::Vector2f{0.0f, kHoverLift * eased},
                               1.0f + (kHoverScale - 1.0f) * eased, layout.handIndex, combat);
        }
        return;
    }

    if (getEndTurnButtonBounds().contains(mousePosition) && activePlays_.empty())
    {
        combat.endPlayerTurn();
        playEndTurnSound();
    }
}

bool BattleView::handleKeyPress(sf::Keyboard::Key key, CombatSystem& combat)
{
    if (hasPendingPlay()) return true;
    if (hud_.handleKeyPress(key, combat)) return true;
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
            updateHoverCardTexture(fadingCardTexture_, fadingCard_.card);
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
        updateHoverCardTexture(fadingCardTexture_, fadingCard_.card);
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
        updateHoverCardTexture(fadingCardTexture_, fadingCard_.card);
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
    fadingCard_ = {};

    selectedCard_.handIndex = handIndex;
    selectedCard_.card = hand[static_cast<std::size_t>(handIndex)];
    selectedCard_.bounds = bounds;
    selectedCard_.targetKind = BattleCast::resolveTargetKind(selectedCard_.card);
    selectedCard_.progress = 0.0f;
    selectedCard_.active = true;
    handState_ = HandState::SelectingTarget;
    selectedTargetHovered_ = false;
}

void BattleView::clearTargetSelection()
{
    selectedCard_.active = false;
    selectedCard_.handIndex = -1;
    selectedCard_.progress = 0.0f;
    selectedTargetHovered_ = false;
    handState_ = activePlays_.empty() ? HandState::Idle : HandState::Playing;
}

bool BattleView::hasPendingPlay() const
{
    return std::any_of(activePlays_.begin(), activePlays_.end(),
                       [](const PlayAnim& anim) { return !anim.committed; });
}

void BattleView::startPlayAnimation(const Card& card, sf::Vector2f startPos, float startScale,
                                    int handIndex, const CombatSystem& combat)
{
    // 施放纹理持有独立快照，手牌变化后不留下旧悬停图，也不引用失效的卡牌。
    hoveredCard_ = {};
    fadingCard_ = {};
    if (!healthSnapshotValid_)
    {
        lastPlayerHealth_ = combat.getPlayer().getCurrentHealth();
        lastEnemyHealth_ = combat.getEnemy().getCurrentHealth();
        healthSnapshotValid_ = true;
    }
    PlayAnim anim;
    anim.card = card;
    anim.startPos = {startPos.x, startPos.y};
    anim.startScale = startScale;
    anim.handIndex = handIndex;
    anim.targetKind = BattleCast::resolveTargetKind(card);
    anim.targetPos = getTargetFocusPoint(anim.targetKind);
    anim.exit = card.type == CardType::Power ? BattleCast::ExitKind::Power :
                combat.willExhaustCard(card) ? BattleCast::ExitKind::Exhaust : BattleCast::ExitKind::Discard;
    anim.texture = std::make_shared<sf::RenderTexture>(sf::Vector2u{160, 220});
    updateHoverCardTexture(*anim.texture, card);
    activePlays_.push_back(std::move(anim));
    handState_ = HandState::Playing;
}

void BattleView::updateActiveVisuals(float deltaSeconds, CombatSystem& combat)
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
    }

    // 先推进既有反馈；本帧新命中的反馈保留首帧，卡顿也不会直接吞掉命中特效。
    for (HitBurst& burst : activeBursts_)
    {
        burst.progress = std::min(1.0f, burst.progress + deltaSeconds / burst.duration);
    }
    for (PlayAnim& anim : activePlays_)
    {
        anim.elapsed += deltaSeconds;
        if (!anim.committed && anim.elapsed >= BattleCast::kCommitSeconds)
        {
            anim.committed = true;
            const int health = combat.getEnemy().getCurrentHealth();
            const int block = combat.getPlayer().getBlock();
            const int enemyBlock = combat.getEnemy().getBlock();
            const auto& hand = combat.getHandCards();
            if (anim.handIndex < 0 || static_cast<std::size_t>(anim.handIndex) >= hand.size() ||
                hand[anim.handIndex].id != anim.card.id || !combat.playCard(anim.handIndex))
            {
                anim.elapsed = BattleCast::kFinishSeconds;
                continue;
            }
            // 防止大时间步在提交结算的同一帧也移除牌面。
            anim.elapsed = BattleCast::kCommitSeconds;
            const auto kind = BattleCast::resolveEffectKind(anim.card);
            const int damage = std::max(0, health - combat.getEnemy().getCurrentHealth());
            const int blockGain = std::max(0, combat.getPlayer().getBlock() - block);
            const int amount = kind == BattleCast::EffectKind::Slash
                                   ? (damage > 0 ? damage : enemyBlock > combat.getEnemy().getBlock() ? 0 : -1)
                                   : kind == BattleCast::EffectKind::Guard ? blockGain : -1;
            activeBursts_.push_back({anim.targetPos, 0.0f, 0.42f, kind, amount});
            if (damage > 0 && kind != BattleCast::EffectKind::Slash)
                activeBursts_.push_back({enemyFocusPoint(), 0.0f, 0.42f, BattleCast::EffectKind::Slash, damage});
            if (blockGain > 0 && kind != BattleCast::EffectKind::Guard)
                activeBursts_.push_back({playerFocusPoint(), 0.0f, 0.42f, BattleCast::EffectKind::Guard, blockGain});
            playCardSound(anim.card);
        }
    }

    activePlays_.erase(
        std::remove_if(activePlays_.begin(), activePlays_.end(),
                       [](const PlayAnim& anim) { return anim.elapsed >= BattleCast::kFinishSeconds; }),
        activePlays_.end());

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

void BattleView::updateDamageFlashes(float deltaSeconds, const CombatSystem& combat)
{
    playerFlashTimer_ = std::max(0.0f, playerFlashTimer_ - deltaSeconds);
    enemyFlashTimer_ = std::max(0.0f, enemyFlashTimer_ - deltaSeconds);

    const int playerHealth = combat.getPlayer().getCurrentHealth();
    const int enemyHealth = combat.getEnemy().getCurrentHealth();

    if (healthSnapshotValid_)
    {
        // 结算已在动画命中点发生，血条、音效与受击白闪使用同一帧状态。
        if (playerHealth < lastPlayerHealth_)
        {
            playerFlashTimer_ = kHitFlashDurationSeconds;
            activeBursts_.push_back({playerFocusPoint(), 0.0f, 0.42f,
                                    BattleCast::EffectKind::Slash, lastPlayerHealth_ - playerHealth});
        }
        if (enemyHealth < lastEnemyHealth_)
        {
            enemyFlashTimer_ = kHitFlashDurationSeconds;
        }
    }

    lastPlayerHealth_ = playerHealth;
    lastEnemyHealth_ = enemyHealth;
    healthSnapshotValid_ = true;
}

float BattleView::hitFlashAlpha(float timer) const
{
    if (timer <= 0.0f)
    {
        return 0.0f;
    }

    const float remaining = std::clamp(timer / kHitFlashDurationSeconds, 0.0f, 1.0f);
    return remaining * remaining;
}

void BattleView::loadHitFlashShader()
{
    if (!sf::Shader::isAvailable())
    {
        return;
    }

    if (!hitFlashShader_.loadFromMemory(kHitFlashFragmentShader, sf::Shader::Type::Fragment))
    {
        return;
    }

    hitFlashShader_.setUniform("texture", sf::Shader::CurrentTexture);
    hitFlashShader_.setUniform("flash_alpha", 1.0f);
    hitFlashShaderLoaded_ = true;
}

void BattleView::drawHitFlashSprite(sf::RenderTarget& target, const sf::Texture& texture,
                                    sf::Vector2f center, float targetHeight,
                                    float alpha01) const
{
    if (!hitFlashShaderLoaded_ || alpha01 <= 0.0f)
    {
        return;
    }

    const sf::Vector2u size = texture.getSize();
    sf::Sprite sprite(texture);
    sprite.setOrigin({static_cast<float>(size.x) / 2.0f,
                      static_cast<float>(size.y) / 2.0f});
    sprite.setPosition(center);
    const float scale = targetHeight / static_cast<float>(size.y);
    sprite.setScale({scale, scale});

    hitFlashShader_.setUniform("flash_alpha", alpha01);
    sf::RenderStates states;
    states.shader = &hitFlashShader_;
    target.draw(sprite, states);
}

void BattleView::drawEnemyFallback(sf::RenderTarget& target) const
{
    sf::CircleShape enemyBody(75.0f, 7);
    enemyBody.setPosition({950.0f, 235.0f});
    enemyBody.setFillColor(sf::Color(58, 50, 70, 235));
    target.draw(enemyBody);

    const float flashAlpha = hitFlashAlpha(enemyFlashTimer_);
    if (flashAlpha > 0.0f)
    {
        sf::CircleShape flashBody(75.0f, 7);
        flashBody.setPosition({950.0f, 235.0f});
        flashBody.setFillColor(sf::Color(
            255, 255, 255, static_cast<std::uint8_t>(255.0f * flashAlpha)));
        target.draw(flashBody);
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

    hoverTypeText_->setString(UiHelpers::toSfString("类型 " + CardPresentation::typeLabel(card)));
    hoverTypeText_->setPosition({hoverPanelPosition_.x + kSidePadding,
                                 hoverPanelPosition_.y + 40.0f});

    const std::string costLabel = CardPresentation::costLabel(card.cost);
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

void BattleView::playEndTurnSound()
{
    if (!endTurnSoundLoaded_)
    {
        return;
    }

    endTurnSound_.stop();
    endTurnSound_.play();
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

    if (!isHovered)
    {
        const sf::Vector2f topLeft{
            cardCenter.x - CardView::getCardSize().x * scale / 2.0f,
            cardCenter.y - CardView::getCardSize().y * scale / 2.0f};
        drawHandCard(target, visual.card, topLeft, scale, 0.0f);
        return;
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
    const sf::Vector2f start = selectedCard_.bounds.position + sf::Vector2f{80.0f, -20.0f};
    const sf::Vector2f end = selectedTargetHovered_ ? getTargetFocusPoint(selectedCard_.targetKind)
                                                   : mousePosition_;
    const sf::Vector2f control{start.x, std::min(start.y, end.y) - 130.0f};
    const sf::Color color = selectedTargetHovered_ ? sf::Color(255, 106, 82, 230)
                                                  : sf::Color(241, 214, 149, 210);
    for (int i = 1; i < 24; ++i)
    {
        const float t = static_cast<float>(i) / 24.0f;
        const sf::Vector2f point = (1.0f - t) * (1.0f - t) * start +
                                    2.0f * (1.0f - t) * t * control + t * t * end;
        sf::CircleShape dot(2.0f + 2.0f * t, 12);
        dot.setOrigin({dot.getRadius(), dot.getRadius()});
        dot.setPosition(point);
        dot.setFillColor(color);
        window.draw(dot);
    }
    const sf::Vector2f tangent = end - control;
    sf::CircleShape arrow(12.0f, 3);
    arrow.setOrigin({12.0f, 12.0f});
    arrow.setPosition(end);
    arrow.setRotation(sf::degrees(std::atan2(tangent.y, tangent.x) * 180.0f / 3.14159265f + 90.0f));
    arrow.setFillColor(color);
    window.draw(arrow);
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

    endTurnSoundLoaded_ = endTurnSound_.openFromFile(kEndTurnSoundPath);
    if (endTurnSoundLoaded_)
    {
        endTurnSound_.setVolume(100.0f);
        endTurnSound_.setLooping(false);
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

    // 仅角色后坐，HUD 与手牌保持固定，避免打断下一次点击。
    const sf::View originalView = window.getView();
    auto recoilView = [&](float timer, float direction)
    {
        sf::View view = originalView;
        const float t = 1.0f - std::clamp(timer / kHitFlashDurationSeconds, 0.0f, 1.0f);
        view.move({-direction * std::sin(t * 3.14159265f) * (1.0f - t) * 22.0f, 0.0f});
        window.setView(view);
    };
    recoilView(playerFlashTimer_, -1.0f);
    for (auto it = activePlays_.rbegin(); it != activePlays_.rend(); ++it)
    {
        if (it->card.type != CardType::Attack || it->elapsed >= 0.26f) continue;
        const float t = it->elapsed / 0.26f;
        sf::View attackView = window.getView();
        attackView.move({-18.0f * std::sin(t * 3.14159265f), 0.0f});
        window.setView(attackView);
        break;
    }
    drawPlayerVisual(window);
    recoilView(enemyFlashTimer_, 1.0f);
    drawEnemyVisual(window, combat.getEnemy());
    window.setView(originalView);

    drawPlayerPanel(window, combat.getPlayer());
    drawEnemyPanel(window, combat.getEnemy(), combat.getEnemyIntentDamage());
    drawHand(window, CardPresentation::handForCombat(combat));
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
        hud_.draw(window, *font_, combat);
    }
}

void BattleView::loadEnemyVisuals()
{
    const bool loaded = belialTexture_.loadFromFile("assets/images/enemies/belial.png");
    (void)loaded;

    static const std::array<std::pair<const char*, const char*>, 6> animationPaths = {{
        {"cultist", "assets/images/enemies/cultist"},
        {"acid_slime", "assets/images/enemies/acid_slime"},
        {"fungi_beast", "assets/images/enemies/fungi_beast"},
        {"jaw_worm", "assets/images/enemies/jaw_worm"},
        {"slime_boss", "assets/images/enemies/dark_beast"},
        {"lagavulin", "assets/images/enemies/lagavulin"},
    }};

    for (const auto& [enemyId, directory] : animationPaths)
    {
        std::vector<sf::Texture> frames;
        for (int frameIndex = 0; frameIndex < 10; ++frameIndex)
        {
            sf::Texture texture;
            const bool frameLoaded = texture.loadFromFile(
                std::string(directory) + "/frame_" +
                (frameIndex < 10 ? "00" : frameIndex < 100 ? "0" : "") +
                std::to_string(frameIndex) + ".png");
            if (!frameLoaded)
            {
                break;
            }
            frames.push_back(std::move(texture));
        }
        if (!frames.empty())
        {
            enemyAnimations_.emplace(enemyId, std::move(frames));
        }
    }
}

void BattleView::loadPlayerVisuals()
{
    playerFrames_.clear();
    playerFrames_.reserve(16);
    for (int frameIndex = 0; frameIndex < 16; ++frameIndex)
    {
        sf::Texture texture;
        const bool loaded = texture.loadFromFile(
            std::string("assets/images/player/tafi_frames/frame_") +
            (frameIndex < 10 ? "00" : "0") + std::to_string(frameIndex) + ".png");
        if (!loaded)
        {
            playerFrames_.clear();
            return;
        }
        playerFrames_.push_back(std::move(texture));
    }
}

void BattleView::drawPlayerVisual(sf::RenderWindow& window) const
{
    if (playerFrames_.empty())
    {
        sf::CircleShape playerBody(68.0f, 24);
        playerBody.setPosition({155.0f, 245.0f});
        playerBody.setFillColor(sf::Color(128, 48, 42, 225));
        window.draw(playerBody);

        const float flashAlpha = hitFlashAlpha(playerFlashTimer_);
        if (flashAlpha > 0.0f)
        {
            sf::CircleShape flashBody(68.0f, 24);
            flashBody.setPosition({155.0f, 245.0f});
            flashBody.setFillColor(sf::Color(
                255, 255, 255, static_cast<std::uint8_t>(255.0f * flashAlpha)));
            window.draw(flashBody);
        }
        return;
    }

    const sf::Texture& texture =
        playerFrames_[playerAnimationFrame_ % playerFrames_.size()];
    const sf::Vector2u size = texture.getSize();
    sf::Sprite sprite(texture);
    sprite.setOrigin({static_cast<float>(size.x) / 2.0f,
                      static_cast<float>(size.y) / 2.0f});
    sprite.setPosition(playerFocusPoint());
    const float targetHeight = 250.0f;
    const float scale = targetHeight / static_cast<float>(size.y);
    sprite.setScale({scale, scale});
    window.draw(sprite);

    const float flashAlpha = hitFlashAlpha(playerFlashTimer_);
    if (flashAlpha > 0.0f)
    {
        drawHitFlashSprite(window, texture, playerFocusPoint(), targetHeight, flashAlpha);
    }
}

void BattleView::drawEnemyVisual(sf::RenderWindow& window, const Enemy& enemy) const
{
    const sf::Texture* texture = nullptr;
    if (enemy.getId() == "belial")
    {
        if (belialTexture_.getSize().x == 0)
        {
            drawEnemyFallback(window);
            return;
        }
        texture = &belialTexture_;
    }
    else
    {
        auto animationIt = enemyAnimations_.find(enemy.getId());
        if (animationIt == enemyAnimations_.end() || animationIt->second.empty())
        {
            drawEnemyFallback(window);
            return;
        }
        const std::vector<sf::Texture>& frames = animationIt->second;
        texture = &frames[enemyAnimationFrame_ % frames.size()];
    }

    const sf::Vector2u size = texture->getSize();
    sf::Sprite sprite(*texture);
    sprite.setOrigin({static_cast<float>(size.x) / 2.0f,
                      static_cast<float>(size.y) / 2.0f});
    sprite.setPosition(enemyFocusPoint());
    const float targetHeight = enemy.getId() == "belial" ? 240.0f : 210.0f;
    const float scale = targetHeight / static_cast<float>(size.y);
    sprite.setScale({scale, scale});
    window.draw(sprite);

    const float flashAlpha = hitFlashAlpha(enemyFlashTimer_);
    if (flashAlpha > 0.0f)
    {
        drawHitFlashSprite(window, *texture, enemyFocusPoint(), targetHeight, flashAlpha);
    }
}

void BattleView::drawPlayerPanel(sf::RenderWindow& window, const Player& player) const
{
    const float maxHealth = static_cast<float>(player.getMaxHealth());
    const float currentHealth = static_cast<float>(player.getCurrentHealth());

    if (font_ != nullptr)
    {
        sf::Text title = UiHelpers::makeText(*font_, "玩家", 24, sf::Color(235, 229, 207));
        title.setPosition({58.0f, 52.0f});
        window.draw(title);

        sf::Text energyText =
            UiHelpers::makeText(*font_, "能量 " + std::to_string(player.getCurrentEnergy()) +
                                             "/" + std::to_string(player.getMaxEnergy()),
                                18, sf::Color(240, 200, 120));
        energyText.setPosition({270.0f, 52.0f});
        window.draw(energyText);

        BattleHud::drawStatuses(window, *font_, {58,150}, player.getStrength(),
                                player.getWeak(), player.getVulnerable(),
                                player.getDexterity());
    }

    const float hpRatio = maxHealth > 0.0f ? currentHealth / maxHealth : 0.0f;
    const float blockRatio = maxHealth > 0.0f
                                 ? static_cast<float>(player.getBlock()) / maxHealth
                                 : 0.0f;
    drawBar(window, {58.0f, 90.0f}, {300.0f, 22.0f}, hpRatio,
            sf::Color(196, 70, 60), sf::Color(60, 40, 40));
    drawBar(window, {58.0f, 118.0f}, {300.0f, 22.0f}, blockRatio,
            sf::Color(186, 190, 198), sf::Color(66, 68, 74));

    if (font_ != nullptr)
    {
        sf::Text hpLabel = UiHelpers::makeText(
            *font_, "生命 " + std::to_string(player.getCurrentHealth()) + "/" +
                        std::to_string(player.getMaxHealth()), 15,
            sf::Color(255, 245, 238));
        hpLabel.setPosition({66.0f, 91.0f});
        window.draw(hpLabel);

        sf::Text blockLabel = UiHelpers::makeText(
            *font_, "护盾 " + std::to_string(player.getBlock()), 15,
            sf::Color(28, 30, 34));
        blockLabel.setPosition({66.0f, 119.0f});
        window.draw(blockLabel);
    }
}

void BattleView::drawEnemyPanel(sf::RenderWindow& window, const Enemy& enemy,
                                int displayedIntentDamage) const
{
    const float maxHealth = static_cast<float>(enemy.getMaxHealth());
    const float currentHealth = static_cast<float>(enemy.getCurrentHealth());

    if (font_ != nullptr)
    {
        sf::Text title =
            UiHelpers::makeText(*font_, enemy.getName(), 24, sf::Color(235, 229, 207));
        title.setPosition({kWindowWidth - 402.0f, 52.0f});
        window.draw(title);

        BattleHud::drawStatuses(window, *font_, {878,150}, enemy.getStrength(),
                                enemy.getWeak(), enemy.getVulnerable(), 0);
        BattleHud::drawIntent(window, *font_, enemy, displayedIntentDamage);
    }

    const float hpRatio = maxHealth > 0.0f ? currentHealth / maxHealth : 0.0f;
    const float blockRatio = maxHealth > 0.0f
                                 ? static_cast<float>(enemy.getBlock()) / maxHealth
                                 : 0.0f;
    drawBar(window, {kWindowWidth - 402.0f, 90.0f}, {300.0f, 22.0f}, hpRatio,
            sf::Color(196, 70, 60), sf::Color(60, 40, 40));
    drawBar(window, {kWindowWidth - 402.0f, 118.0f}, {300.0f, 22.0f}, blockRatio,
            sf::Color(186, 190, 198), sf::Color(66, 68, 74));

    if (font_ != nullptr)
    {
        sf::Text hpLabel = UiHelpers::makeText(
            *font_, "生命 " + std::to_string(enemy.getCurrentHealth()) + "/" +
                        std::to_string(enemy.getMaxHealth()), 15,
            sf::Color(255, 245, 238));
        hpLabel.setPosition({kWindowWidth - 394.0f, 91.0f});
        window.draw(hpLabel);

        sf::Text blockLabel = UiHelpers::makeText(
            *font_, "护盾 " + std::to_string(enemy.getBlock()), 15,
            sf::Color(28, 30, 34));
        blockLabel.setPosition({kWindowWidth - 394.0f, 119.0f});
        window.draw(blockLabel);
    }
}

void BattleView::drawHand(sf::RenderWindow& window, const std::vector<Card>& hand) const
{
    const auto layouts = handLayouts(hand);

    for (const BattleHover::HandCardLayout& layout : layouts)
    {
        if (std::any_of(activePlays_.begin(), activePlays_.end(), [&](const PlayAnim& anim)
            { return !anim.committed && anim.handIndex == layout.handIndex; })) continue;
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
    const auto pose = BattleCast::samplePose(anim.elapsed, anim.startPos + sf::Vector2f{80.0f, 110.0f},
                                             anim.startScale, anim.exit);
    sf::Sprite sprite(anim.texture->getTexture());
    sprite.setOrigin({80.0f, 110.0f});
    sprite.setPosition(pose.center);
    sprite.setScale({pose.scale, pose.scale});
    sprite.setRotation(sf::degrees(pose.rotation));
    sprite.setColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(255.0f * pose.opacity)));
    target.draw(sprite);

    if (anim.exit != BattleCast::ExitKind::Discard && anim.elapsed > BattleCast::kReleaseSeconds)
    {
        const float t = (anim.elapsed - BattleCast::kReleaseSeconds) /
                        (BattleCast::kFinishSeconds - BattleCast::kReleaseSeconds);
        for (int i = 0; i < 18; ++i)
        {
            const float x = static_cast<float>((i * 47) % 137) - 68.0f;
            const float y = static_cast<float>((i * 71) % 179) - 89.0f;
            sf::RectangleShape ember({3.0f + 3.0f * (1.0f - t), 6.0f});
            ember.setPosition(pose.center + sf::Vector2f{x * (0.8f + t), y - t * (25.0f + i * 3.0f)});
            ember.setFillColor(anim.exit == BattleCast::ExitKind::Power
                                  ? sf::Color(255, 222, 122, static_cast<std::uint8_t>(220.0f * pose.opacity))
                                  : sf::Color(247, 118, 60, static_cast<std::uint8_t>(220.0f * pose.opacity)));
            target.draw(ember);
        }
    }
}

void BattleView::drawHitBurst(sf::RenderTarget& target, const HitBurst& burst) const
{
    const float eased = BattleHover::easeOutCubic(burst.progress);
    const float fade = 1.0f - burst.progress;
    const auto alpha = static_cast<std::uint8_t>(235.0f * fade * fade);
    const bool slash = burst.kind == BattleCast::EffectKind::Slash;
    const bool guard = burst.kind == BattleCast::EffectKind::Guard;
    sf::Color color = slash ? sf::Color(255, 203, 148, alpha) :
                      guard ? sf::Color(126, 213, 255, alpha) :
                      burst.kind == BattleCast::EffectKind::Debuff ? sf::Color(193, 133, 235, alpha) :
                      sf::Color(255, 223, 126, alpha);
    if (slash)
    {
        for (int i = 0; i < 3; ++i)
        {
            sf::ConvexShape cut(4);
            const float length = 65.0f + 75.0f * eased;
            const float width = (12.0f - i * 3.0f) * fade;
            cut.setPoint(0, {-length, 0.0f});
            cut.setPoint(1, {10.0f, -width});
            cut.setPoint(2, {length, 0.0f});
            cut.setPoint(3, {-10.0f, width});
            cut.setPosition(burst.position + sf::Vector2f{static_cast<float>(i * 13 - 13), 0.0f});
            cut.setRotation(sf::degrees(-42.0f + i * 8.0f));
            cut.setFillColor(i == 0 ? sf::Color(255, 250, 228, alpha) : color);
            target.draw(cut);
        }
    }
    else if (guard)
    {
        sf::ConvexShape shield(6);
        shield.setPoint(0, {-48.0f, -57.0f});
        shield.setPoint(1, {0.0f, -70.0f});
        shield.setPoint(2, {48.0f, -57.0f});
        shield.setPoint(3, {42.0f, 13.0f});
        shield.setPoint(4, {0.0f, 58.0f});
        shield.setPoint(5, {-42.0f, 13.0f});
        shield.setPosition(burst.position);
        shield.setScale({0.8f + 0.3f * eased, 0.8f + 0.3f * eased});
        shield.setFillColor(sf::Color(70, 150, 220, static_cast<std::uint8_t>(55.0f * fade)));
        shield.setOutlineColor(color);
        shield.setOutlineThickness(4.0f * fade);
        target.draw(shield);
    }
    else
    {
        sf::CircleShape ring(38.0f + 48.0f * eased, 48);
        ring.setOrigin({ring.getRadius(), ring.getRadius()});
        ring.setPosition(burst.position);
        ring.setScale({1.0f, burst.kind == BattleCast::EffectKind::Power ? 0.4f : 1.0f});
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineColor(color);
        ring.setOutlineThickness(4.0f * fade);
        target.draw(ring);
    }
    for (int i = 0; i < 9; ++i)
    {
        const float angle = static_cast<float>(i) * 2.39996f;
        const float distance = 22.0f + (28.0f + (i % 3) * 14.0f) * eased;
        sf::RectangleShape spark({slash ? 15.0f * fade : 4.0f, 3.0f});
        spark.setPosition(burst.position + sf::Vector2f{std::cos(angle) * distance,
                                                       std::sin(angle) * distance - (slash ? 0.0f : eased * 35.0f)});
        spark.setRotation(sf::degrees(angle * 180.0f / 3.14159265f));
        spark.setFillColor(color);
        target.draw(spark);
    }
    if (font_ != nullptr && burst.amount >= 0 && (burst.amount > 0 || guard || slash))
    {
        const std::string label = burst.amount > 0 ? (guard ? "+" : "") + std::to_string(burst.amount)
                                                   : slash ? "格挡" : "";
        auto number = UiHelpers::makeText(*font_, label, 34, sf::Color(color.r, color.g, color.b,
                                       static_cast<std::uint8_t>(255.0f * fade)));
        number.setStyle(sf::Text::Bold);
        number.setOutlineThickness(2.0f);
        number.setOutlineColor(sf::Color(26, 20, 24, static_cast<std::uint8_t>(255.0f * fade)));
        const auto bounds = number.getLocalBounds();
        number.setOrigin({bounds.position.x + bounds.size.x / 2.0f, bounds.position.y + bounds.size.y / 2.0f});
        number.setPosition(burst.position + sf::Vector2f{0.0f, -70.0f - 48.0f * eased});
        target.draw(number);
    }
}

void BattleView::drawEndTurnButton(sf::RenderWindow& window) const
{
    const sf::FloatRect bounds = getEndTurnButtonBounds();

    sf::RectangleShape button({bounds.size.x, bounds.size.y});
    button.setPosition(bounds.position);
    button.setFillColor(endTurnHovered_ ? sf::Color(245, 193, 84)
                                        : sf::Color(230, 174, 72));
    button.setOutlineColor(sf::Color(36, 28, 18));
    button.setOutlineThickness(endTurnHovered_ ? 4.0f : 3.0f);
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
