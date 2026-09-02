#pragma once

#include "core/GameState.hpp"

class RestSystem
{
public:
    int getHealAmount(const GameState& state) const;
    int rest(GameState& state) const;
};
