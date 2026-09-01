#include "card/CardDatabase.hpp"

Card CardDatabase::createStrike()
{
    return {
        "strike",
        "Strike",
        CardType::Attack,
        1,
        6,
        0,
        "Deal 6 damage."
    };
}

Card CardDatabase::createDefend()
{
    return {
        "defend",
        "Defend",
        CardType::Skill,
        1,
        0,
        5,
        "Gain 5 Block."
    };
}

Card CardDatabase::createBash()
{
    return {
        "bash",
        "Bash",
        CardType::Attack,
        2,
        8,
        0,
        "Deal 8 damage."
    };
}

std::vector<Card> CardDatabase::createStarterDeck()
{
    std::vector<Card> starterDeck;
    starterDeck.reserve(10);

    for (int index = 0; index < 5; ++index)
    {
        starterDeck.push_back(createStrike());
    }

    for (int index = 0; index < 4; ++index)
    {
        starterDeck.push_back(createDefend());
    }

    starterDeck.push_back(createBash());
    return starterDeck;
}
