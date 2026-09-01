#include "event/EventSystem.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

EventSystem::EventSystem(const EventDatabase& database)
    : database_(&database),
      currentEvent_(nullptr),
      currentStateIndex_(0),
      finished_(true)
{
}

void EventSystem::setDatabase(const EventDatabase& database)
{
    database_ = &database;
    currentEvent_ = nullptr;
    currentStateIndex_ = 0;
    finished_ = true;
    lastError_.clear();
}

bool EventSystem::startEvent(const std::string& eventId)
{
    if (database_ == nullptr)
    {
        lastError_ = "事件数据库未设置";
        return false;
    }

    try
    {
        currentEvent_ = &database_->getEvent(eventId);
        currentStateIndex_ = 0;
        finished_ = false;
        lastError_.clear();
        return true;
    }
    catch (const std::exception& exception)
    {
        currentEvent_ = nullptr;
        currentStateIndex_ = 0;
        finished_ = true;
        lastError_ = exception.what();
        return false;
    }
}

bool EventSystem::chooseOption(int optionIndex, GameState& gameState)
{
    if (!hasActiveEvent())
    {
        lastError_ = "当前没有激活的事件";
        return false;
    }

    if (optionIndex < 0 || static_cast<std::size_t>(optionIndex) >= currentEvent_->options.size())
    {
        lastError_ = "事件选项索引越界";
        return false;
    }

    GameState trialState = gameState;
    const EventOption& option = currentEvent_->options[static_cast<std::size_t>(optionIndex)];
    if (option.state != static_cast<int>(currentStateIndex_))
    {
        lastError_ = "事件选项不属于当前阶段";
        return false;
    }

    for (const EventEffect& effect : option.effects)
    {
        if (!applyEffect(effect, trialState))
        {
            lastError_ = "事件效果执行失败: " + eventEffectTypeToString(effect.type);
            return false;
        }
    }

    if (!option.closesEvent && option.nextState >= 0)
    {
        const bool hasNextState = std::any_of(
            currentEvent_->options.begin(),
            currentEvent_->options.end(),
            [&option](const EventOption& candidate)
            {
                return candidate.state == option.nextState;
            });

        if (!hasNextState)
        {
            lastError_ = "事件下一阶段索引越界";
            return false;
        }

        gameState = std::move(trialState);
        currentStateIndex_ = static_cast<std::size_t>(option.nextState);
        lastError_.clear();
        return true;
    }

    gameState = std::move(trialState);
    gameState.markEventVisited(currentEvent_->id);
    finished_ = true;
    lastError_.clear();
    return true;
}

const EventDefinition& EventSystem::getCurrentEvent() const
{
    if (currentEvent_ == nullptr)
    {
        throw std::runtime_error("当前没有激活的事件");
    }

    return *currentEvent_;
}

std::size_t EventSystem::getCurrentStateIndex() const
{
    return currentStateIndex_;
}

bool EventSystem::isFinished() const
{
    return finished_;
}

bool EventSystem::hasActiveEvent() const
{
    return currentEvent_ != nullptr && !finished_;
}

const std::string& EventSystem::getLastError() const
{
    return lastError_;
}

bool EventSystem::applyEffect(const EventEffect& effect, GameState& gameState)
{
    const int repeatCount = effect.value <= 0 ? 1 : effect.value;
    switch (effect.type)
    {
    case EventEffectType::None:
        return true;
    case EventEffectType::Heal:
        gameState.heal(repeatCount);
        return true;
    case EventEffectType::LoseHealth:
        gameState.loseHealth(repeatCount);
        return true;
    case EventEffectType::GainGold:
        gameState.gainGold(repeatCount);
        return true;
    case EventEffectType::LoseGold:
        return gameState.spendGold(repeatCount);
    case EventEffectType::AddCard:
        if (effect.parameter.empty())
        {
            return false;
        }

        for (int index = 0; index < repeatCount; ++index)
        {
            gameState.addCard(effect.parameter);
        }
        return true;
    case EventEffectType::RemoveCard:
        if (effect.parameter.empty())
        {
            return false;
        }

        for (int index = 0; index < repeatCount; ++index)
        {
            if (!gameState.removeCard(effect.parameter))
            {
                return false;
            }
        }
        return true;
    case EventEffectType::UpgradeCard:
        if (effect.parameter.empty())
        {
            return false;
        }

        for (int index = 0; index < repeatCount; ++index)
        {
            if (!gameState.upgradeCard(effect.parameter))
            {
                return false;
            }
        }
        return true;
    }

    return false;
}
