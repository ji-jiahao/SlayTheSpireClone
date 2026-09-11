#pragma once

#include "combat/CombatSystem.hpp"

namespace CardPresentation
{
inline std::string typeLabel(const Card& card)
{
    if (card.rarity == CardRarity::Status) return "状态";
    if (card.rarity == CardRarity::Curse) return "诅咒";
    switch (card.type)
    {
    case CardType::Attack: return "攻击";
    case CardType::Skill: return "技能";
    case CardType::Power: return "能力";
    }
    return "卡牌";
}

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
