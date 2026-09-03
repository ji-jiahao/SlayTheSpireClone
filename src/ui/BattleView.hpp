#pragma once

#include "card/Card.hpp"
#include "combat/CombatSystem.hpp"
#include "ui/BattleCast.hpp"
#include "ui/BattleHover.hpp"

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>

#include <optional>
#include <string>
#include <vector>

// 战斗界面：绘制玩家、敌人、手牌和“结束回合”按钮，
// 并把鼠标点击转换为手牌索引、目标选择和结束回合操作。
// 只通过 CombatSystem 的公开接口读写，不直接修改玩家或敌人血量。
class BattleView
{
public:
    BattleView();

    void setFont(const sf::Font& font);
    void setBackground(const sf::Texture& texture);
    void reset();
    void update(float deltaSeconds);
    void handleMouseMove(sf::Vector2f mousePosition, const CombatSystem& combat);
    void handleMouseClick(sf::Vector2f mousePosition, CombatSystem& combat);
    bool handleKeyPress(sf::Keyboard::Key key, CombatSystem& combat);
    void draw(sf::RenderWindow& window, const CombatSystem& combat) const;

    // 结束回合按钮所在区域，便于上层做额外判断或提示。
    sf::FloatRect getEndTurnButtonBounds() const;
    bool isVisualLocked() const;

private:
    enum class HandState
    {
        Idle,
        SelectingTarget,
        Playing
    };

    struct HoverCardVisual
    {
        int handIndex = -1;
        Card card;
        sf::FloatRect bounds;
        float progress = 0.0f;
        bool active = false;
    };

    struct SelectedCardVisual
    {
        int handIndex = -1;
        Card card;
        sf::FloatRect bounds;
        BattleTargetKind targetKind = BattleTargetKind::Enemy;
        float progress = 0.0f;
        bool active = false;
    };

    struct PlayAnim
    {
        Card card;
        sf::Vector2f startPos;
        sf::Vector2f targetPos;
        float progress = 0.0f;
        float duration = 0.4f;
        float arcHeight = 72.0f;
        float rotationStart = -12.0f;
        float rotationEnd = 0.0f;
        bool finished = false;
    };

    struct HitBurst
    {
        sf::Vector2f position;
        float progress = 0.0f;
        float duration = 0.24f;
    };

    void beginHoverVisual(const std::vector<Card>& hand, int handIndex,
                          const sf::FloatRect& bounds);
    void clearHoverVisual();
    void beginTargetSelection(const std::vector<Card>& hand, int handIndex,
                              const sf::FloatRect& bounds);
    void clearTargetSelection();
    void startPlayAnimation(const Card& card, sf::Vector2f startPos,
                            BattleTargetKind targetKind);
    void updateActiveVisuals(float deltaSeconds);
    void updateHoverCardTexture(sf::RenderTexture& texture, const Card& card) const;
    void updateHoverPanel(const Card& card, const sf::FloatRect& bounds);
    void playHoverSound();
    std::string cardTypeLabel(CardType type) const;
    void drawHoverVisual(sf::RenderTarget& target, const HoverCardVisual& visual,
                         const sf::RenderTexture& texture, bool isHovered) const;
    void drawHandCard(sf::RenderTarget& target, const Card& card, sf::Vector2f position,
                      float scale, float rotation) const;
    void drawHoverPanel(sf::RenderTarget& target) const;
    void drawTargetSelectionOverlay(sf::RenderWindow& window) const;
    void drawTargetHighlight(sf::RenderTarget& target) const;
    void drawPlayAnim(sf::RenderTarget& target, const PlayAnim& anim) const;
    void drawHitBurst(sf::RenderTarget& target, const HitBurst& burst) const;
    void loadCardSounds();
    void playCardSound(const Card& card);
    void drawPlayerPanel(sf::RenderWindow& window, const Player& player) const;
    void drawEnemyPanel(sf::RenderWindow& window, const Enemy& enemy) const;
    void drawHand(sf::RenderWindow& window, const std::vector<Card>& hand) const;
    void drawEndTurnButton(sf::RenderWindow& window) const;
    sf::FloatRect getSelectionTargetBounds(BattleTargetKind targetKind) const;
    sf::Vector2f getTargetFocusPoint(BattleTargetKind targetKind) const;
    void updateSelectionTargetHover(sf::Vector2f mousePosition);

    const sf::Font* font_;
    const sf::Texture* background_;
    sf::Music hoverCardSound_;
    sf::Music attackCardSound_;
    sf::Music defenseCardSound_;
    bool hoverCardSoundLoaded_;
    bool attackCardSoundLoaded_;
    bool defenseCardSoundLoaded_;
    bool hoverTextureReady_ = false;
    HandState handState_ = HandState::Idle;
    HoverCardVisual hoveredCard_;
    HoverCardVisual fadingCard_;
    SelectedCardVisual selectedCard_;
    bool selectedTargetHovered_ = false;
    sf::RenderTexture hoveredCardTexture_;
    sf::RenderTexture fadingCardTexture_;
    sf::RectangleShape hoverPanelBackground_;
    sf::RectangleShape hoverPanelOutline_;
    sf::CircleShape hoverCostCircle_;
    std::optional<sf::Text> hoverNameText_;
    std::optional<sf::Text> hoverTypeText_;
    std::optional<sf::Text> hoverCostText_;
    std::optional<sf::Text> hoverDescriptionText_;
    sf::Vector2f hoverPanelPosition_{0.0f, 0.0f};
    sf::Vector2f hoverPanelSize_{0.0f, 0.0f};
    bool hoverPanelVisible_ = false;
    std::vector<PlayAnim> activePlays_;
    std::vector<HitBurst> activeBursts_;
    float selectionTimer_ = 0.0f;
};
