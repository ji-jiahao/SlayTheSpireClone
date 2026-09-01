#pragma once

#include "card/Card.hpp"

#include <algorithm>
#include <string>
#include <vector>

struct GameState
{
    int currentHealth = 80;
    int maxHealth = 80;
    int gold = 99;
    unsigned int seed = 20260901;
    std::vector<CardInstance> deck;
    std::vector<std::string> relicIds{"burning_blood"};

    GameState()
    {
        reset();
    }

    void reset()
    {
        currentHealth = 80;
        maxHealth = 80;
        gold = 99;
        deck.clear();
        for (int index = 0; index < 5; ++index) deck.push_back({"strike", false});
        for (int index = 0; index < 4; ++index) deck.push_back({"defend", false});
        deck.push_back({"bash", false});
        relicIds = {"burning_blood"};
    }

    int heal(int amount)
    {
        const int oldHealth = currentHealth;
        currentHealth = std::min(maxHealth, currentHealth + std::max(0, amount));
        return currentHealth - oldHealth;
    }

    void loseHealth(int amount)
    {
        currentHealth = std::max(0, currentHealth - std::max(0, amount));
    }

    void gainGold(int amount)
    {
        gold += std::max(0, amount);
    }

    bool spendGold(int amount)
    {
        if (amount < 0 || amount > gold) return false;
        gold -= amount;
        return true;
    }

    void addCard(const std::string& cardId)
    {
        deck.push_back({cardId, false});
    }

    bool removeFirstCard(const std::string& cardId)
    {
        const auto found = std::find_if(deck.begin(), deck.end(), [&cardId](const CardInstance& card) {
            return card.definitionId == cardId;
        });
        if (found == deck.end()) return false;
        deck.erase(found);
        return true;
    }

    bool upgradeFirstCard()
    {
        const auto found = std::find_if(deck.begin(), deck.end(), [](const CardInstance& card) {
            return !card.upgraded;
        });
        if (found == deck.end()) return false;
        found->upgraded = true;
        return true;
    }

    bool upgradeCardAt(std::size_t index)
    {
        if (index >= deck.size() || deck[index].upgraded) return false;
        deck[index].upgraded = true;
        return true;
    }

    void increaseMaxHealth(int amount)
    {
        const int increase = std::max(0, amount);
        maxHealth += increase;
        currentHealth += increase;
    }
};
