#include "core/GameState.hpp"
#include "relic/RelicDatabase.hpp"
#include "relic/RelicSystem.hpp"

#include <cassert>
#include <iostream>

int main()
{
    const Relic burningBlood = RelicDatabase::createBurningBlood();
    assert(burningBlood.id == "burning_blood");

    GameState state;
    state.currentHealth = 70;
    RelicSystem relics;
    relics.beginBattle();
    assert(relics.applyBattleVictory(state) == 6);
    assert(state.currentHealth == 76);
    assert(relics.applyBattleVictory(state) == 0);
    assert(state.currentHealth == 76);

    state.currentHealth = 78;
    relics.beginBattle();
    assert(relics.applyBattleVictory(state) == 2);
    assert(state.currentHealth == 80);

    std::cout << "Relic tests passed.\n";
    return 0;
}
