#pragma once

#include "event/Event.hpp"

#include <string>
#include <unordered_map>
#include <vector>

class EventDatabase
{
public:
    bool loadFromFile(const std::string& filePath);
    const EventDefinition& getEvent(const std::string& eventId) const;
    std::vector<std::string> getActOneEventIds() const;
    std::vector<std::string> getEventIdsForAct(int act) const;
    bool hasEvent(const std::string& eventId) const;
    std::size_t getEventCount() const;
    const std::string& getLastError() const;

private:
    std::unordered_map<std::string, EventDefinition> events_;
    std::unordered_map<int, std::vector<std::string>> eventIdsByAct_;
    std::string lastError_;
};
