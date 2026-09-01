#pragma once

#include "core/GameState.hpp"

#include <string>

struct RelicBattleStartModifiers
{
    int block = 0;
    int strength = 0;
    int energy = 0;
    int drawCards = 0;
    int healing = 0;
};

class RelicSystem
{
public:
    void beginBattle();
    bool obtainRelic(GameState& state, const std::string& relicId);
    RelicBattleStartModifiers applyBattleStart(GameState& state) const;
    int applyBattleVictory(GameState& state);

    int getBattleStartBlock(const GameState& state) const;
    int getBattleStartStrength(const GameState& state) const;

private:
    int upgradeCards(GameState& state, CardType type, int count) const;
    bool battleVictoryApplied = false;
};
