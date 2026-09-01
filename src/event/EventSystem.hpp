#pragma once

#include "core/GameState.hpp"
#include "event/EventDatabase.hpp"

#include <cstddef>
#include <string>

class EventSystem
{
public:
    explicit EventSystem(const EventDatabase& database);

    void setDatabase(const EventDatabase& database);
    bool startEvent(const std::string& eventId);
    bool chooseOption(int optionIndex, GameState& gameState);
    const EventDefinition& getCurrentEvent() const;
    std::size_t getCurrentStateIndex() const;
    bool isFinished() const;
    bool hasActiveEvent() const;
    const std::string& getLastError() const;

private:
    bool applyEffect(const EventEffect& effect, GameState& gameState);

    const EventDatabase* database_;
    const EventDefinition* currentEvent_;
    std::size_t currentStateIndex_;
    bool finished_;
    std::string lastError_;
};
