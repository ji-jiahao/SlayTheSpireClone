#pragma once

#include <string>
#include <vector>

enum class EventEffectType
{
    None,
    Heal,
    LoseHealth,
    GainGold,
    LoseGold,
    AddCard,
    RemoveCard,
    UpgradeCard
};

struct EventEffect
{
    EventEffectType type = EventEffectType::None;
    int value = 0;
    std::string parameter;
};

struct EventOption
{
    std::string text;
    std::string condition;
    std::vector<EventEffect> effects;
    int state = 0;
    int nextState = -1;
    bool closesEvent = true;
};

struct EventState
{
    std::string text;
    std::string imagePath;
    std::string soundPath;
};

struct EventDefinition
{
    std::string id;
    std::string title;
    std::string description;
    std::string backgroundPath;
    std::string imagePath;
    std::string soundPath;
    int act = 1;
    int weight = 0;
    std::vector<EventState> states;
    std::vector<EventOption> options;
};

std::string eventEffectTypeToString(EventEffectType type);
EventEffectType eventEffectTypeFromString(const std::string& typeName);
