#include "ui/EventView.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace
{
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

void drawRoundedFill(sf::RenderTarget& target, const sf::FloatRect& rect,
                     float radius, sf::Color fillColor)
{
    const float clampedRadius =
        std::min(radius, std::min(rect.size.x, rect.size.y) / 2.0f);

    sf::RectangleShape center({rect.size.x - 2.0f * clampedRadius, rect.size.y});
    center.setPosition({rect.position.x + clampedRadius, rect.position.y});
    center.setFillColor(fillColor);
    target.draw(center);

    sf::RectangleShape middle({rect.size.x, rect.size.y - 2.0f * clampedRadius});
    middle.setPosition({rect.position.x, rect.position.y + clampedRadius});
    middle.setFillColor(fillColor);
    target.draw(middle);

    sf::CircleShape corner(clampedRadius, 24);
    corner.setFillColor(fillColor);

    corner.setPosition({rect.position.x, rect.position.y});
    target.draw(corner);
    corner.setPosition({rect.position.x + rect.size.x - 2.0f * clampedRadius,
                        rect.position.y});
    target.draw(corner);
    corner.setPosition({rect.position.x,
                        rect.position.y + rect.size.y - 2.0f * clampedRadius});
    target.draw(corner);
    corner.setPosition({rect.position.x + rect.size.x - 2.0f * clampedRadius,
                        rect.position.y + rect.size.y - 2.0f * clampedRadius});
    target.draw(corner);
}

void drawRoundedBox(sf::RenderTarget& target, const sf::FloatRect& rect,
                    float radius, sf::Color fillColor, sf::Color outlineColor,
                    float outlineThickness)
{
    if (outlineThickness > 0.0f)
    {
        sf::FloatRect outlineRect(
            {rect.position.x - outlineThickness, rect.position.y - outlineThickness},
            {rect.size.x + 2.0f * outlineThickness,
             rect.size.y + 2.0f * outlineThickness});
        drawRoundedFill(target, outlineRect, radius + outlineThickness, outlineColor);
    }

    drawRoundedFill(target, rect, radius, fillColor);
}

sf::Color buttonFillColor(bool hovered)
{
    if (hovered)
    {
        return sf::Color(244, 209, 112);
    }

    return sf::Color(230, 174, 72);
}

std::string buildStatusText(const GameState& gameState)
{
    std::ostringstream out;
    out << "HP " << gameState.currentHealth << "/" << gameState.maxHealth
        << "    Gold " << gameState.gold;
    if (gameState.isDead())
    {
        out << "    已死亡";
    }

    return out.str();
}
} // namespace

bool EventView::loadFont(const std::string& fontPath)
{
    lastError_.clear();
    if (!font_.openFromFile(fontPath))
    {
        lastError_ = "无法加载字体: " + fontPath;
        return false;
    }

    return true;
}

bool EventView::prepareEvent(const EventDefinition& eventDefinition)
{
    preparedEvent_ = &eventDefinition;
    textures_.clear();
    floatingTexts_.clear();
    feedbackText_.clear();
    feedbackTimer_ = 0.0f;
    returnDelay_ = 0.0f;
    hurtFlash_ = false;
    hurtFlashTimer_ = 0.0f;
    hoveredOptionIndex_ = -1;
    returnToMap_ = false;
    lastError_.clear();
    stopSound();

    for (const EventState& state : eventDefinition.states)
    {
        if (state.imagePath.empty() || textures_.find(state.imagePath) != textures_.end())
        {
            continue;
        }

        sf::Texture texture;
        if (!texture.loadFromFile(state.imagePath))
        {
            lastError_ = "无法加载事件图片: " + state.imagePath;
            return false;
        }

        textures_.emplace(state.imagePath, std::move(texture));
    }

    return true;
}

void EventView::enterCurrentState(const EventSystem& eventSystem)
{
    activeStateIndex_ = eventSystem.getCurrentStateIndex();
    hoveredOptionIndex_ = -1;
    playStateSound(getCurrentState(eventSystem));
}

void EventView::update(float deltaSeconds)
{
    if (feedbackTimer_ > 0.0f)
    {
        feedbackTimer_ = std::max(0.0f, feedbackTimer_ - deltaSeconds);
        if (feedbackTimer_ <= 0.0f)
        {
            feedbackText_.clear();
        }
    }

    if (hurtFlashTimer_ > 0.0f)
    {
        hurtFlashTimer_ = std::max(0.0f, hurtFlashTimer_ - deltaSeconds);
        hurtFlash_ = hurtFlashTimer_ > 0.0f;
    }

    for (FloatingText& text : floatingTexts_)
    {
        text.lifetime -= deltaSeconds;
        text.position.y -= 44.0f * deltaSeconds;
        const float alphaRatio = std::clamp(text.lifetime / 1.2f, 0.0f, 1.0f);
        text.color.a = static_cast<std::uint8_t>(255.0f * alphaRatio);
    }

    floatingTexts_.erase(
        std::remove_if(floatingTexts_.begin(), floatingTexts_.end(),
                       [](const FloatingText& text)
                       {
                           return text.lifetime <= 0.0f;
                       }),
        floatingTexts_.end());

    if (returnToMap_ && returnDelay_ > 0.0f)
    {
        returnDelay_ = std::max(0.0f, returnDelay_ - deltaSeconds);
    }
}

void EventView::handleMouseMove(sf::Vector2f mousePosition,
                                const EventSystem& eventSystem)
{
    hoveredOptionIndex_ = -1;
    const sf::Vector2u assumedWindowSize(1280, 720);
    for (const OptionButton& button : layoutButtons(eventSystem, assumedWindowSize))
    {
        if (button.bounds.contains(mousePosition))
        {
            hoveredOptionIndex_ = static_cast<int>(button.optionIndex);
            return;
        }
    }
}

bool EventView::handleMouseClick(sf::Vector2f mousePosition, EventSystem& eventSystem,
                                 GameState& gameState)
{
    const sf::Vector2u assumedWindowSize(1280, 720);
    for (const OptionButton& button : layoutButtons(eventSystem, assumedWindowSize))
    {
        if (!button.bounds.contains(mousePosition))
        {
            continue;
        }

        const EventDefinition& eventDefinition = eventSystem.getCurrentEvent();
        const EventOption& option = eventDefinition.options[button.optionIndex];
        const GameState before = gameState;
        if (!eventSystem.chooseOption(static_cast<int>(button.optionIndex), gameState))
        {
            lastError_ = eventSystem.getLastError();
            return false;
        }

        addChoiceFeedback(option, before, gameState);
        if (eventSystem.isFinished())
        {
            stopSound();
            returnToMap_ = true;
            returnDelay_ = feedbackText_.empty() ? 0.85f : 1.5f;
        }
        else
        {
            enterCurrentState(eventSystem);
        }

        return true;
    }

    return false;
}

bool EventView::shouldReturnToMap() const
{
    return returnToMap_ && returnDelay_ <= 0.0f;
}

void EventView::clearReturnToMapRequest()
{
    returnToMap_ = false;
    returnDelay_ = 0.0f;
}

void EventView::draw(sf::RenderWindow& window, const EventSystem& eventSystem,
                     const GameState& gameState) const
{
    const sf::Vector2u size = window.getSize();
    const float width = static_cast<float>(size.x);
    const float height = static_cast<float>(size.y);

    sf::RectangleShape background({width, height});
    background.setFillColor(sf::Color(25, 25, 29));
    window.draw(background);

    for (int index = 0; index < 9; ++index)
    {
        sf::RectangleShape stone({width, 68.0f});
        stone.setPosition({0.0f, static_cast<float>(index) * 82.0f});
        const std::uint8_t shade = static_cast<std::uint8_t>(31 + (index % 2) * 9);
        stone.setFillColor(sf::Color(shade, shade, shade + 5));
        window.draw(stone);
    }

    const EventState& state = getCurrentState(eventSystem);
    const auto textureIt = textures_.find(state.imagePath);
    if (textureIt != textures_.end())
    {
        sf::Sprite sprite(textureIt->second);
        const sf::FloatRect localBounds = sprite.getLocalBounds();
        sprite.setOrigin({localBounds.position.x + localBounds.size.x / 2.0f,
                          localBounds.position.y + localBounds.size.y / 2.0f});

        const float maxImageWidth = width * 0.30f;
        const float maxImageHeight = height * 0.78f;
        const float scale = std::min(maxImageWidth / localBounds.size.x,
                                     maxImageHeight / localBounds.size.y);
        sprite.setScale({scale, scale});
        sprite.setPosition({width * 0.27f, height * 0.55f});
        window.draw(sprite);
    }

    const sf::FloatRect bubbleRect({width * 0.52f, height * 0.14f},
                                   {width * 0.39f, height * 0.22f});
    drawRoundedBox(window, bubbleRect, 22.0f, sf::Color(245, 242, 232),
                   sf::Color(32, 28, 24), 4.0f);

    sf::Text dialogue = makeText(font_, state.text, 32, sf::Color(20, 18, 16));
    dialogue.setPosition({bubbleRect.position.x + 34.0f, bubbleRect.position.y + 58.0f});
    window.draw(dialogue);

    const std::vector<OptionButton> buttons = layoutButtons(eventSystem, size);
    for (const OptionButton& button : buttons)
    {
        const bool hovered = hoveredOptionIndex_ == static_cast<int>(button.optionIndex);
        drawRoundedBox(window, button.bounds, 14.0f, buttonFillColor(hovered),
                       sf::Color(36, 28, 18), hovered ? 5.0f : 3.0f);

        const EventOption& option =
            eventSystem.getCurrentEvent().options[button.optionIndex];
        sf::Text label = makeText(font_, option.text, 24, sf::Color(24, 19, 14));
        const sf::FloatRect textBounds = label.getLocalBounds();
        label.setPosition({button.bounds.position.x +
                               (button.bounds.size.x - textBounds.size.x) / 2.0f -
                               textBounds.position.x,
                           button.bounds.position.y +
                               (button.bounds.size.y - textBounds.size.y) / 2.0f -
                               textBounds.position.y - 2.0f});
        window.draw(label);
    }

    sf::Text status = makeText(font_, buildStatusText(gameState), 22,
                               sf::Color(230, 224, 204));
    status.setPosition({34.0f, 24.0f});
    window.draw(status);

    if (!feedbackText_.empty())
    {
        sf::Text feedback = makeText(font_, feedbackText_, 20,
                                     sf::Color(236, 229, 205));
        feedback.setPosition({width * 0.52f, height * 0.79f});
        window.draw(feedback);
    }

    for (const FloatingText& floatingText : floatingTexts_)
    {
        sf::Text drawableText =
            makeText(font_, floatingText.text, 24, floatingText.color);
        drawableText.setOutlineThickness(2.0f);
        drawableText.setOutlineColor(sf::Color(30, 20, 20, floatingText.color.a));
        drawableText.setPosition(floatingText.position);
        window.draw(drawableText);
    }

    if (hurtFlash_)
    {
        sf::RectangleShape flash({width, height});
        flash.setFillColor(sf::Color(180, 28, 28, 75));
        window.draw(flash);
    }
}

const std::string& EventView::getLastError() const
{
    return lastError_;
}

std::vector<EventView::OptionButton> EventView::layoutButtons(
    const EventSystem& eventSystem, sf::Vector2u windowSize) const
{
    std::vector<OptionButton> buttons;
    if (!eventSystem.hasActiveEvent())
    {
        return buttons;
    }

    const float width = static_cast<float>(windowSize.x);
    const float height = static_cast<float>(windowSize.y);
    const float buttonWidth = width * 0.39f;
    const float buttonHeight = 72.0f;
    const float gap = 18.0f;
    const float startX = width * 0.52f;
    const float startY = height * 0.46f;
    const int currentState = static_cast<int>(eventSystem.getCurrentStateIndex());

    const EventDefinition& eventDefinition = eventSystem.getCurrentEvent();
    std::size_t visibleIndex = 0;
    for (std::size_t index = 0; index < eventDefinition.options.size(); ++index)
    {
        if (eventDefinition.options[index].state != currentState)
        {
            continue;
        }

        OptionButton button;
        button.optionIndex = index;
        button.bounds = sf::FloatRect(
            {startX, startY + static_cast<float>(visibleIndex) * (buttonHeight + gap)},
            {buttonWidth, buttonHeight});
        buttons.push_back(button);
        ++visibleIndex;
    }

    return buttons;
}

const EventState& EventView::getCurrentState(const EventSystem& eventSystem) const
{
    const EventDefinition& eventDefinition = eventSystem.getCurrentEvent();
    const std::size_t stateIndex =
        std::min(eventSystem.getCurrentStateIndex(), eventDefinition.states.size() - 1);
    return eventDefinition.states[stateIndex];
}

void EventView::playStateSound(const EventState& state)
{
    stopSound();
    if (state.soundPath.empty())
    {
        return;
    }

    if (!sound_.openFromFile(state.soundPath))
    {
        lastError_ = "无法加载事件音频: " + state.soundPath;
        return;
    }

    sound_.setVolume(100.0f);
    sound_.setLooping(false);
    sound_.play();
}

void EventView::stopSound()
{
    sound_.stop();
}

void EventView::addChoiceFeedback(const EventOption& option, const GameState& before,
                                  const GameState& after)
{
    if (after.currentHealth < before.currentHealth)
    {
        FloatingText healthText;
        healthText.text = "-" + std::to_string(before.currentHealth - after.currentHealth) + " HP";
        healthText.position = {300.0f, 210.0f};
        healthText.color = sf::Color(230, 70, 60);
        healthText.lifetime = 1.2f;
        floatingTexts_.push_back(healthText);

        hurtFlash_ = true;
        hurtFlashTimer_ = 0.28f;
    }

    if (after.gold > before.gold)
    {
        FloatingText goldText;
        goldText.text = "+" + std::to_string(after.gold - before.gold) + " Gold";
        goldText.position = {170.0f, 58.0f};
        goldText.color = sf::Color(247, 197, 60);
        goldText.lifetime = 1.2f;
        floatingTexts_.push_back(goldText);
    }

    if (option.text == "艰难绷住")
    {
        feedbackText_ = "你绷住了，但内心毫无波澜。";
        feedbackTimer_ = 1.5f;
    }
}
