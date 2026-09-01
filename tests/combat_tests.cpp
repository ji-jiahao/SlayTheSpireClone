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
        assert(combat.getPlayer().getCurrentHealth() == 80);
        assert(combat.getEnemy().getStrength() == 3);
        assert(combat.getEnemy().getIntent().name == "黑暗打击");
        assert(combat.getEnemyIntentDamage() == 9);
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

    {
        CombatSystem combat;
        combat.startBattle(80, 5, CardDatabase::createStarterDeck(),
                           {"jaw_worm", "颚虫", 42, 11}, 0, 0);
        assert(combat.getEnemy().getIntent().name == "咬击");
        assert(combat.getEnemyIntentDamage() == 11);
        combat.endPlayerTurn();
        assert(combat.getPlayer().getCurrentHealth() == 69);
        assert(combat.getEnemy().getIntent().name == "吼叫" ||
               combat.getEnemy().getIntent().name == "痛击");
    }

    {
        Card wakeAttack = CardDatabase::createStrike();
        wakeAttack.cost = 0;
        CombatSystem combat;
        combat.startBattle(80, 7, fiveCopies(wakeAttack),
                           {"lagavulin", "乐加维林", 110, 18}, 0, 0);
        assert(combat.getEnemy().getIntent().type == EnemyIntentType::Sleep);
        assert(combat.playCard(0));
        assert(combat.getEnemy().getIntent().type == EnemyIntentType::Stunned);
        combat.endPlayerTurn();
        assert(combat.getPlayer().getCurrentHealth() == 80);
        assert(combat.getEnemy().getIntent().type == EnemyIntentType::Attack);
        assert(combat.getEnemyIntentDamage() == 18);
    }

    {
        Card finisher = CardDatabase::createStrike();
        finisher.cost = 0;
        finisher.effects[0].value = 30;
        CombatSystem combat;
        combat.startBattle(80, 9, fiveCopies(finisher),
                           {"fungi_beast", "真菌兽", 25, 6}, 0, 0);
        assert(combat.playCard(0));
        assert(combat.getResult() == BattleResult::Victory);
        assert(combat.getPlayer().getVulnerable() == 2);
    }

    {
        Card splitAttack = CardDatabase::createStrike();
        splitAttack.cost = 0;
        splitAttack.effects[0].value = 70;
        CombatSystem combat;
        combat.startBattle(80, 11, fiveCopies(splitAttack),
                           {"slime_boss", "史莱姆老大", 140, 35}, 0, 0);
        assert(combat.playCard(0));
        assert(combat.getEnemy().getIntent().type == EnemyIntentType::Split);
        combat.endPlayerTurn();
        assert(combat.getEnemy().getArchetype() == EnemyArchetype::SlimePair);
        assert(combat.getEnemy().getCurrentHealth() == 140);
    }

    {
        bool verifiedSlimed = false;
        for (std::uint32_t seed = 0; seed < 100 && !verifiedSlimed; ++seed)
        {
            CombatSystem combat;
            combat.startBattle(80, seed, CardDatabase::createStarterDeck(),
                               {"acid_slime", "酸液史莱姆", 30, 10}, 0, 0);
            if (combat.getEnemy().getIntent().slimed == 0) continue;
            combat.endPlayerTurn();
            assert(!combat.getDeck().getDiscardPile().empty());
            assert(combat.getDeck().getDiscardPile().back().id == "slimed");
            verifiedSlimed = true;
        }
        assert(verifiedSlimed);
    }

    std::cout << "Combat tests passed.\n";
    return 0;
}
