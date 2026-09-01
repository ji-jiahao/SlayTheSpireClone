#include "combat/Enemy.hpp"

#include <algorithm>
#include <utility>

namespace
{
EnemyIntent attack(const char* name, int damage, const char* description)
{
    EnemyIntent result;
    result.type = EnemyIntentType::Attack;
    result.name = name;
    result.description = description;
    result.damage = damage;
    return result;
}
}

Enemy::Enemy(std::string name, int maxHealth)
    : Enemy("generic", std::move(name), maxHealth, 0)
{
}

Enemy::Enemy(std::string id, std::string name, int maxHealth, std::uint32_t seed)
    : id(std::move(id)), name(std::move(name)), maxHealth(std::max(0, maxHealth)),
      currentHealth(this->maxHealth), block(0), strength(0), weak(0), vulnerable(0),
      ritual(0), metallicize(0), deathVulnerable(0), sleepingTurns(0), patternStep(0),
      repeatedIntentCount(0), previousIntentType(EnemyIntentType::Stunned),
      archetype(archetypeFromId(this->id)), randomEngine(seed)
{
    if (archetype == EnemyArchetype::FungiBeast) deathVulnerable = 2;
    if (archetype == EnemyArchetype::Lagavulin) metallicize = 8;
    chooseInitialIntent();
}

EnemyArchetype Enemy::archetypeFromId(const std::string& enemyId) const
{
    if (enemyId == "cultist") return EnemyArchetype::Cultist;
    if (enemyId == "jaw_worm") return EnemyArchetype::JawWorm;
    if (enemyId == "acid_slime") return EnemyArchetype::AcidSlime;
    if (enemyId == "fungi_beast") return EnemyArchetype::FungiBeast;
    if (enemyId == "lagavulin") return EnemyArchetype::Lagavulin;
    if (enemyId == "slime_boss") return EnemyArchetype::SlimeBoss;
    if (enemyId == "slime_pair") return EnemyArchetype::SlimePair;
    return EnemyArchetype::Generic;
}

void Enemy::chooseInitialIntent()
{
    switch (archetype)
    {
    case EnemyArchetype::Cultist:
        setIntent({EnemyIntentType::Buff, "祈祷", "获得 3 层仪式；之后每回合增加 3 点力量。"});
        break;
    case EnemyArchetype::JawWorm:
        setIntent(attack("咬击", 11, "造成 11 点伤害。"));
        break;
    case EnemyArchetype::AcidSlime:
        chooseAcidSlimeIntent();
        break;
    case EnemyArchetype::FungiBeast:
        chooseFungiBeastIntent();
        break;
    case EnemyArchetype::Lagavulin:
        setIntent({EnemyIntentType::Sleep, "沉睡", "不会行动；回合结束获得 8 点格挡。"});
        break;
    case EnemyArchetype::SlimeBoss:
    {
        EnemyIntent next{EnemyIntentType::Status, "黏液喷射", "将 3 张黏液加入弃牌堆。"};
        next.slimed = 3;
        setIntent(next);
        break;
    }
    default:
        setIntent(attack("攻击", 0, "不会造成伤害。"));
        break;
    }
}

void Enemy::startTurn()
{
    block = 0;
}

void Enemy::endTurn()
{
    weak = std::max(0, weak - 1);
    vulnerable = std::max(0, vulnerable - 1);
    if (archetype == EnemyArchetype::Cultist && intent.name == "祈祷" && ritual == 0)
        ritual = 3;
    if (ritual > 0) strength += ritual;
    if (metallicize > 0) block += metallicize;
}

void Enemy::advanceIntent()
{
    switch (archetype)
    {
    case EnemyArchetype::Cultist:
        if (ritual == 0) ritual = 3;
        setIntent(attack("黑暗打击", 6, "造成 6 点基础伤害，仪式会使其逐回合增强。"));
        break;
    case EnemyArchetype::JawWorm:
        chooseJawWormIntent();
        break;
    case EnemyArchetype::AcidSlime:
        chooseAcidSlimeIntent();
        break;
    case EnemyArchetype::FungiBeast:
        chooseFungiBeastIntent();
        break;
    case EnemyArchetype::Lagavulin:
        if (intent.type == EnemyIntentType::Sleep)
        {
            ++sleepingTurns;
            if (sleepingTurns < 3)
                setIntent({EnemyIntentType::Sleep, "沉睡", "不会行动；回合结束获得 8 点格挡。"});
            else
            {
                metallicize = 0;
                patternStep = 0;
                setIntent(attack("攻击", 18, "造成 18 点伤害。"));
            }
        }
        else if (intent.type == EnemyIntentType::Stunned)
        {
            patternStep = 0;
            setIntent(attack("攻击", 18, "造成 18 点伤害。"));
        }
        else
        {
            ++patternStep;
            if (patternStep % 3 == 2)
            {
                EnemyIntent next{EnemyIntentType::Debuff, "灵魂虹吸",
                                 "使玩家失去 1 点力量和 1 点敏捷。"};
                next.strength = -1;
                next.dexterity = -1;
                setIntent(next);
            }
            else
                setIntent(attack("攻击", 18, "造成 18 点伤害。"));
        }
        break;
    case EnemyArchetype::SlimeBoss:
        patternStep = (patternStep + 1) % 3;
        if (patternStep == 1)
            setIntent({EnemyIntentType::Preparing, "准备", "正在蓄力，本回合不会行动。"});
        else if (patternStep == 2)
            setIntent(attack("重击", 35, "造成 35 点伤害。"));
        else
        {
            EnemyIntent next{EnemyIntentType::Status, "黏液喷射", "将 3 张黏液加入弃牌堆。"};
            next.slimed = 3;
            setIntent(next);
        }
        break;
    case EnemyArchetype::SlimePair:
        chooseSlimePairIntent();
        break;
    default:
        break;
    }
}

void Enemy::chooseJawWormIntent()
{
    const std::string previousName = intent.name;
    const int roll = rollPercent();
    EnemyIntent next;
    if (previousName == "咬击")
    {
        next = roll < 59
                   ? EnemyIntent{EnemyIntentType::DefendBuff, "吼叫", "获得 3 点力量和 6 点格挡。", 0, 1, 6, 3}
                   : EnemyIntent{EnemyIntentType::AttackDefend, "痛击", "造成 7 点伤害并获得 5 点格挡。", 7, 1, 5};
    }
    else if (previousName == "吼叫")
    {
        next = roll < 56
                   ? EnemyIntent{EnemyIntentType::AttackDefend, "痛击", "造成 7 点伤害并获得 5 点格挡。", 7, 1, 5}
                   : attack("咬击", 11, "造成 11 点伤害。");
    }
    else
    {
        const bool repeatedThrash = previousName == "痛击" && repeatedIntentCount >= 2;
        if (repeatedThrash)
            next = roll < 64
                       ? EnemyIntent{EnemyIntentType::DefendBuff, "吼叫", "获得 3 点力量和 6 点格挡。", 0, 1, 6, 3}
                       : attack("咬击", 11, "造成 11 点伤害。");
        else if (roll < 45)
            next = {EnemyIntentType::DefendBuff, "吼叫", "获得 3 点力量和 6 点格挡。", 0, 1, 6, 3};
        else if (roll < 75)
            next = {EnemyIntentType::AttackDefend, "痛击", "造成 7 点伤害并获得 5 点格挡。", 7, 1, 5};
        else
            next = attack("咬击", 11, "造成 11 点伤害。");
    }
    setIntent(next);
}

void Enemy::chooseAcidSlimeIntent()
{
    for (int attempt = 0; attempt < 12; ++attempt)
    {
        const int roll = rollPercent();
        EnemyIntent next;
        if (roll < 30)
        {
            next = {EnemyIntentType::Composite, "腐蚀喷吐", "造成 7 点伤害并将 1 张黏液加入弃牌堆。", 7};
            next.slimed = 1;
        }
        else if (roll < 70)
            next = attack("撞击", 10, "造成 10 点伤害。");
        else
        {
            next = {EnemyIntentType::Debuff, "舔舐", "施加 1 层虚弱。"};
            next.weak = 1;
        }

        const bool tackleTwice = next.name == "撞击" && intent.name == "撞击";
        const bool otherThreeTimes = next.name == intent.name && repeatedIntentCount >= 2;
        if (!tackleTwice && !otherThreeTimes)
        {
            setIntent(next);
            return;
        }
    }
    if (intent.name == "撞击")
    {
        EnemyIntent next{EnemyIntentType::Debuff, "舔舐", "施加 1 层虚弱。"};
        next.weak = 1;
        setIntent(next);
    }
    else
        setIntent(attack("撞击", 10, "造成 10 点伤害。"));
}

void Enemy::chooseFungiBeastIntent()
{
    const bool forceGrow = intent.name == "啃咬" && repeatedIntentCount >= 2;
    const bool forceBite = intent.name == "生长";
    if (forceGrow || (!forceBite && rollPercent() >= 60))
        setIntent({EnemyIntentType::Buff, "生长", "获得 3 点力量。", 0, 1, 0, 3});
    else
        setIntent(attack("啃咬", 6, "造成 6 点伤害。"));
}

void Enemy::chooseSlimePairIntent()
{
    const int roll = rollPercent();
    if (roll < 35)
        setIntent({EnemyIntentType::Composite, "双重撞击", "两只大型史莱姆合计造成 32 点伤害。", 32});
    else if (roll < 65)
    {
        EnemyIntent next{EnemyIntentType::Composite, "黏液喷吐", "造成 11 点伤害并将 4 张黏液加入弃牌堆。", 11};
        next.slimed = 4;
        setIntent(next);
    }
    else
    {
        EnemyIntent next{EnemyIntentType::Debuff, "双重舔舐", "施加 2 层虚弱和 2 层脆弱。"};
        next.weak = 2;
        next.frail = 2;
        setIntent(next);
    }
}

int Enemy::rollPercent()
{
    return std::uniform_int_distribution<int>(0, 99)(randomEngine);
}

void Enemy::setIntent(const EnemyIntent& nextIntent)
{
    if (intent.name == nextIntent.name && !intent.name.empty())
        ++repeatedIntentCount;
    else
        repeatedIntentCount = 1;
    previousIntentType = intent.type;
    intent = nextIntent;
}

void Enemy::takeDamage(int amount)
{
    const int damage = std::max(0, amount);
    const int unblocked = std::max(0, damage - block);
    block = std::max(0, block - damage);
    currentHealth = std::max(0, currentHealth - unblocked);

    if (unblocked > 0 && archetype == EnemyArchetype::Lagavulin && intent.type == EnemyIntentType::Sleep)
    {
        metallicize = 0;
        sleepingTurns = 3;
        setIntent({EnemyIntentType::Stunned, "眩晕", "被提前唤醒，本回合不会行动。"});
    }
    if (currentHealth > 0 && archetype == EnemyArchetype::SlimeBoss && currentHealth * 2 <= maxHealth)
        setIntent({EnemyIntentType::Split, "分裂", "分裂为酸液史莱姆和尖刺史莱姆。"});
}

void Enemy::gainBlock(int amount) { block += std::max(0, amount); }
void Enemy::setIntentDamage(int amount) { intent = attack("攻击", std::max(0, amount), "造成固定伤害。"); }
void Enemy::applyStrength(int amount) { strength += amount; }
void Enemy::applyWeak(int turns) { weak += std::max(0, turns); }
void Enemy::applyVulnerable(int turns) { vulnerable += std::max(0, turns); }

void Enemy::resolveSplit()
{
    if (archetype != EnemyArchetype::SlimeBoss) return;
    const int splitHealth = currentHealth;
    archetype = EnemyArchetype::SlimePair;
    id = "slime_pair";
    name = "大型史莱姆双体";
    maxHealth = splitHealth * 2;
    currentHealth = maxHealth;
    block = 0;
    patternStep = 0;
    chooseSlimePairIntent();
}

const std::string& Enemy::getId() const { return id; }
const std::string& Enemy::getName() const { return name; }
int Enemy::getCurrentHealth() const { return currentHealth; }
int Enemy::getMaxHealth() const { return maxHealth; }
int Enemy::getBlock() const { return block; }
int Enemy::getIntentDamage() const { return intent.damage; }
int Enemy::getStrength() const { return strength; }
int Enemy::getWeak() const { return weak; }
int Enemy::getVulnerable() const { return vulnerable; }
int Enemy::getRitual() const { return ritual; }
int Enemy::getMetallicize() const { return metallicize; }
int Enemy::getDeathVulnerable() const { return deathVulnerable; }
EnemyArchetype Enemy::getArchetype() const { return archetype; }
const EnemyIntent& Enemy::getIntent() const { return intent; }

std::string Enemy::getPowerDescription() const
{
    switch (archetype)
    {
    case EnemyArchetype::Cultist:
        return ritual > 0 ? "仪式 " + std::to_string(ritual) : "准备仪式";
    case EnemyArchetype::FungiBeast:
        return "孢子云 2：死亡时施加 2 层易伤";
    case EnemyArchetype::Lagavulin:
        return metallicize > 0 ? "金属化 8：回合结束获得 8 格挡" : "金属化已解除";
    case EnemyArchetype::SlimeBoss:
        return "分裂：生命不高于 50% 时改变意图";
    case EnemyArchetype::SlimePair:
        return "双体：以共享血量表示两只大型史莱姆";
    default:
        return {};
    }
}

bool Enemy::isDead() const { return currentHealth <= 0; }
