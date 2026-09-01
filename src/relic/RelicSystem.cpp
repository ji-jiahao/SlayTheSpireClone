#include "relic/RelicSystem.hpp"

#include <algorithm>

void RelicSystem::beginBattle()
{
    battleVictoryApplied = false;
}

int RelicSystem::applyBattleVictory(GameState& state)
{
    if (battleVictoryApplied)
    {
        return 0;
    }
    battleVictoryApplied = true;

    const auto relic = std::find(state.relicIds.begin(), state.relicIds.end(), "burning_blood");
    return relic == state.relicIds.end() ? 0 : state.heal(6);
}
