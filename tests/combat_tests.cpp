#include "card/CardDatabase.hpp"
#include "combat/CombatSystem.hpp"
#include "ui/CardPresentation.hpp"

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

bool pileContains(const std::vector<Card>& cards, const std::string& id)
{
    for (const Card& card : cards)
    {
        if (card.id == id)
        {
            return true;
        }
    }
    return false;
}

}

int main()
{
    {
        // 本项目约定：打出的能力牌进入本场战斗的消耗牌堆。
        std::vector<Card> cards = {CardDatabase::createById("inflame"),
                                   CardDatabase::createStrike(), CardDatabase::createStrike(),
                                   CardDatabase::createDefend(), CardDatabase::createDefend()};
        CombatSystem combat;
        combat.startBattle(80, 7, cards);
        const int index = findCard(combat, "inflame");
        assert(index >= 0);
        assert(combat.playCard(index));
        assert(!pileContains(combat.getDeck().getDiscardPile(), "inflame"));
        assert(pileContains(combat.getDeck().getExhaustPile(), "inflame"));
        assert(combat.getHandCards().size() == 4);
        assert(combat.getDeck().getExhaustPile().size() == 1);
        assert(combat.getPlayer().getStrength() == 2);
        combat.endPlayerTurn();
        assert(!pileContains(combat.getHandCards(), "inflame"));
        assert(!pileContains(combat.getDeck().getDrawPile(), "inflame"));
        assert(combat.getDeck().getExhaustPile().size() == 1);
        assert(combat.getPlayer().getStrength() == 2);
    }

    {
        Card power = CardDatabase::createById("inflame");
        power.effects.push_back({CardEffectType::Exhaust, 1, CardTarget::Self, {}});
        CombatSystem combat;
        combat.startBattle(80, 7, {power});
        assert(combat.playCard(0));
        assert(combat.getHandCards().empty());
        assert(combat.getDeck().getExhaustPile().size() == 1);
        assert(combat.getDeck().getDiscardPile().empty());
        assert(combat.getPlayer().getStrength() == 2);
    }

    {
        // Discounted copies must display the actual price without mutating the deck.
        CombatSystem combat;
        combat.startBattle(80, 8, {CardDatabase::createById("corruption"),
                                  CardDatabase::createDefend(), CardDatabase::createStrike()});
        const auto originalDefend = CardDatabase::createDefend();
        assert(CardPresentation::forCombat(originalDefend, combat).cost == 1);
        assert(combat.playCard(findCard(combat, "corruption")));
        const auto& actual = combat.getHandCards()[findCard(combat, "defend")];
        assert(CardPresentation::forCombat(actual, combat).cost == 0);
        assert(actual.cost == 1);
        const int energy = combat.getPlayer().getCurrentEnergy();
        assert(combat.playCard(findCard(combat, "defend")));
        assert(combat.getPlayer().getCurrentEnergy() == energy);
        assert(combat.getDeck().getExhaustPile().size() == 2);
        assert(CardPresentation::forCombat(combat.getDeck().getExhaustPile().back(), combat).cost == 0);
        assert(CardPresentation::forCombat(CardDatabase::createStrike(), combat).cost == 1);
        combat.startBattle(80, 9, {originalDefend});
        assert(CardPresentation::forCombat(combat.getHandCards()[0], combat).cost == 1);
    }

    {
        CombatSystem combat;
        combat.startBattle(80, 9, {CardDatabase::createById("blood_for_blood"),
                                  CardDatabase::createById("bloodletting")});
        assert(combat.playCard(findCard(combat, "bloodletting")));
        const auto& original = combat.getHandCards()[findCard(combat, "blood_for_blood")];
        assert(original.cost == 4);
        assert(CardPresentation::forCombat(original, combat).cost == 3);
        const int energy = combat.getPlayer().getCurrentEnergy();
        assert(combat.playCard(findCard(combat, "blood_for_blood")));
        assert(combat.getPlayer().getCurrentEnergy() == energy - 3);
    }

    {
        CombatSystem combat;
        combat.startBattle(80, 10, {CardDatabase::createById("infernal_blade")});
        assert(combat.playCard(0));
        assert(combat.getHandCards().size() == 1);
        assert(CardPresentation::forCombat(combat.getHandCards()[0], combat).cost == 0);
        Card upgraded = CardDatabase::createById("havoc");
        assert(upgraded.upgrade());
        assert(CardPresentation::forCombat(upgraded, combat).cost == 0);
        assert(CardPresentation::costLabel(0) == "0");
        assert(CardPresentation::costLabel(2) == "2");
        assert(CardPresentation::costLabel(-1) == "X");
        assert(CardPresentation::costLabel(-2) == "—");
        assert(CardPresentation::forCombat(CardDatabase::createById("whirlwind"), combat).cost == -1);
        Card status; status.cost = -2;
        assert(CardPresentation::forCombat(status, combat).cost == -2);
    }

    {
        CombatSystem combat;
        combat.startBattle(80, 0, fiveCopies(CardDatabase::createStrike()));
        const bool played = combat.playCard(0);
        assert(played);
        assert(combat.getEnemy().getCurrentHealth() == 34);
    }

    {
        // 牌堆多于一手牌时，结束回合必须弃掉旧手牌并补满下一手。
        CombatSystem combat;
        combat.startBattle(80, 1,
                           std::vector<Card>(10, CardDatabase::createDefend()));
        assert(combat.getHandCards().size() == 5);
        assert(combat.getDeck().getDrawPile().size() == 5);
        combat.endPlayerTurn();
        assert(combat.getHandCards().size() == 5);
        assert(combat.getDeck().getDiscardPile().size() == 5);
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
                           {"颚虫", 42, 11, "jaw_worm"}, 0, 0);
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
                           {"乐加维林", 110, 18, "lagavulin"}, 0, 0);
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
                           {"真菌兽", 25, 6, "fungi_beast"}, 0, 0);
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
                           {"史莱姆老大", 140, 35, "slime_boss"}, 0, 0);
        assert(combat.playCard(0));
        assert(combat.getEnemy().getIntent().type == EnemyIntentType::Split);
        combat.endPlayerTurn();
        assert(combat.getEnemy().getArchetype() == EnemyArchetype::SlimePair);
        assert(combat.getEnemy().getCurrentHealth() == 140);
    }

    {
        CombatSystem combat;
        combat.startBattle(80, 7, CardDatabase::createStarterDeck(),
                           {"酸液史莱姆", 30, 10, "acid_slime"}, 0, 0);
        assert(combat.getEnemy().getIntent().slimed > 0);
        combat.endPlayerTurn();
        assert(!combat.getDeck().getDiscardPile().empty());
        assert(combat.getDeck().getDiscardPile().back().id == "slimed");
    }

    {
        Card heal;
        heal.id = "test_heal";
        heal.name = "治疗测试";
        heal.cost = 0;
        heal.description = "恢复 10 点生命。";
        heal.effects = {{CardEffectType::Heal, 10, CardTarget::Self, {}}};
        heal.upgradedEffects = heal.effects;

        CombatSystem combat;
        combat.startBattle(40, 14, fiveCopies(heal));
        assert(combat.playCard(0));
        assert(combat.getPlayer().getCurrentHealth() == 50);
    }

    {
        CombatSystem combat;
        combat.startBattle(80, 15, fiveCopies(CardDatabase::createById("ghostly_armor")));
        assert(combat.playCard(0));
        assert(pileContains(combat.getDeck().getDiscardPile(), "ghostly_armor"));
        assert(combat.getDeck().getExhaustPile().empty());
    }

    {
        std::vector<Card> cards = {
            CardDatabase::createById("exhume"),
            CardDatabase::createById("seeing_red"),
            CardDatabase::createStrike(),
            CardDatabase::createDefend(),
            CardDatabase::createDefend()
        };
        CombatSystem combat;
        combat.startBattle(80, 16, cards);
        assert(combat.playCard(findCard(combat, "seeing_red")));
        assert(combat.playCard(findCard(combat, "exhume")));
        assert(findCard(combat, "seeing_red") >= 0);
    }

    {
        std::vector<Card> cards = {
            CardDatabase::createById("seeing_red"),
            CardDatabase::createDefend(),
            CardDatabase::createDefend(),
            CardDatabase::createDefend(),
            CardDatabase::createDefend()
        };
        CombatSystem combat;
        combat.startBattle(80, 160, cards,
                           {"测试敌人", 100, 0, "generic"}, 0, 0);
        assert(combat.playCard(findCard(combat, "seeing_red")));
        assert(combat.getPlayer().getCurrentEnergy() > combat.getPlayer().getMaxEnergy());
        combat.endPlayerTurn();
        assert(combat.getPlayer().getCurrentEnergy() == combat.getPlayer().getMaxEnergy());
    }

    {
        std::vector<Card> cards = {
            CardDatabase::createById("headbutt"),
            CardDatabase::createStrike(),
            CardDatabase::createDefend(),
            CardDatabase::createDefend(),
            CardDatabase::createDefend()
        };
        CombatSystem combat;
        combat.startBattle(80, 17, cards);
        assert(combat.playCard(findCard(combat, "strike")));
        assert(combat.playCard(findCard(combat, "headbutt")));
        assert(combat.hasPendingDiscardChoice());
        assert(combat.getDiscardChoiceCards().size() == 1);
        assert(combat.chooseDiscardCard(0));
        assert(!combat.getDeck().getDrawPile().empty());
        assert(combat.getDeck().getDrawPile().back().id == "strike");
    }

    {
        // Choose the second discarded instance rather than auto-taking the last.
        CombatSystem combat;
        combat.startBattle(80,42,{CardDatabase::createDefend(),CardDatabase::createStrike(),
            CardDatabase::createById("headbutt"),CardDatabase::createById("pommel_strike")},
            {"测试",200,0,"generic"},0,0,10);
        assert(combat.playCard(findCard(combat,"defend")));
        assert(combat.playCard(findCard(combat,"strike")));
        assert(combat.playCard(findCard(combat,"headbutt")));
        assert(combat.getDiscardChoiceCards().size()==2);
        assert(!combat.chooseDiscardCard(99));
        const int energy = combat.getPlayer().getCurrentEnergy();
        combat.endPlayerTurn();
        assert(combat.getPlayer().getCurrentEnergy()==energy);
        assert(!combat.playCard(findCard(combat,"pommel_strike")));
        assert(combat.chooseDiscardCard(0));
        assert(!combat.hasPendingDiscardChoice());
        assert(combat.getDeck().getDrawPile().back().id=="defend");
        assert(combat.playCard(findCard(combat,"pommel_strike")));
        assert(combat.getHandCards().back().id=="defend");
        combat.startBattle(80,42,{CardDatabase::createById("headbutt")});
        assert(combat.playCard(0));
        assert(!combat.hasPendingDiscardChoice());
    }

    {
        // Double Tap queues two picks; same-name instances remain selectable.
        CombatSystem combat;
        combat.startBattle(80,42,{CardDatabase::createById("double_tap"),CardDatabase::createStrike(),
            CardDatabase::createStrike(),CardDatabase::createById("headbutt")},
            {"测试",200,0,"generic"},0,0,10);
        assert(combat.playCard(findCard(combat,"strike")));
        assert(combat.playCard(findCard(combat,"strike")));
        assert(combat.playCard(findCard(combat,"double_tap")));
        assert(combat.playCard(findCard(combat,"headbutt")));
        assert(combat.chooseDiscardCard(0));
        assert(combat.hasPendingDiscardChoice());
        assert(combat.reviveFromLastSafeSnapshot());
        assert(combat.hasPendingDiscardChoice());
        assert(combat.chooseDiscardCard(0));
        assert(!combat.hasPendingDiscardChoice());
        assert(combat.getDeck().getDrawPile().size()==2);
        assert(combat.getDeck().getDrawPile().back().id=="strike");
    }

    {
        CombatSystem combat;
        combat.startBattle(80,42,{CardDatabase::createDefend(),CardDatabase::createById("headbutt")},
            {"测试",5,0,"generic"},0,0);
        assert(combat.playCard(findCard(combat,"defend")));
        assert(combat.playCard(findCard(combat,"headbutt")));
        assert(combat.getResult()==BattleResult::Victory);
        assert(!combat.hasPendingDiscardChoice());
        combat.startBattle(80,42,{CardDatabase::createById("headbutt")});
        assert(!combat.hasPendingDiscardChoice());
    }

    {
        std::vector<Card> cards = {
            CardDatabase::createById("armaments"),
            CardDatabase::createStrike(),
            CardDatabase::createStrike(),
            CardDatabase::createStrike(),
            CardDatabase::createStrike()
        };
        CombatSystem combat;
        combat.startBattle(80, 18, cards);
        assert(combat.playCard(findCard(combat, "armaments")));
        const int strikeIndex = findCard(combat, "strike");
        assert(strikeIndex >= 0);
        assert(combat.getHandCards()[static_cast<std::size_t>(strikeIndex)].upgraded);
        assert(combat.getHandCards()[static_cast<std::size_t>(strikeIndex)].damage == 9);
    }

    {
        Card upgradedArmaments = CardDatabase::createById("armaments");
        assert(upgradedArmaments.upgrade());
        std::vector<Card> cards = {
            upgradedArmaments,
            CardDatabase::createStrike(),
            CardDatabase::createStrike(),
            CardDatabase::createStrike(),
            CardDatabase::createStrike()
        };
        CombatSystem combat;
        combat.startBattle(80, 19, cards);
        assert(combat.playCard(findCard(combat, "armaments")));
        int upgradedStrikes = 0;
        for (const Card& card : combat.getHandCards())
        {
            if (card.id == "strike" && card.upgraded)
            {
                ++upgradedStrikes;
            }
        }
        assert(upgradedStrikes == 4);
    }

    {
        CombatSystem combat;
        combat.startBattle(80, 20, fiveCopies(CardDatabase::createById("whirlwind")),
                           {"测试敌人", 100, 0, "generic"}, 0, 0);
        assert(combat.playCard(0));
        assert(combat.getEnemy().getCurrentHealth() == 85);
        assert(combat.getPlayer().getCurrentEnergy() == 0);
    }

    {
        std::vector<Card> cards = {
            CardDatabase::createById("second_wind"),
            CardDatabase::createDefend(),
            CardDatabase::createDefend(),
            CardDatabase::createDefend(),
            CardDatabase::createDefend()
        };
        CombatSystem combat;
        combat.startBattle(80, 21, cards);
        assert(combat.playCard(findCard(combat, "second_wind")));
        assert(combat.getPlayer().getBlock() == 20);
        assert(combat.getDeck().getExhaustPile().size() == 4);
    }

    {
        CombatSystem combat;
        combat.startBattle(80, 22, fiveCopies(CardDatabase::createById("flex")));
        assert(combat.playCard(0));
        assert(combat.getPlayer().getStrength() == 2);
        combat.endPlayerTurn();
        assert(combat.getPlayer().getStrength() == 0);
    }

    {
        CombatSystem combat;
        combat.startBattle(80, 23, fiveCopies(CardDatabase::createById("clash")));
        assert(combat.playCard(0));
        assert(combat.getEnemy().getCurrentHealth() == 26);
    }

    {
        CombatSystem combat;
        combat.startBattle(80, 24, fiveCopies(CardDatabase::createById("metallicize")),
                           {"测试敌人", 40, 6, "generic"}, 0, 0);
        assert(combat.playCard(0));
        combat.endPlayerTurn();
        assert(combat.getPlayer().getCurrentHealth() == 77);
    }

    {
        CombatSystem combat;
        combat.startBattle(80, 25, fiveCopies(CardDatabase::createById("combust")),
                           {"测试敌人", 40, 0, "generic"}, 0, 0);
        assert(combat.playCard(0));
        combat.endPlayerTurn();
        assert(combat.getPlayer().getCurrentHealth() == 79);
        assert(combat.getEnemy().getCurrentHealth() == 35);
    }

    {
        Card burn;
        burn.id = "burn";
        burn.name = "灼伤";
        burn.cost = -2;
        burn.description = "回合结束时失去 2 点生命。消耗。";
        burn.effects = {{CardEffectType::Exhaust, 1, CardTarget::Self, "ethereal"}};
        burn.upgradedEffects = burn.effects;

        CombatSystem combat;
        combat.startBattle(80, 26, fiveCopies(burn),
                           {"测试敌人", 40, 0, "generic"}, 0, 0);
        combat.endPlayerTurn();
        assert(combat.getPlayer().getCurrentHealth() == 70);
        assert(combat.getDeck().getExhaustPile().size() == 5);
    }

    {
        std::vector<Card> cards = {
            CardDatabase::createById("rage"),
            CardDatabase::createStrike(),
            CardDatabase::createStrike(),
            CardDatabase::createStrike(),
            CardDatabase::createStrike()
        };
        CombatSystem combat;
        combat.startBattle(80, 27, cards,
                           {"测试敌人", 100, 0, "generic"}, 0, 0);
        assert(combat.playCard(findCard(combat, "rage")));
        assert(combat.playCard(findCard(combat, "strike")));
        combat.endPlayerTurn();
        assert(combat.playCard(findCard(combat, "strike")));
        assert(combat.getPlayer().getBlock() == 0);
    }

    {
        std::vector<Card> cards = {
            CardDatabase::createById("double_tap"),
            CardDatabase::createStrike(),
            CardDatabase::createStrike(),
            CardDatabase::createStrike(),
            CardDatabase::createStrike()
        };
        CombatSystem combat;
        combat.startBattle(80, 28, cards,
                           {"测试敌人", 100, 0, "generic"}, 0, 0);
        assert(combat.playCard(findCard(combat, "double_tap")));
        assert(combat.playCard(findCard(combat, "strike")));
        assert(combat.getEnemy().getCurrentHealth() == 88);
        combat.endPlayerTurn();
        assert(combat.playCard(findCard(combat, "strike")));
        assert(combat.getEnemy().getCurrentHealth() == 82);
    }

    {
        CombatSystem combat;
        combat.startBattle(80, 29, CardDatabase::createStarterDeck(),
                           {"黑暗奥特曼 贝利亚", 150, 35, "belial"}, 0, 0);
        assert(combat.getEnemy().getId() == "belial");
        assert(combat.getEnemy().getMaxHealth() == 150);
        assert(combat.getEnemy().getIntent().name == "终极战斗仪·守");
    }

    {
        Enemy belial("belial", "黑暗奥特曼 贝利亚", 150, 30);
        belial.takeDamage(75);
        assert(belial.getDarkCharge() == 3);
        assert(belial.getStrength() == 2);
        assert(belial.getBlock() == 15);
        belial.advanceIntent();
        assert(belial.getIntent().name == "帝斯修姆光线·极");
    }

    {
        CombatSystem combat;
        combat.startBattle(20, 31, CardDatabase::createStarterDeck(),
                           {"黑暗奥特曼 贝利亚", 150, 35, "belial"}, 0, 0);
        assert(combat.reviveFromLastSafeSnapshot(10, 10, true));
        assert(combat.getPlayer().getCurrentHealth() == combat.getPlayer().getMaxHealth());
        assert(combat.getPlayer().getStrength() == 10);
        assert(combat.getPlayer().getDexterity() == 10);
    }

    {
        CombatSystem combat;
        for (int battle = 0; battle < 2; ++battle)
        {
            // The combined starting strength includes both relic and event bonuses.
            combat.startBattle(80, 42 + battle, fiveCopies(CardDatabase::createStrike()),
                               {"测试敌人", 100, 8, "generic"}, 0, 2 + 1, 0, 0, 80, 1);
            assert(combat.getPlayer().getStrength() == 3);
            assert(combat.getEnemy().getWeak() == 1);
            assert(combat.getEnemyIntentDamage() == 6);
            assert(combat.reviveFromLastSafeSnapshot());
            assert(combat.getPlayer().getStrength() == 3);
            assert(combat.getEnemy().getWeak() == 1);
            assert(combat.playCard(0));
            assert(combat.getEnemy().getCurrentHealth() == 91);
            combat.endPlayerTurn();
            assert(combat.getPlayer().getCurrentHealth() == 74);
            assert(combat.getEnemy().getWeak() == 0);
            assert(combat.getPlayer().getStrength() == 3);
            assert(combat.getEnemyIntentDamage() == 8);
        }
        combat.startBattle();
        assert(combat.getPlayer().getStrength() == 0);
        assert(combat.getEnemy().getWeak() == 0);
    }

    std::cout << "战斗测试通过。\n";
    return 0;
}
