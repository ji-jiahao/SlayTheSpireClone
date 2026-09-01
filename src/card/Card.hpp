#pragma once

#include <string>

enum class CardType
{
    Attack,
    Skill,
    Power
};

struct Card
{
    std::string id;
    std::string name;
    CardType type = CardType::Skill;
    int cost = 0;
    int damage = 0;
    int block = 0;
    std::string description;
};
