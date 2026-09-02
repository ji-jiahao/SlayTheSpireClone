#pragma once

#include "core/GameState.hpp"
#include "room/ShopSystem.hpp"

#include <SFML/Graphics.hpp>

#include <cstddef>
#include <random>
#include <string>
#include <vector>

enum class ShopActionType
{
    None,
    BuyCard,
    BuyRelic,
    OpenRemove,
    RemoveCard,
    CancelRemove,
    Leave
};

struct ShopAction
{
    ShopActionType type = ShopActionType::None;
    std::size_t index = 0;
};

class ShopView
{
public:
    void setFont(const sf::Font& font);
    bool loadMerchantAnimation(const std::string& framesDirectory);
    void resetDialogue(unsigned int seed);
    void update(float deltaSeconds);
    void handleMouseMove(sf::Vector2f position, const ShopSystem& shop,
                         const GameState& state, bool removingCard);
    ShopAction handleMouseClick(sf::Vector2f position, const ShopSystem& shop,
                                const GameState& state, bool removingCard) const;
    void draw(sf::RenderWindow& window, const ShopSystem& shop,
              const GameState& state, bool removingCard,
              const std::string& message) const;

private:
    sf::FloatRect cardBounds(std::size_t index) const;
    sf::FloatRect relicBounds(std::size_t index) const;
    sf::FloatRect removeButtonBounds() const;
    sf::FloatRect leaveButtonBounds() const;
    sf::FloatRect cancelRemoveButtonBounds() const;
    sf::FloatRect deckCardBounds(std::size_t index) const;
    void chooseNextDialogue();
    float dialogueAlpha() const;
    void drawMerchant(sf::RenderWindow& window) const;
    void drawDialogueBubble(sf::RenderWindow& window) const;

    const sf::Font* font_ = nullptr;
    std::vector<sf::Texture> merchantFrames_;
    bool merchantLoaded_ = false;
    std::size_t merchantFrameIndex_ = 0;
    float merchantFrameTimer_ = 0.0f;
    ShopAction hoveredAction_;
    std::mt19937 dialogueRandomEngine_;
    std::string dialogueText_ = "不来点什么？";
    float dialogueTimer_ = 0.0f;
    float dialoguePauseTimer_ = 0.0f;
};
