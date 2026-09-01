#include "card/CardDatabase.hpp"
#include "combat/CombatSystem.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace
{
int findCard(const CombatSystem& combat, const std::string& id)
{
    const auto& hand = combat.getHandCards();
    for (std::size_t index = 0; index < hand.size(); ++index)
    {
        if (hand[index].id == id)
        {
            return static_cast<int>(index);
        }
    }
    return -1;
}

std::vector<Card> fiveCopies(const Card& card)
{
    return std::vector<Card>(5, card);
}
}

int main()
{
    {
        CombatSystem combat;
        combat.startBattle(80, 0, fiveCopies(CardDatabase::createStrike()));
        assert(combat.playCard(0));
        assert(combat.getEnemy().getCurrentHealth() == 34);
    }

    {
        CombatSystem combat;
        combat.startBattle(80, 0, fiveCopies(CardDatabase::createDefend()));
        assert(combat.playCard(0));
        assert(combat.getPlayer().getBlock() == 5);
        combat.endPlayerTurn();
        assert(combat.getPlayer().getCurrentHealth() == 79);
    }

    {
        std::vector<Card> cards = {CardDatabase::createBash(), CardDatabase::createStrike(),
                                   CardDatabase::createDefend(), CardDatabase::createDefend(),
                                   CardDatabase::createDefend()};
        CombatSystem combat;
        combat.startBattle(80, 0, cards);
        assert(combat.playCard(findCard(combat, "bash")));
        assert(combat.getEnemy().getCurrentHealth() == 32);
        assert(combat.getEnemy().getVulnerable() == 2);
        assert(combat.playCard(findCard(combat, "strike")));
        assert(combat.getEnemy().getCurrentHealth() == 23);
    }

    {
        CombatSystem combat;
        combat.startBattle(80, 0, fiveCopies(CardDatabase::createBash()));
        assert(combat.playCard(0));
        assert(!combat.playCard(0));
        assert(combat.getPlayer().getCurrentEnergy() == 1);
    }

    {
        Card finisher = CardDatabase::createStrike();
        finisher.cost = 0;
        finisher.effects[0].value = 40;
        CombatSystem combat;
        combat.startBattle(80, 0, fiveCopies(finisher));
        assert(combat.playCard(0));
        assert(combat.getResult() == BattleResult::Victory);
    }

    {
        CombatSystem combat;
        combat.startBattle(80, 0, CardDatabase::createStarterDeck(),
                           EncounterDefinition{}, 10, 1, 1, 2, 80);
        assert(combat.getPlayer().getBlock() == 10);
        assert(combat.getPlayer().getStrength() == 1);
        assert(combat.getPlayer().getCurrentEnergy() == 4);
        assert(combat.getHandCards().size() == 7);
    }

    std::cout << "Combat tests passed.\n";
    return 0;
}
