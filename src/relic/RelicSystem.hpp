#pragma once

#include "core/GameState.hpp"

class RelicSystem
{
public:
    void beginBattle();
    int applyBattleVictory(GameState& state);

private:
    bool battleVictoryApplied = false;
};
