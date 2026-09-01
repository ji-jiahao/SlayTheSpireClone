#pragma once

#include "core/GameState.hpp"
#include "event/EventSystem.hpp"

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

class EventView
{
public:
    bool loadFont(const std::string& fontPath);
    bool prepareEvent(const EventDefinition& eventDefinition);
    void enterCurrentState(const EventSystem& eventSystem);
    void update(float deltaSeconds);
    void handleMouseMove(sf::Vector2f mousePosition, const EventSystem& eventSystem);
    bool handleAnyInput(EventSystem& eventSystem, GameState& gameState);
    bool handleMouseClick(sf::Vector2f mousePosition, EventSystem& eventSystem,
                          GameState& gameState);
    bool shouldReturnToMap() const;
    void clearReturnToMapRequest();
    void draw(sf::RenderWindow& window, const EventSystem& eventSystem,
              const GameState& gameState) const;
    const std::string& getLastError() const;

private:
    struct OptionButton
    {
        sf::FloatRect bounds;
        std::size_t optionIndex = 0;
    };

    struct FloatingText
    {
        std::string text;
        sf::Vector2f position;
        sf::Color color;
        float lifetime = 0.0f;
    };

    const EventDefinition* preparedEvent_ = nullptr;
    sf::Font font_;
    sf::Texture backgroundTexture_;
    bool hasBackgroundTexture_ = false;
    std::unordered_map<std::string, sf::Texture> textures_;
    sf::Music sound_;
    std::size_t activeStateIndex_ = 0;
    int hoveredOptionIndex_ = -1;
    bool returnToMap_ = false;
    bool hurtFlash_ = false;
    float hurtFlashTimer_ = 0.0f;
    float feedbackTimer_ = 0.0f;
    float returnDelay_ = 0.0f;
    std::string feedbackText_;
    std::vector<FloatingText> floatingTexts_;
    std::string lastError_;

    std::vector<OptionButton> layoutButtons(const EventSystem& eventSystem,
                                            sf::Vector2u windowSize) const;
    const EventState& getCurrentState(const EventSystem& eventSystem) const;
    bool isChoiceBannerState(const EventState& state) const;
    void playStateSound(const EventState& state);
    void stopSound();
    void addChoiceFeedback(const EventOption& option, const GameState& before,
                           const GameState& after);
};
