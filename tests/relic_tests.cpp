#include "core/GameState.hpp"
#include "relic/RelicDatabase.hpp"
#include "relic/RelicSystem.hpp"

#include <cassert>
#include <iostream>

int main()
{
    assert(RelicDatabase::createFoundationRelics().size() == 12);
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

    GameState pickupState;
    RelicSystem pickupRelics;
    assert(pickupRelics.obtainRelic(pickupState, "strawberry"));
    assert(pickupState.maxHealth == 87);
    assert(pickupState.currentHealth == 87);
    assert(!pickupRelics.obtainRelic(pickupState, "strawberry"));
    assert(pickupState.maxHealth == 87);

    const int oldGold = pickupState.gold;
    assert(pickupRelics.obtainRelic(pickupState, "old_coin"));
    assert(pickupState.gold == oldGold + 300);

    assert(pickupRelics.obtainRelic(pickupState, "anchor"));
    assert(pickupRelics.obtainRelic(pickupState, "vajra"));
    assert(pickupRelics.obtainRelic(pickupState, "lantern"));
    assert(pickupRelics.obtainRelic(pickupState, "bag_of_preparation"));
    const RelicBattleStartModifiers modifiers = pickupRelics.applyBattleStart(pickupState);
    assert(modifiers.block == 10);
    assert(modifiers.strength == 1);
    assert(modifiers.energy == 1);
    assert(modifiers.drawCards == 2);

    GameState upgradeState;
    RelicSystem upgradeRelics;
    assert(upgradeRelics.obtainRelic(upgradeState, "whetstone"));
    int upgradedAttacks = 0;
    for (const CardInstance& card : upgradeState.deck)
    {
        if (card.upgraded && (card.definitionId == "strike" || card.definitionId == "bash"))
        {
            ++upgradedAttacks;
        }
    }
    assert(upgradedAttacks == 2);

    std::cout << "Relic tests passed.\n";
    return 0;
}
