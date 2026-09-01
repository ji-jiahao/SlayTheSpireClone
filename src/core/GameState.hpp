#pragma once

#include <algorithm>
#include <string>
#include <vector>

struct GameState
{
    int currentHealth = 80;
    int maxHealth = 80;
    std::vector<std::string> relicIds{"burning_blood"};

    int heal(int amount)
    {
        const int oldHealth = currentHealth;
        currentHealth = std::min(maxHealth, currentHealth + std::max(0, amount));
        return currentHealth - oldHealth;
    }
};
