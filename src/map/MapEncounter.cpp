#include "map/MapEncounter.hpp"

#include <algorithm>
#include <array>
#include <random>
#include <stdexcept>

EncounterDefinition MapEncounter::forNode(const MapNode& node, unsigned int runSeed,
                                          unsigned int completedOrdinaryBattles)
{
    if (node.type == MapNodeType::Elite)
        return {"乐加维林", 90, 18, "lagavulin"};
    if (node.type == MapNodeType::Boss)
        return {"黑暗奥特曼 贝利亚", 230, 35, "belial"};
    if (node.type != MapNodeType::Battle)
        throw std::invalid_argument("非战斗节点不能生成怪物遭遇");

    std::array<EncounterDefinition, 4> encounters = {{
        {"邪教徒", 40, 6, "cultist"},
        {"颚虫", 42, 11, "jaw_worm"},
        {"酸液史莱姆", 30, 10, "acid_slime"},
        {"真菌兽", 40, 6, "fungi_beast"},
    }};
    std::mt19937 engine(runSeed);
    std::string previousId;
    // 每轮耗尽整个普通池；按完成数量重建，重入同一战斗不会重新抽怪。
    for (unsigned int cycle = 0; cycle <= completedOrdinaryBattles / encounters.size(); ++cycle)
    {
        std::shuffle(encounters.begin(), encounters.end(), engine);
        if (encounters.front().enemyId == previousId)
        {
            std::uniform_int_distribution<std::size_t> next(1, encounters.size() - 1);
            std::swap(encounters.front(), encounters[next(engine)]);
        }
        previousId = encounters.back().enemyId;
    }
    return encounters[completedOrdinaryBattles % encounters.size()];
}
