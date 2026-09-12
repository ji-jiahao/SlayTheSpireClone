#include "map/MapGenerator.hpp"
#include "map/MapEncounter.hpp"
#include "core/GameState.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <set>
#include <vector>

namespace
{
void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

int findMaxRow(const std::vector<MapNode>& nodes)
{
    int maxRow = 0;
    for (const MapNode& node : nodes)
    {
        maxRow = std::max(maxRow, node.row);
    }

    return maxRow;
}

std::unordered_map<int, const MapNode*> buildNodeLookup(
    const std::vector<MapNode>& nodes)
{
    std::unordered_map<int, const MapNode*> lookup;
    for (const MapNode& node : nodes)
    {
        lookup.emplace(node.id, &node);
    }

    return lookup;
}

void verifyShopLimitOnPath(
    const MapNode& node,
    const std::unordered_map<int, const MapNode*>& nodeLookup,
    int shopCount)
{
    const int nextShopCount =
        shopCount + (node.type == MapNodeType::Shop ? 1 : 0);
    require(nextShopCount <= 2, "同一条路线上的商店数量不能超过 2 个");

    for (int targetId : node.nextNodeIds)
    {
        const auto targetIt = nodeLookup.find(targetId);
        require(targetIt != nodeLookup.end(), "连线不能指向不存在的节点");
        require(targetIt->second->row == node.row + 1,
                "地图只能连接到上一层节点");
        require(!(node.type == MapNodeType::Shop &&
                  targetIt->second->type == MapNodeType::Shop),
                "相邻两个房间不允许同时为商店");
        verifyShopLimitOnPath(*targetIt->second, nodeLookup, nextShopCount);
    }
}

void verifyPathComposition(
    const MapNode& node,
    const std::unordered_map<int, const MapNode*>& nodeLookup,
    int battleCount,
    int eliteCount,
    int eventCount)
{
    const int nextBattleCount =
        battleCount + (node.type == MapNodeType::Battle ? 1 : 0);
    const int nextEliteCount =
        eliteCount + (node.type == MapNodeType::Elite ? 1 : 0);
    const int nextEventCount =
        eventCount + (node.type == MapNodeType::Event ? 1 : 0);
    if (node.nextNodeIds.empty())
    {
        require(nextEventCount >= 1 && nextEventCount <= 2,
                "每条路线必须经过 1 到 2 个随机事件");
        require(nextEliteCount == 1,
                "每条路线必须且只能经过 1 个精英节点");
        require(nextBattleCount + nextEliteCount >= 4 &&
                    nextBattleCount + nextEliteCount <= 6,
                "每条路线必须经过 4 到 6 个非 Boss 战斗节点");
        return;
    }

    for (int targetId : node.nextNodeIds)
    {
        const auto targetIt = nodeLookup.find(targetId);
        require(targetIt != nodeLookup.end(), "战斗数量检查遇到不存在的节点");
        verifyPathComposition(*targetIt->second, nodeLookup, nextBattleCount,
                              nextEliteCount, nextEventCount);
    }
}

void verifyGeneratedMap(const std::vector<MapNode>& nodes)
{
    require(!nodes.empty(), "地图不能为空");

    const int maxRow = findMaxRow(nodes);
    int tripleConnectionNodeCount = 0;
    int battleCount = 0;
    int eventCount = 0;
    int eliteCount = 0;
    int shopRowCount = 0;
    std::vector<const MapNode*> startNodes;

    for (const MapNode& node : nodes)
    {
        require(node.nextNodeIds.size() <= 3, "节点最多只能连接 3 个后续节点");
        if (node.nextNodeIds.size() == 3)
        {
            ++tripleConnectionNodeCount;
        }

        if (node.row == 0)
        {
            startNodes.push_back(&node);
            require(node.type == MapNodeType::Battle,
                    "第一层需要保留基础战斗教学节点");
        }

        if (node.row == maxRow)
        {
            require(node.type == MapNodeType::Boss, "最顶层必须是 Boss 节点");
            require(node.nextNodeIds.empty(), "Boss 节点不能再向上连接");
        }
        else
        {
            require(!node.nextNodeIds.empty(), "非顶层节点必须有后续路线");
        }

        if (node.row == maxRow - 1)
        {
            require(node.type == MapNodeType::Rest,
                    "Boss 前一层只能放休息节点");
        }
        else if (node.row == maxRow - 2)
        {
            require(node.type == MapNodeType::Elite,
                    "Boss 前两层必须是精英节点");
        }
        else if (node.row < maxRow - 1)
        {
            require(node.type != MapNodeType::Rest,
                    "普通分支层不能生成休息节点");
        }

        if (node.type == MapNodeType::Battle)
        {
            ++battleCount;
        }
        else if (node.type == MapNodeType::Event)
        {
            ++eventCount;
        }
        else if (node.type == MapNodeType::Elite)
        {
            ++eliteCount;
        }

        if (node.row == 2)
        {
            ++shopRowCount;
            require(node.type == MapNodeType::Shop,
                    "第 3 层必须是商店层，确保每条路线至少经过一个商店");
        }

    }

    require(tripleConnectionNodeCount <= 2,
            "三出边节点总数不能超过 2 个");
    require(!startNodes.empty(), "底层必须存在可选起点");
    require(shopRowCount > 0, "地图必须生成至少一个商店层节点");
    require(eventCount > 0, "随机事件节点至少要出现 1 个");
    require(eliteCount > 0, "地图必须生成精英节点");

    const auto nodeLookup = buildNodeLookup(nodes);
    for (const auto& node : nodes)
    {
        if (node.row > 0)
            require(std::any_of(nodes.begin(),nodes.end(),[&](const auto& parent) {
                return std::find(parent.nextNodeIds.begin(),parent.nextNodeIds.end(),node.id) != parent.nextNodeIds.end();
            }), "每个节点都必须能从下面到达");
        for (int target : node.nextNodeIds)
        {
            const auto& dest = *nodeLookup.at(target);
            if (dest.type != MapNodeType::Boss)
                require(std::abs(node.column-dest.column) <= 1, "不能跨越相邻列连接远处节点");
            for (const auto& other : nodes)
                if (other.row == node.row && other.column > node.column)
                    for (int otherTarget : other.nextNodeIds)
                        require(dest.column <= nodeLookup.at(otherTarget)->column,
                                "任意两条边都不能交叉");
        }
        if (node.row < maxRow - 1)
            require(std::any_of(node.nextNodeIds.begin(),node.nextNodeIds.end(),[&](int id) {
                return nodeLookup.at(id)->column == node.column;
            }), "必须保留通往正上方最近节点的连接");
    }
    for (const MapNode* startNode : startNodes)
    {
        verifyShopLimitOnPath(*startNode, nodeLookup, 0);
        verifyPathComposition(*startNode, nodeLookup, 0, 0, 0);
    }
}
void verifyEncounterRandomization()
{
    const std::set<std::string> ordinaryIds = {"cultist", "jaw_worm", "acid_slime", "fungi_beast"};
    std::set<std::string> sequences;
    std::set<std::string> openingIds;
    for (unsigned int seed = 0; seed < 128; ++seed)
    {
        std::string previousId;
        std::string sequence;
        for (unsigned int cycle = 0; cycle < 3; ++cycle)
        {
            std::set<std::string> encountered;
            for (unsigned int offset = 0; offset < 4; ++offset)
            {
                const unsigned int progress = cycle * 4 + offset;
                const MapNode node{static_cast<int>(progress), static_cast<int>(offset), 0,
                                   MapNodeType::Battle, {}};
                const auto encounter = MapEncounter::forNode(node, seed, progress);
                require(ordinaryIds.count(encounter.enemyId) == 1,
                        "普通战斗不得出现精英、Boss 或分裂形态");
                require(encounter.enemyId != previousId, "普通怪不可相邻重复，包括重新洗牌边界");
                require(MapEncounter::forNode(node, seed, progress).enemyId == encounter.enemyId,
                        "相同种子和进度必须复现同一遭遇");
                encountered.insert(encounter.enemyId);
                previousId = encounter.enemyId;
                if (cycle == 0) sequence += encounter.enemyId + ",";
                if (progress == 0) openingIds.insert(encounter.enemyId);
            }
            require(encountered == ordinaryIds, "每四场普通战斗必须覆盖全部普通怪");
        }
        sequences.insert(sequence);
        const auto nodes = MapGenerator{}.generateMap(9, seed);
        for (const auto& node : nodes)
        {
            if (node.type == MapNodeType::Elite)
                require(MapEncounter::forNode(node, seed, 0).enemyId == "lagavulin",
                        "精英只能是乐加维林");
            else if (node.type == MapNodeType::Boss)
                require(MapEncounter::forNode(node, seed, 0).enemyId == "belial",
                        "最终Boss必须保持贝利亚");
            else if (node.type == MapNodeType::Battle)
                require(ordinaryIds.count(MapEncounter::forNode(node, seed, 0).enemyId) == 1,
                        "真实地图的普通节点不得越池");
            else
            {
                bool rejected = false;
                try { MapEncounter::forNode(node, seed, 0); }
                catch (const std::invalid_argument&) { rejected = true; }
                require(rejected, "非战斗节点必须拒绝创建遭遇");
            }
        }
    }
    require(openingIds == ordinaryIds, "全部普通怪都必须有机会在第一场出现");
    require(sequences.size() > 12, "新种子必须带来多种怪物顺序");
    GameState state;
    state.completedOrdinaryBattles = 3;
    state.reset();
    require(state.completedOrdinaryBattles == 0, "新局必须重置普通战斗进度");
}
} // namespace

int main()
{
    try
    {
        verifyEncounterRandomization();
        MapGenerator generator;
        require(generator.generateMap(0).empty(), "0 层地图应为空");
        int earlyServiceCount = 0;
        int earlyShopOrEventMaps = 0;
        int variedEventRouteMaps = 0;

        for (int index = 0; index < 2000; ++index)
        {
            const std::vector<MapNode> nodes = generator.generateMap(9, index);
            verifyGeneratedMap(nodes);
            bool hasEarlyService = false;
            bool hasOptionalEvent = false;
            bool hasOptionalBattle = false;
            int maxRow = findMaxRow(nodes);
            for (const MapNode& node : nodes)
            {
                if (node.row >= 2 && node.row < 5 &&
                    (node.type == MapNodeType::Event || node.type == MapNodeType::Shop))
                {
                    earlyServiceCount++;
                    hasEarlyService = true;
                }
                if (node.row == maxRow - 3 && node.type == MapNodeType::Event)
                {
                    hasOptionalEvent = true;
                }
                if (node.row == maxRow - 3 && node.type == MapNodeType::Battle)
                {
                    hasOptionalBattle = true;
                }
            }
            if (hasEarlyService)
            {
                ++earlyShopOrEventMaps;
            }
            if (hasOptionalEvent && hasOptionalBattle)
            {
                ++variedEventRouteMaps;
            }
        }

        require(earlyServiceCount > 0 && earlyShopOrEventMaps > 0,
                "前段地图应能随机出现事件或商店，不能固定为连续战斗");
        require(variedEventRouteMaps > 0,
                "地图应同时存在一个事件和两个事件的不同路线");

        std::cout << "地图测试通过。\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "地图测试失败: " << error.what() << '\n';
        return 1;
    }
}
