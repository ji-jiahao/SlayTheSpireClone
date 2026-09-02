#include "room/RestSystem.hpp"

#include <algorithm>

int RestSystem::getHealAmount(const GameState& state) const
{
    return std::max(1, state.maxHealth * 30 / 100);
}

int RestSystem::rest(GameState& state) const
{
    return state.heal(getHealAmount(state));
}
