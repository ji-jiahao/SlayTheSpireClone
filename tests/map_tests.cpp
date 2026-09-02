#include "map/MapGenerator.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
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

void verifyGeneratedMap(const std::vector<MapNode>& nodes)
{
    require(!nodes.empty(), "地图不能为空");

    const int maxRow = findMaxRow(nodes);
    int tripleConnectionNodeCount = 0;
    int battleCount = 0;
    int eventCount = 0;
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
                    "第一个关卡不管哪条线都必须是战斗");
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
        else if (node.row < maxRow - 1)
        {
            require(node.type != MapNodeType::Rest,
                    "普通分支层不能生成休息节点");
            require(node.type != MapNodeType::Elite,
                    "当前地图不应生成精英节点");
        }

        if (node.type == MapNodeType::Battle)
        {
            ++battleCount;
        }
        else if (node.type == MapNodeType::Event)
        {
            ++eventCount;
        }
    }

    require(tripleConnectionNodeCount <= 2,
            "三出边节点总数不能超过 2 个");
    require(!startNodes.empty(), "底层必须存在可选起点");
    require(eventCount > 0, "随机事件节点至少要出现 1 个");
    require(battleCount >= eventCount * 3,
            "战斗与随机事件比例不能低于 3:1");
    require(battleCount <= eventCount * 5,
            "战斗与随机事件比例不能高于 5:1");

    const auto nodeLookup = buildNodeLookup(nodes);
    for (const MapNode* startNode : startNodes)
    {
        verifyShopLimitOnPath(*startNode, nodeLookup, 0);
    }
}
} // namespace

int main()
{
    try
    {
        MapGenerator generator;
        require(generator.generateMap(0).empty(), "0 层地图应为空");

        for (int index = 0; index < 200; ++index)
        {
            verifyGeneratedMap(generator.generateMap(6));
        }

        std::cout << "地图测试通过。\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "地图测试失败: " << error.what() << '\n';
        return 1;
    }
}
