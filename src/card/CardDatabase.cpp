#include "card/CardDatabase.hpp"

#include <stdexcept>
#include <utility>

namespace
{
CardEffect makeEffect(CardEffectType type, int value,
                      CardTarget target = CardTarget::Self,
                      std::string parameter = {})
{
    return {type, value, target, std::move(parameter)};
}

Card makeCard(std::string id, std::string name, CardType type, CardRarity rarity,
              int cost, std::string description, std::vector<CardEffect> effects,
              std::string upgradedDescription = {},
              std::vector<CardEffect> upgradedEffects = {}, int upgradedCost = -1)
{
    Card card;
    card.id = std::move(id);
    card.name = std::move(name);
    card.type = type;
    card.rarity = rarity;
    card.cost = cost;
    card.description = std::move(description);
    card.effects = std::move(effects);
    card.upgradedCost = upgradedCost < 0 ? cost : upgradedCost;
    card.upgradedDescription = upgradedDescription.empty() ? card.description : std::move(upgradedDescription);
    card.upgradedEffects = upgradedEffects.empty() ? card.effects : std::move(upgradedEffects);

    for (const CardEffect& cardEffect : card.effects)
    {
        if (cardEffect.type == CardEffectType::Damage && card.damage == 0)
        {
            card.damage = cardEffect.value;
        }
        if (cardEffect.type == CardEffectType::Block && card.block == 0)
        {
            card.block = cardEffect.value;
        }
    }
    return card;
}

Card attack(std::string id, std::string name, CardRarity rarity, int cost,
            int damage, std::string description, int upgradedDamage = -1,
            int upgradedCost = -1)
{
    const int improvedDamage = upgradedDamage < 0 ? damage : upgradedDamage;
    return makeCard(std::move(id), std::move(name), CardType::Attack, rarity, cost,
        std::move(description), {makeEffect(CardEffectType::Damage, damage, CardTarget::Enemy)},
        {}, {makeEffect(CardEffectType::Damage, improvedDamage, CardTarget::Enemy)}, upgradedCost);
}

Card skill(std::string id, std::string name, CardRarity rarity, int cost,
           int block, std::string description, int upgradedBlock = -1,
           int upgradedCost = -1)
{
    const int improvedBlock = upgradedBlock < 0 ? block : upgradedBlock;
    return makeCard(std::move(id), std::move(name), CardType::Skill, rarity, cost,
        std::move(description), {makeEffect(CardEffectType::Block, block)},
        {}, {makeEffect(CardEffectType::Block, improvedBlock)}, upgradedCost);
}
}

Card CardDatabase::createStrike()
{
    return attack("strike", "打击", CardRarity::Starter, 1, 6, "造成 6 点伤害。", 9);
}

Card CardDatabase::createDefend()
{
    return skill("defend", "防御", CardRarity::Starter, 1, 5, "获得 5 点格挡。", 8);
}

Card CardDatabase::createBash()
{
    return makeCard("bash", "痛击", CardType::Attack, CardRarity::Starter, 2,
        "造成 8 点伤害，施加 2 层易伤。",
        {makeEffect(CardEffectType::Damage, 8, CardTarget::Enemy),
         makeEffect(CardEffectType::ApplyVulnerable, 2, CardTarget::Enemy)},
        "造成 10 点伤害，施加 3 层易伤。",
        {makeEffect(CardEffectType::Damage, 10, CardTarget::Enemy),
         makeEffect(CardEffectType::ApplyVulnerable, 3, CardTarget::Enemy)});
}

std::vector<Card> CardDatabase::createStarterDeck()
{
    std::vector<Card> deck;
    deck.reserve(10);
    for (int index = 0; index < 5; ++index) deck.push_back(createStrike());
    for (int index = 0; index < 4; ++index) deck.push_back(createDefend());
    deck.push_back(createBash());
    return deck;
}

std::vector<Card> CardDatabase::createIroncladCardPool()
{
    return {
        createStrike(), createDefend(), createBash(),
        attack("anger", "愤怒", CardRarity::Common, 0, 6, "造成 6 点伤害。将一张愤怒加入弃牌堆。", 8),
        makeCard("armaments", "武装", CardType::Skill, CardRarity::Common, 1,
            "获得 5 点格挡。升级手牌中的一张牌。",
            {makeEffect(CardEffectType::Block, 5), makeEffect(CardEffectType::UpgradeCard, 1)},
            "获得 5 点格挡。升级手牌中的所有牌。",
            {makeEffect(CardEffectType::Block, 5), makeEffect(CardEffectType::UpgradeCard, -1)}),
        makeCard("body_slam", "全身撞击", CardType::Attack, CardRarity::Common, 1,
            "造成等同于格挡值的伤害。", {makeEffect(CardEffectType::Damage, 0, CardTarget::Enemy, "player_block")},
            "造成等同于格挡值的伤害。", {makeEffect(CardEffectType::Damage, 0, CardTarget::Enemy, "player_block")}, 0),
        attack("clash", "碰撞", CardRarity::Common, 0, 14, "手牌中全是攻击牌时才能打出。造成 14 点伤害。", 18),
        makeCard("cleave", "顺劈斩", CardType::Attack, CardRarity::Common, 1,
            "对所有敌人造成 8 点伤害。", {makeEffect(CardEffectType::MultiDamage, 8, CardTarget::AllEnemies)},
            "对所有敌人造成 11 点伤害。", {makeEffect(CardEffectType::MultiDamage, 11, CardTarget::AllEnemies)}),
        makeCard("clothesline", "铁斩波", CardType::Attack, CardRarity::Common, 2,
            "造成 12 点伤害，施加 2 层虚弱。",
            {makeEffect(CardEffectType::Damage, 12, CardTarget::Enemy), makeEffect(CardEffectType::ApplyWeak, 2, CardTarget::Enemy)},
            "造成 14 点伤害，施加 3 层虚弱。",
            {makeEffect(CardEffectType::Damage, 14, CardTarget::Enemy), makeEffect(CardEffectType::ApplyWeak, 3, CardTarget::Enemy)}),
        makeCard("flex", "肌肉强化", CardType::Skill, CardRarity::Common, 0,
            "获得 2 点力量，回合结束时失去 2 点力量。",
            {makeEffect(CardEffectType::ApplyStrength, 2), makeEffect(CardEffectType::ApplyStrength, -2, CardTarget::Self, "end_turn")},
            "获得 4 点力量，回合结束时失去 4 点力量。",
            {makeEffect(CardEffectType::ApplyStrength, 4), makeEffect(CardEffectType::ApplyStrength, -4, CardTarget::Self, "end_turn")}),
        makeCard("headbutt", "头槌", CardType::Attack, CardRarity::Common, 1,
            "造成 9 点伤害。将弃牌堆的一张牌放到抽牌堆顶。",
            {makeEffect(CardEffectType::Damage, 9, CardTarget::Enemy), makeEffect(CardEffectType::Retain, 1, CardTarget::Self, "discard_to_top")},
            "造成 12 点伤害。将弃牌堆的一张牌放到抽牌堆顶。",
            {makeEffect(CardEffectType::Damage, 12, CardTarget::Enemy), makeEffect(CardEffectType::Retain, 1, CardTarget::Self, "discard_to_top")}),
        makeCard("heavy_blade", "重刃", CardType::Attack, CardRarity::Common, 2,
            "造成 14 点伤害。力量使伤害额外增加。", {makeEffect(CardEffectType::Damage, 14, CardTarget::Enemy, "strength_multiplier_3")},
            "造成 14 点伤害。力量使伤害额外增加。", {makeEffect(CardEffectType::Damage, 14, CardTarget::Enemy, "strength_multiplier_5")}),
        makeCard("iron_wave", "铁波", CardType::Attack, CardRarity::Common, 1,
            "造成 5 点伤害。获得 5 点格挡。", {makeEffect(CardEffectType::Damage, 5, CardTarget::Enemy), makeEffect(CardEffectType::Block, 5)},
            "造成 7 点伤害。获得 7 点格挡。", {makeEffect(CardEffectType::Damage, 7, CardTarget::Enemy), makeEffect(CardEffectType::Block, 7)}),
        makeCard("perfected_strike", "完美打击", CardType::Attack, CardRarity::Common, 2,
            "造成 6 点伤害，每有一张名称含打击的牌，伤害增加 2。", {makeEffect(CardEffectType::Damage, 6, CardTarget::Enemy, "strike_count")},
            "造成 9 点伤害，每有一张名称含打击的牌，伤害增加 3。", {makeEffect(CardEffectType::Damage, 9, CardTarget::Enemy, "strike_count_upgraded")}),
        makeCard("pommel_strike", "剑柄打击", CardType::Attack, CardRarity::Common, 1,
            "造成 9 点伤害。抽 1 张牌。", {makeEffect(CardEffectType::Damage, 9, CardTarget::Enemy), makeEffect(CardEffectType::Draw, 1)},
            "造成 10 点伤害。抽 2 张牌。", {makeEffect(CardEffectType::Damage, 10, CardTarget::Enemy), makeEffect(CardEffectType::Draw, 2)}),
        makeCard("shrug_it_off", "耸肩无视", CardType::Skill, CardRarity::Common, 1,
            "获得 8 点格挡。抽 1 张牌。", {makeEffect(CardEffectType::Block, 8), makeEffect(CardEffectType::Draw, 1)},
            "获得 11 点格挡。抽 1 张牌。", {makeEffect(CardEffectType::Block, 11), makeEffect(CardEffectType::Draw, 1)}),
        makeCard("sword_boomerang", "剑舞", CardType::Attack, CardRarity::Common, 1,
            "随机对敌人造成 3 次 3 点伤害。", {makeEffect(CardEffectType::MultiDamage, 3, CardTarget::RandomEnemy, "hits_3")},
            "随机对敌人造成 4 次 3 点伤害。", {makeEffect(CardEffectType::MultiDamage, 3, CardTarget::RandomEnemy, "hits_4")}),
        makeCard("thunderclap", "雷霆打击", CardType::Attack, CardRarity::Common, 1,
            "对所有敌人造成 4 点伤害并施加 1 层易伤。", {makeEffect(CardEffectType::MultiDamage, 4, CardTarget::AllEnemies), makeEffect(CardEffectType::ApplyVulnerable, 1, CardTarget::AllEnemies)},
            "对所有敌人造成 7 点伤害并施加 1 层易伤。", {makeEffect(CardEffectType::MultiDamage, 7, CardTarget::AllEnemies), makeEffect(CardEffectType::ApplyVulnerable, 1, CardTarget::AllEnemies)}),
        makeCard("true_grit", "坚毅", CardType::Skill, CardRarity::Common, 1,
            "获得 7 点格挡。随机消耗一张手牌。", {makeEffect(CardEffectType::Block, 7), makeEffect(CardEffectType::Exhaust, 1, CardTarget::Self, "random_hand")},
            "获得 9 点格挡。选择一张手牌并消耗。", {makeEffect(CardEffectType::Block, 9), makeEffect(CardEffectType::Exhaust, 1, CardTarget::Self, "choose_hand")}),
        makeCard("twin_strike", "双重打击", CardType::Attack, CardRarity::Common, 1,
            "造成 5 点伤害两次。", {makeEffect(CardEffectType::MultiDamage, 5, CardTarget::Enemy, "hits_2")},
            "造成 7 点伤害两次。", {makeEffect(CardEffectType::MultiDamage, 7, CardTarget::Enemy, "hits_2")}),
        makeCard("warcry", "战吼", CardType::Skill, CardRarity::Common, 0,
            "抽 1 张牌。将一张手牌放到抽牌堆顶。消耗。", {makeEffect(CardEffectType::Draw, 1), makeEffect(CardEffectType::Retain, 1, CardTarget::Self, "hand_to_top"), makeEffect(CardEffectType::Exhaust, 1)},
            "抽 2 张牌。将一张手牌放到抽牌堆顶。消耗。", {makeEffect(CardEffectType::Draw, 2), makeEffect(CardEffectType::Retain, 1, CardTarget::Self, "hand_to_top"), makeEffect(CardEffectType::Exhaust, 1)}),
        makeCard("havoc", "破灭", CardType::Skill, CardRarity::Common, 1,
            "打出抽牌堆顶的牌并将其消耗。", {makeEffect(CardEffectType::PlayTopCard, 1, CardTarget::Self, "exhaust")},
            "打出抽牌堆顶的牌并将其消耗。", {makeEffect(CardEffectType::PlayTopCard, 1, CardTarget::Self, "exhaust")}, 0),
        makeCard("wild_strike", "野蛮冲撞", CardType::Attack, CardRarity::Common, 1,
            "造成 12 点伤害。将一张伤口加入弃牌堆。", {makeEffect(CardEffectType::Damage, 12, CardTarget::Enemy), makeEffect(CardEffectType::AddCard, 1, CardTarget::Self, "wound")},
            "造成 17 点伤害。将一张伤口加入弃牌堆。", {makeEffect(CardEffectType::Damage, 17, CardTarget::Enemy), makeEffect(CardEffectType::AddCard, 1, CardTarget::Self, "wound")}),
        makeCard("battle_trance", "战斗专注", CardType::Skill, CardRarity::Uncommon, 0,
            "抽 3 张牌。本回合不能再抽牌。", {makeEffect(CardEffectType::Draw, 3), makeEffect(CardEffectType::EndTurn, 1, CardTarget::Self, "no_draw")},
            "抽 4 张牌。本回合不能再抽牌。", {makeEffect(CardEffectType::Draw, 4), makeEffect(CardEffectType::EndTurn, 1, CardTarget::Self, "no_draw")}),
        makeCard("blood_for_blood", "以血还血", CardType::Attack, CardRarity::Uncommon, 4,
            "造成 18 点伤害。本场战斗中每失去一次生命，费用降低 1。", {makeEffect(CardEffectType::Damage, 18, CardTarget::Enemy, "cost_minus_on_damage")},
            "造成 22 点伤害。本场战斗中每失去一次生命，费用降低 1。", {makeEffect(CardEffectType::Damage, 22, CardTarget::Enemy, "cost_minus_on_damage")}, 3),
        makeCard("burning_pact", "燃烧契约", CardType::Skill, CardRarity::Uncommon, 1,
            "消耗一张手牌。抽 2 张牌。", {makeEffect(CardEffectType::Exhaust, 1, CardTarget::Self, "choose_hand"), makeEffect(CardEffectType::Draw, 2)},
            "消耗一张手牌。抽 3 张牌。", {makeEffect(CardEffectType::Exhaust, 1, CardTarget::Self, "choose_hand"), makeEffect(CardEffectType::Draw, 3)}),
        makeCard("bloodletting", "放血", CardType::Skill, CardRarity::Uncommon, 0,
            "失去 3 点生命。获得 2 点能量。", {makeEffect(CardEffectType::LoseHealth, 3), makeEffect(CardEffectType::GainEnergy, 2)},
            "失去 3 点生命。获得 3 点能量。", {makeEffect(CardEffectType::LoseHealth, 3), makeEffect(CardEffectType::GainEnergy, 3)}),
        makeCard("carnage", "大切割", CardType::Attack, CardRarity::Uncommon, 2,
            "造成 20 点伤害。虚无。", {makeEffect(CardEffectType::Damage, 20, CardTarget::Enemy), makeEffect(CardEffectType::Exhaust, 1, CardTarget::Self, "ethereal")},
            "造成 28 点伤害。虚无。", {makeEffect(CardEffectType::Damage, 28, CardTarget::Enemy), makeEffect(CardEffectType::Exhaust, 1, CardTarget::Self, "ethereal")}),
        makeCard("combust", "燃烧", CardType::Power, CardRarity::Uncommon, 1,
            "回合结束时失去 1 点生命，并对所有敌人造成 5 点伤害。", {makeEffect(CardEffectType::LoseHealth, 1, CardTarget::Self, "end_turn"), makeEffect(CardEffectType::MultiDamage, 5, CardTarget::AllEnemies, "end_turn")},
            "回合结束时失去 1 点生命，并对所有敌人造成 7 点伤害。", {makeEffect(CardEffectType::LoseHealth, 1, CardTarget::Self, "end_turn"), makeEffect(CardEffectType::MultiDamage, 7, CardTarget::AllEnemies, "end_turn")}),
        makeCard("disarm", "缴械", CardType::Skill, CardRarity::Uncommon, 1,
            "使敌人失去 2 点力量。消耗。", {makeEffect(CardEffectType::ApplyStrength, -2, CardTarget::Enemy), makeEffect(CardEffectType::Exhaust, 1)},
            "使敌人失去 3 点力量。消耗。", {makeEffect(CardEffectType::ApplyStrength, -3, CardTarget::Enemy), makeEffect(CardEffectType::Exhaust, 1)}),
        makeCard("dropkick", "飞身踢", CardType::Attack, CardRarity::Uncommon, 1,
            "造成 5 点伤害。若敌人处于易伤，抽 1 张牌并获得 1 点能量。", {makeEffect(CardEffectType::Damage, 5, CardTarget::Enemy), makeEffect(CardEffectType::Draw, 1, CardTarget::Self, "if_vulnerable"), makeEffect(CardEffectType::GainEnergy, 1, CardTarget::Self, "if_vulnerable")},
            "造成 8 点伤害。若敌人处于易伤，抽 1 张牌并获得 1 点能量。", {makeEffect(CardEffectType::Damage, 8, CardTarget::Enemy), makeEffect(CardEffectType::Draw, 1, CardTarget::Self, "if_vulnerable"), makeEffect(CardEffectType::GainEnergy, 1, CardTarget::Self, "if_vulnerable")}),
        makeCard("dark_embrace", "黑暗之拥", CardType::Power, CardRarity::Uncommon, 2,
            "每当一张牌被消耗时，抽 1 张牌。", {makeEffect(CardEffectType::Draw, 1, CardTarget::Self, "on_exhaust")},
            "每当一张牌被消耗时，抽 1 张牌。", {makeEffect(CardEffectType::Draw, 1, CardTarget::Self, "on_exhaust")}, 1),
        makeCard("dual_wield", "双持", CardType::Skill, CardRarity::Uncommon, 1,
            "选择一张攻击牌或能力牌，将其的一张复制加入手牌。", {makeEffect(CardEffectType::AddCard, 1, CardTarget::Self, "copy_attack_or_power")},
            "选择一张攻击牌或能力牌，将其的两张复制加入手牌。", {makeEffect(CardEffectType::AddCard, 2, CardTarget::Self, "copy_attack_or_power")}),
        makeCard("entrench", "巩固", CardType::Skill, CardRarity::Uncommon, 2,
            "格挡翻倍。", {makeEffect(CardEffectType::Block, 2, CardTarget::Self, "double_block")},
            "格挡翻倍。", {makeEffect(CardEffectType::Block, 2, CardTarget::Self, "double_block")}, 1),
        makeCard("evolve", "进化", CardType::Power, CardRarity::Uncommon, 1,
            "每当你抽到状态牌时，抽 1 张牌。", {makeEffect(CardEffectType::Draw, 1, CardTarget::Self, "on_status_draw")},
            "每当你抽到状态牌时，抽 2 张牌。", {makeEffect(CardEffectType::Draw, 2, CardTarget::Self, "on_status_draw")}),
        makeCard("feel_no_pain", "无惧疼痛", CardType::Power, CardRarity::Uncommon, 1,
            "每当一张牌被消耗时，获得 3 点格挡。", {makeEffect(CardEffectType::Block, 3, CardTarget::Self, "on_exhaust")},
            "每当一张牌被消耗时，获得 4 点格挡。", {makeEffect(CardEffectType::Block, 4, CardTarget::Self, "on_exhaust")}),
        makeCard("fire_breathing", "火焰吐息", CardType::Power, CardRarity::Uncommon, 1,
            "每当你抽到状态牌或诅咒牌时，对所有敌人造成 6 点伤害。", {makeEffect(CardEffectType::MultiDamage, 6, CardTarget::AllEnemies, "on_status_draw")},
            "每当你抽到状态牌或诅咒牌时，对所有敌人造成 10 点伤害。", {makeEffect(CardEffectType::MultiDamage, 10, CardTarget::AllEnemies, "on_status_draw")}),
        makeCard("flame_barrier", "火焰屏障", CardType::Skill, CardRarity::Uncommon, 2,
            "获得 12 点格挡。受到攻击时，对攻击者造成 4 点伤害。", {makeEffect(CardEffectType::Block, 12), makeEffect(CardEffectType::AddCard, 4, CardTarget::Self, "flame_barrier")},
            "获得 16 点格挡。受到攻击时，对攻击者造成 6 点伤害。", {makeEffect(CardEffectType::Block, 16), makeEffect(CardEffectType::AddCard, 6, CardTarget::Self, "flame_barrier")}),
        makeCard("ghostly_armor", "幽灵铠甲", CardType::Skill, CardRarity::Uncommon, 1,
            "获得 10 点格挡。虚无。", {makeEffect(CardEffectType::Block, 10), makeEffect(CardEffectType::Exhaust, 1, CardTarget::Self, "ethereal")},
            "获得 13 点格挡。虚无。", {makeEffect(CardEffectType::Block, 13), makeEffect(CardEffectType::Exhaust, 1, CardTarget::Self, "ethereal")}),
        makeCard("hemokinesis", "御血术", CardType::Attack, CardRarity::Uncommon, 1,
            "失去 2 点生命。造成 15 点伤害。", {makeEffect(CardEffectType::LoseHealth, 2), makeEffect(CardEffectType::Damage, 15, CardTarget::Enemy)},
            "失去 2 点生命。造成 20 点伤害。", {makeEffect(CardEffectType::LoseHealth, 2), makeEffect(CardEffectType::Damage, 20, CardTarget::Enemy)}),
        makeCard("infernal_blade", "地狱之刃", CardType::Skill, CardRarity::Uncommon, 1,
            "将一张随机攻击牌加入手牌，其费用为 0。消耗。", {makeEffect(CardEffectType::AddCard, 1, CardTarget::Self, "random_attack_cost_zero"), makeEffect(CardEffectType::Exhaust, 1)},
            "将一张随机攻击牌加入手牌，其费用为 0。消耗。", {makeEffect(CardEffectType::AddCard, 1, CardTarget::Self, "random_attack_cost_zero"), makeEffect(CardEffectType::Exhaust, 1)}, 0),
        makeCard("inflame", "燃烧", CardType::Power, CardRarity::Uncommon, 1,
            "获得 2 点力量。", {makeEffect(CardEffectType::ApplyStrength, 2)},
            "获得 3 点力量。", {makeEffect(CardEffectType::ApplyStrength, 3)}),
        makeCard("metallicize", "金属化", CardType::Power, CardRarity::Uncommon, 1,
            "每回合结束时获得 3 点格挡。", {makeEffect(CardEffectType::Block, 3, CardTarget::Self, "end_turn")},
            "每回合结束时获得 4 点格挡。", {makeEffect(CardEffectType::Block, 4, CardTarget::Self, "end_turn")}),
        makeCard("intimidate", "威吓", CardType::Skill, CardRarity::Uncommon, 0,
            "对所有敌人施加 1 层虚弱。消耗。", {makeEffect(CardEffectType::ApplyWeak, 1, CardTarget::AllEnemies), makeEffect(CardEffectType::Exhaust, 1)},
            "对所有敌人施加 2 层虚弱。消耗。", {makeEffect(CardEffectType::ApplyWeak, 2, CardTarget::AllEnemies), makeEffect(CardEffectType::Exhaust, 1)}),
        makeCard("power_through", "势不可挡", CardType::Skill, CardRarity::Uncommon, 1,
            "获得 15 点格挡。将 2 张伤口加入手牌。", {makeEffect(CardEffectType::Block, 15), makeEffect(CardEffectType::AddCard, 2, CardTarget::Self, "wound_to_hand")},
            "获得 20 点格挡。将 2 张伤口加入手牌。", {makeEffect(CardEffectType::Block, 20), makeEffect(CardEffectType::AddCard, 2, CardTarget::Self, "wound_to_hand")}),
        makeCard("pummel", "重拳", CardType::Attack, CardRarity::Uncommon, 1,
            "造成 2 点伤害 4 次。消耗。", {makeEffect(CardEffectType::MultiDamage, 2, CardTarget::Enemy, "hits_4"), makeEffect(CardEffectType::Exhaust, 1)},
            "造成 2 点伤害 5 次。消耗。", {makeEffect(CardEffectType::MultiDamage, 2, CardTarget::Enemy, "hits_5"), makeEffect(CardEffectType::Exhaust, 1)}),
        makeCard("rage", "狂怒", CardType::Skill, CardRarity::Uncommon, 0,
            "本回合每打出一张攻击牌，获得 3 点格挡。", {makeEffect(CardEffectType::Block, 3, CardTarget::Self, "on_attack_played")},
            "本回合每打出一张攻击牌，获得 5 点格挡。", {makeEffect(CardEffectType::Block, 5, CardTarget::Self, "on_attack_played")}),
        makeCard("rampage", "狂暴", CardType::Attack, CardRarity::Uncommon, 1,
            "造成 8 点伤害。本场战斗中每打出一次，本牌伤害增加 5。", {makeEffect(CardEffectType::Damage, 8, CardTarget::Enemy, "repeated_plus_5")},
            "造成 8 点伤害。本场战斗中每打出一次，本牌伤害增加 5。", {makeEffect(CardEffectType::Damage, 8, CardTarget::Enemy, "repeated_plus_5")}),
        makeCard("reckless_charge", "无谋冲锋", CardType::Attack, CardRarity::Uncommon, 0,
            "造成 7 点伤害。将一张眩晕加入抽牌堆。", {makeEffect(CardEffectType::Damage, 7, CardTarget::Enemy), makeEffect(CardEffectType::AddCard, 1, CardTarget::Self, "dazed_to_draw")},
            "造成 10 点伤害。将一张眩晕加入抽牌堆。", {makeEffect(CardEffectType::Damage, 10, CardTarget::Enemy), makeEffect(CardEffectType::AddCard, 1, CardTarget::Self, "dazed_to_draw")}),
        makeCard("rupture", "撕裂", CardType::Power, CardRarity::Uncommon, 1,
            "每当你因卡牌失去生命时，获得 1 点力量。", {makeEffect(CardEffectType::ApplyStrength, 1, CardTarget::Self, "on_card_lose_health")},
            "每当你因卡牌失去生命时，获得 2 点力量。", {makeEffect(CardEffectType::ApplyStrength, 2, CardTarget::Self, "on_card_lose_health")}),
        makeCard("sentinel", "哨卫", CardType::Skill, CardRarity::Uncommon, 1,
            "获得 5 点格挡。若此牌被消耗，获得 2 点能量。", {makeEffect(CardEffectType::Block, 5), makeEffect(CardEffectType::GainEnergy, 2, CardTarget::Self, "on_exhaust")},
            "获得 8 点格挡。若此牌被消耗，获得 3 点能量。", {makeEffect(CardEffectType::Block, 8), makeEffect(CardEffectType::GainEnergy, 3, CardTarget::Self, "on_exhaust")}),
        makeCard("sever_soul", "断魂斩", CardType::Attack, CardRarity::Uncommon, 2,
            "造成 16 点伤害。消耗手牌中所有非攻击牌。", {makeEffect(CardEffectType::Damage, 16, CardTarget::Enemy), makeEffect(CardEffectType::Exhaust, -1, CardTarget::Self, "all_non_attacks")},
            "造成 22 点伤害。消耗手牌中所有非攻击牌。", {makeEffect(CardEffectType::Damage, 22, CardTarget::Enemy), makeEffect(CardEffectType::Exhaust, -1, CardTarget::Self, "all_non_attacks")}),
        makeCard("searing_blow", "灼热打击", CardType::Attack, CardRarity::Uncommon, 2,
            "造成 12 点伤害。这张牌可以被升级任意次数。", {makeEffect(CardEffectType::Damage, 12, CardTarget::Enemy, "repeatable_upgrade")}),
        makeCard("second_wind", "第二风", CardType::Skill, CardRarity::Uncommon, 1,
            "消耗所有手牌中的非攻击牌，每消耗一张获得 5 点格挡。", {makeEffect(CardEffectType::Block, 5, CardTarget::Self, "per_non_attack_exhausted")},
            "消耗所有手牌中的非攻击牌，每消耗一张获得 8 点格挡。", {makeEffect(CardEffectType::Block, 8, CardTarget::Self, "per_non_attack_exhausted")}),
        makeCard("spot_weakness", "观察弱点", CardType::Skill, CardRarity::Uncommon, 1,
            "若敌人正在攻击，获得 3 点力量。", {makeEffect(CardEffectType::ApplyStrength, 3, CardTarget::Self, "if_enemy_attacking")},
            "若敌人正在攻击，获得 4 点力量。", {makeEffect(CardEffectType::ApplyStrength, 4, CardTarget::Self, "if_enemy_attacking")}),
        makeCard("seeing_red", "盛怒", CardType::Skill, CardRarity::Uncommon, 1,
            "获得 2 点能量。消耗。", {makeEffect(CardEffectType::GainEnergy, 2), makeEffect(CardEffectType::Exhaust, 1)},
            "获得 3 点能量。消耗。", {makeEffect(CardEffectType::GainEnergy, 3), makeEffect(CardEffectType::Exhaust, 1)}, 0),
        makeCard("shockwave", "震荡波", CardType::Skill, CardRarity::Uncommon, 2,
            "对所有敌人施加 3 层虚弱和易伤。消耗。", {makeEffect(CardEffectType::ApplyWeak, 3, CardTarget::AllEnemies), makeEffect(CardEffectType::ApplyVulnerable, 3, CardTarget::AllEnemies), makeEffect(CardEffectType::Exhaust, 1)},
            "对所有敌人施加 5 层虚弱和易伤。消耗。", {makeEffect(CardEffectType::ApplyWeak, 5, CardTarget::AllEnemies), makeEffect(CardEffectType::ApplyVulnerable, 5, CardTarget::AllEnemies), makeEffect(CardEffectType::Exhaust, 1)}),
        makeCard("uppercut", "上勾拳", CardType::Attack, CardRarity::Uncommon, 2,
            "造成 13 点伤害。施加 1 层虚弱和易伤。", {makeEffect(CardEffectType::Damage, 13, CardTarget::Enemy), makeEffect(CardEffectType::ApplyWeak, 1, CardTarget::Enemy), makeEffect(CardEffectType::ApplyVulnerable, 1, CardTarget::Enemy)},
            "造成 17 点伤害。施加 2 层虚弱和易伤。", {makeEffect(CardEffectType::Damage, 17, CardTarget::Enemy), makeEffect(CardEffectType::ApplyWeak, 2, CardTarget::Enemy), makeEffect(CardEffectType::ApplyVulnerable, 2, CardTarget::Enemy)}, 1),
        makeCard("whirlwind", "旋风斩", CardType::Attack, CardRarity::Uncommon, -1,
            "对所有敌人造成 X 次 5 点伤害。", {makeEffect(CardEffectType::MultiDamage, 5, CardTarget::AllEnemies, "x_cost")},
            "对所有敌人造成 X 次 8 点伤害。", {makeEffect(CardEffectType::MultiDamage, 8, CardTarget::AllEnemies, "x_cost")}),
        makeCard("berserk", "狂暴", CardType::Skill, CardRarity::Uncommon, 0,
            "获得 2 层易伤。每回合开始时获得 1 点能量。", {makeEffect(CardEffectType::ApplyVulnerable, 2), makeEffect(CardEffectType::GainEnergy, 1, CardTarget::Self, "start_turn")},
            "获得 1 层易伤。每回合开始时获得 1 点能量。", {makeEffect(CardEffectType::ApplyVulnerable, 1), makeEffect(CardEffectType::GainEnergy, 1, CardTarget::Self, "start_turn")}),
        makeCard("barricade", "壁垒", CardType::Power, CardRarity::Rare, 3,
            "格挡不会在回合开始时消失。", {makeEffect(CardEffectType::Retain, 1, CardTarget::Self, "block")},
            "格挡不会在回合开始时消失。", {makeEffect(CardEffectType::Retain, 1, CardTarget::Self, "block")}, 2),
        attack("bludgeon", "重锤", CardRarity::Rare, 3, 32, "造成 32 点伤害。", 42),
        makeCard("demon_form", "恶魔形态", CardType::Power, CardRarity::Rare, 3,
            "每回合开始时获得 2 点力量。", {makeEffect(CardEffectType::ApplyStrength, 2, CardTarget::Self, "start_turn")},
            "每回合开始时获得 3 点力量。", {makeEffect(CardEffectType::ApplyStrength, 3, CardTarget::Self, "start_turn")}),
        makeCard("corruption", "腐化", CardType::Power, CardRarity::Rare, 3,
            "技能牌费用变为 0，但打出时会消耗。", {makeEffect(CardEffectType::ReduceCost, 0, CardTarget::Self, "skills_cost_zero_exhaust")},
            "技能牌费用变为 0，但打出时会消耗。", {makeEffect(CardEffectType::ReduceCost, 0, CardTarget::Self, "skills_cost_zero_exhaust")}, 2),
        makeCard("brutality", "残暴", CardType::Power, CardRarity::Rare, 0,
            "每回合开始时失去 1 点生命并抽 1 张牌。", {makeEffect(CardEffectType::LoseHealth, 1, CardTarget::Self, "start_turn"), makeEffect(CardEffectType::Draw, 1, CardTarget::Self, "start_turn")}),
        makeCard("double_tap", "双发", CardType::Skill, CardRarity::Rare, 1,
            "本回合你打出的下一张攻击牌会打出两次。消耗。", {makeEffect(CardEffectType::PlayTopCard, 1, CardTarget::Self, "next_attack_twice"), makeEffect(CardEffectType::Exhaust, 1)},
            "本回合你打出的下两张攻击牌会打出两次。消耗。", {makeEffect(CardEffectType::PlayTopCard, 2, CardTarget::Self, "next_two_attacks_twice"), makeEffect(CardEffectType::Exhaust, 1)}),
        makeCard("exhume", "发掘", CardType::Skill, CardRarity::Rare, 1,
            "将一张消耗牌放入手牌。消耗。", {makeEffect(CardEffectType::AddCard, 1, CardTarget::Self, "from_exhaust"), makeEffect(CardEffectType::Exhaust, 1)},
            "将一张消耗牌放入手牌。消耗。", {makeEffect(CardEffectType::AddCard, 1, CardTarget::Self, "from_exhaust"), makeEffect(CardEffectType::Exhaust, 1)}, 0),
        makeCard("feed", "噬咬", CardType::Attack, CardRarity::Rare, 1,
            "造成 10 点伤害。若致死，最大生命值增加 3。消耗。", {makeEffect(CardEffectType::Damage, 10, CardTarget::Enemy, "gain_max_health_on_kill"), makeEffect(CardEffectType::Exhaust, 1)},
            "造成 12 点伤害。若致死，最大生命值增加 4。消耗。", {makeEffect(CardEffectType::Damage, 12, CardTarget::Enemy, "gain_max_health_on_kill"), makeEffect(CardEffectType::Exhaust, 1)}),
        makeCard("fiend_fire", "恶魔之焰", CardType::Attack, CardRarity::Rare, 2,
            "消耗所有手牌。每消耗一张，对随机敌人造成 7 点伤害。", {makeEffect(CardEffectType::Exhaust, -1, CardTarget::Self, "all_hand"), makeEffect(CardEffectType::MultiDamage, 7, CardTarget::RandomEnemy, "per_exhausted_card")},
            "消耗所有手牌。每消耗一张，对随机敌人造成 10 点伤害。", {makeEffect(CardEffectType::Exhaust, -1, CardTarget::Self, "all_hand"), makeEffect(CardEffectType::MultiDamage, 10, CardTarget::RandomEnemy, "per_exhausted_card")}),
        makeCard("immolate", "献祭", CardType::Attack, CardRarity::Rare, 2,
            "对所有敌人造成 21 点伤害。将一张灼伤加入弃牌堆。", {makeEffect(CardEffectType::MultiDamage, 21, CardTarget::AllEnemies), makeEffect(CardEffectType::AddCard, 1, CardTarget::Self, "burn")},
            "对所有敌人造成 28 点伤害。将一张灼伤加入弃牌堆。", {makeEffect(CardEffectType::MultiDamage, 28, CardTarget::AllEnemies), makeEffect(CardEffectType::AddCard, 1, CardTarget::Self, "burn")}),
        makeCard("juggernaut", "铁甲冲击", CardType::Power, CardRarity::Rare, 2,
            "每当你获得格挡时，对随机敌人造成 5 点伤害。", {makeEffect(CardEffectType::Damage, 5, CardTarget::RandomEnemy, "on_gain_block")},
            "每当你获得格挡时，对随机敌人造成 7 点伤害。", {makeEffect(CardEffectType::Damage, 7, CardTarget::RandomEnemy, "on_gain_block")}),
        makeCard("limit_break", "突破极限", CardType::Skill, CardRarity::Rare, 1,
            "力量翻倍。消耗。", {makeEffect(CardEffectType::ApplyStrength, 2, CardTarget::Self, "double"), makeEffect(CardEffectType::Exhaust, 1)},
            "力量翻倍。", {makeEffect(CardEffectType::ApplyStrength, 2, CardTarget::Self, "double")}),
        makeCard("offering", "祭品", CardType::Skill, CardRarity::Rare, 0,
            "失去 6 点生命。获得 2 点能量。抽 3 张牌。消耗。", {makeEffect(CardEffectType::LoseHealth, 6), makeEffect(CardEffectType::GainEnergy, 2), makeEffect(CardEffectType::Draw, 3), makeEffect(CardEffectType::Exhaust, 1)},
            "失去 6 点生命。获得 2 点能量。抽 5 张牌。消耗。", {makeEffect(CardEffectType::LoseHealth, 6), makeEffect(CardEffectType::GainEnergy, 2), makeEffect(CardEffectType::Draw, 5), makeEffect(CardEffectType::Exhaust, 1)}),
        makeCard("reaper", "收割", CardType::Attack, CardRarity::Rare, 2,
            "对所有敌人造成 4 点伤害，恢复未被格挡伤害的生命。消耗。", {makeEffect(CardEffectType::MultiDamage, 4, CardTarget::AllEnemies, "heal_unblocked"), makeEffect(CardEffectType::Exhaust, 1)},
            "对所有敌人造成 5 点伤害，恢复未被格挡伤害的生命。消耗。", {makeEffect(CardEffectType::MultiDamage, 5, CardTarget::AllEnemies, "heal_unblocked"), makeEffect(CardEffectType::Exhaust, 1)})
    };
}

Card CardDatabase::createById(const std::string& id)
{
    for (const Card& card : createIroncladCardPool())
    {
        if (card.id == id)
        {
            return card;
        }
    }
    throw std::invalid_argument("Unknown card id: " + id);
}

Card CardDatabase::createFromInstance(const CardInstance& instance)
{
    Card card = createById(instance.definitionId);
    if (!instance.upgraded)
    {
        return card;
    }

    card.name += "+";
    card.cost = card.upgradedCost;
    card.description = card.upgradedDescription;
    card.effects = card.upgradedEffects;
    card.damage = 0;
    card.block = 0;
    for (const CardEffect& effect : card.effects)
    {
        if ((effect.type == CardEffectType::Damage || effect.type == CardEffectType::MultiDamage) &&
            card.damage == 0)
        {
            card.damage = effect.value;
        }
        if (effect.type == CardEffectType::Block && card.block == 0)
        {
            card.block = effect.value;
        }
    }
    return card;
}
