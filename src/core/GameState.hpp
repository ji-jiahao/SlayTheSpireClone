#pragma once

#include <algorithm>
#include <string>
#include <vector>

struct GameState
{
    int currentHealth = 80;
    int maxHealth = 80;
    int gold = 99;
    int currentNodeId = -1;
    unsigned int seed = 0;
    std::vector<std::string> deckIds{"strike", "strike", "strike", "strike", "strike",
                                     "defend", "defend", "defend", "defend", "bash"};
    std::vector<std::string> relicIds{"burning_blood"};
    std::vector<std::string> potionIds;
    std::vector<std::string> visitedEventIds;

    int heal(int amount)
    {
        const int oldHealth = currentHealth;
        currentHealth = std::min(maxHealth, currentHealth + std::max(0, amount));
        return currentHealth - oldHealth;
    }

    void loseHealth(int amount)
    {
        if (amount <= 0)
        {
            return;
        }

        currentHealth = std::max(0, currentHealth - amount);
    }

    void gainGold(int amount)
    {
        if (amount > 0)
        {
            gold += amount;
        }
    }

    bool spendGold(int amount)
    {
        if (amount <= 0)
        {
            return true;
        }

        if (gold < amount)
        {
            return false;
        }

        gold -= amount;
        return true;
    }

    void loseAllGold()
    {
        gold = 0;
    }

    bool hasVisitedEvent(const std::string& eventId) const
    {
        return std::find(visitedEventIds.begin(), visitedEventIds.end(), eventId)
               != visitedEventIds.end();
    }

    void markEventVisited(const std::string& eventId)
    {
        if (eventId.empty() || hasVisitedEvent(eventId))
        {
            return;
        }

        visitedEventIds.push_back(eventId);
    }

    bool isDead() const
    {
        return currentHealth <= 0;
    }

    void addCard(const std::string& cardId)
    {
        if (!cardId.empty())
        {
            deckIds.push_back(cardId);
        }
    }

    bool removeCard(const std::string& cardId)
    {
        auto it = std::find(deckIds.begin(), deckIds.end(), cardId);
        if (it == deckIds.end())
        {
            return false;
        }

        deckIds.erase(it);
        return true;
    }

    bool upgradeCard(const std::string& cardId)
    {
        auto it = std::find(deckIds.begin(), deckIds.end(), cardId);
        if (it == deckIds.end())
        {
            return false;
        }

        if (!it->empty() && it->back() == '+')
        {
            return false;
        }

        *it = cardId + "+";
        return true;
    }
};

using RunState = GameState;
