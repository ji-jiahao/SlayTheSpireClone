#pragma once

#include "card/Card.hpp"
#include "combat/CombatSystem.hpp"
#include "ui/BattleCast.hpp"
#include "ui/BattleHover.hpp"
#include "ui/BattleHud.hpp"

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
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
    void update(float deltaSeconds, CombatSystem& combat);
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
        BattleTargetKind targetKind = BattleTargetKind::Enemy;
        BattleCast::ExitKind exit = BattleCast::ExitKind::Discard;
        std::shared_ptr<sf::RenderTexture> texture;
        int handIndex = -1;
        float elapsed = 0.0f;
        float startScale = 1.0f;
        bool committed = false;
    };

    struct HitBurst
    {
        sf::Vector2f position;
        float progress = 0.0f;
        float duration = 0.42f;
        BattleCast::EffectKind kind = BattleCast::EffectKind::Slash;
        int amount = 0;
    };

    void beginHoverVisual(const std::vector<Card>& hand, int handIndex,
                          const sf::FloatRect& bounds);
    void clearHoverVisual();
    void beginTargetSelection(const std::vector<Card>& hand, int handIndex,
                              const sf::FloatRect& bounds);
    void clearTargetSelection();
    void startPlayAnimation(const Card& card, sf::Vector2f startPos, float startScale,
                            int handIndex, const CombatSystem& combat);
    void updateActiveVisuals(float deltaSeconds, CombatSystem& combat);
    void updateDamageFlashes(float deltaSeconds, const CombatSystem& combat);
    float hitFlashAlpha(float timer) const;
    void loadHitFlashShader();
    void drawHitFlashSprite(sf::RenderTarget& target, const sf::Texture& texture,
                            sf::Vector2f center, float targetHeight, float alpha01) const;
    void drawEnemyFallback(sf::RenderTarget& target) const;
    bool hasPendingPlay() const;
    void updateHoverCardTexture(sf::RenderTexture& texture, const Card& card) const;
    void updateHoverPanel(const Card& card, const sf::FloatRect& bounds);
    void playHoverSound();
    void playEndTurnSound();
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
    void drawPlayerVisual(sf::RenderWindow& window) const;
    void drawEnemyPanel(sf::RenderWindow& window, const Enemy& enemy,
                        int displayedIntentDamage) const;
    void loadEnemyVisuals();
    void loadPlayerVisuals();
    void drawEnemyVisual(sf::RenderWindow& window, const Enemy& enemy) const;
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
    sf::Music endTurnSound_;
    bool hoverCardSoundLoaded_;
    bool attackCardSoundLoaded_;
    bool defenseCardSoundLoaded_;
    bool endTurnSoundLoaded_;
    bool hoverTextureReady_ = false;
    HandState handState_ = HandState::Idle;
    HoverCardVisual hoveredCard_;
    HoverCardVisual fadingCard_;
    SelectedCardVisual selectedCard_;
    bool selectedTargetHovered_ = false;
    sf::Vector2f mousePosition_;
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
    bool endTurnHovered_ = false;
    std::vector<PlayAnim> activePlays_;
    std::vector<HitBurst> activeBursts_;
    sf::Texture belialTexture_;
    std::vector<sf::Texture> playerFrames_;
    std::unordered_map<std::string, std::vector<sf::Texture>> enemyAnimations_;
    std::size_t playerAnimationFrame_ = 0;
    std::size_t enemyAnimationFrame_ = 0;
    float playerAnimationTimer_ = 0.0f;
    float enemyAnimationTimer_ = 0.0f;

    // 受击白色闪烁：shader 在绘制时以当前纹理生成白色剪影，计时器控制闪烁强度。
    mutable sf::Shader hitFlashShader_;
    bool hitFlashShaderLoaded_ = false;
    float playerFlashTimer_ = 0.0f;
    float enemyFlashTimer_ = 0.0f;
    int lastPlayerHealth_ = 0;
    int lastEnemyHealth_ = 0;
    bool healthSnapshotValid_ = false;
    BattleHud hud_;
};
