#pragma once

#include "card/Card.hpp"

#include <string>
#include <vector>

class CardDatabase
{
public:
    static Card createStrike();
    static Card createDefend();
    static Card createBash();
    static std::vector<Card> createStarterDeck();
    static Card createById(const std::string& id);
    static Card createFromInstance(const CardInstance& instance);
    static std::vector<Card> createIroncladCardPool();
};
