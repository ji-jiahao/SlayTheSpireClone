#pragma once

#include "card/Card.hpp"

#include <vector>

class CardDatabase
{
public:
    static Card createStrike();
    static Card createDefend();
    static Card createBash();
    static std::vector<Card> createStarterDeck();
};
