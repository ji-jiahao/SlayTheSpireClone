#pragma once

#include "combat/CombatSystem.hpp"

namespace CardPresentation
{
// Display-only copies. Never feed these back into cost calculation or playCard.
inline Card forCombat(const Card& card, const CombatSystem& combat)
{
    Card display = card;
    // X remains X; -2 means unplayable, not an X-cost card.
    if (card.cost >= 0) display.cost = combat.getPlayableCardCost(card);
    return display;
}

inline std::vector<Card> handForCombat(const CombatSystem& combat)
{
    auto cards = combat.getHandCards();
    for (auto& card : cards) card = forCombat(card, combat);
    return cards;
}

inline std::string costLabel(int cost)
{
    if (cost == -1) return "X";
    if (cost < -1) return "—";
    return std::to_string(cost);
}
}
