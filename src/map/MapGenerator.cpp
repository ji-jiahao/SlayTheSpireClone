#include "MapGenerator.hpp"

#include <algorithm>
#include <random>
#include <unordered_map>

namespace
{
constexpr int kMaxOutgoingConnectionCount = 3;
constexpr int kMaxTripleConnectionNodeCount = 2;
constexpr int kMaxShopCountPerPath = 2;

struct NodeTypeAllocation
{
    int battleCount = 0;
    int shopCount = 0;
    int eventCount = 0;
};

struct TypeCount
{
    int battleCount = 0;
    int shopCount = 0;
    int eventCount = 0;
};

std::vector<int> findRowIndexes(const std::vector<MapNode>& nodeList, int row)
{
    std::vector<int> indexes;
    for (int index = 0; index < static_cast<int>(nodeList.size()); ++index)
    {
        if (nodeList[index].row == row)
        {
            indexes.push_back(index);
        }
    }

    return indexes;
}

bool addConnection(MapNode& node, int targetId, int& tripleConnectionNodeCount)
{
    if (std::find(node.nextNodeIds.begin(), node.nextNodeIds.end(), targetId) !=
        node.nextNodeIds.end())
    {
        return true;
    }

    const int currentConnectionCount = static_cast<int>(node.nextNodeIds.size());
    if (currentConnectionCount >= kMaxOutgoingConnectionCount)
    {
        return false;
    }

    if (currentConnectionCount == kMaxOutgoingConnectionCount - 1 &&
        tripleConnectionNodeCount >= kMaxTripleConnectionNodeCount)
    {
        return false;
    }

    node.nextNodeIds.push_back(targetId);
    if (currentConnectionCount == kMaxOutgoingConnectionCount - 1)
    {
        ++tripleConnectionNodeCount;
    }

    return true;
}

template <typename RandomEngine>
bool addToLeastConnectedNode(std::vector<MapNode>& nodeList,
                             const std::vector<int>& currentRowIndexes,
                             int targetId,
                             int& tripleConnectionNodeCount,
                             RandomEngine& randomEngine)
{
    std::vector<int> candidates = currentRowIndexes;
    std::shuffle(candidates.begin(), candidates.end(), randomEngine);
    std::stable_sort(candidates.begin(), candidates.end(),
                     [&nodeList](int leftIndex, int rightIndex)
                     {
                         return nodeList[leftIndex].nextNodeIds.size() <
                                nodeList[rightIndex].nextNodeIds.size();
                     });

    for (int currentIndex : candidates)
    {
        if (addConnection(nodeList[currentIndex], targetId,
                          tripleConnectionNodeCount))
        {
            return true;
        }
    }

    return false;
}

bool canKeepBattleEventRatio(int nonShopCount)
{
    if (nonShopCount <= 0)
    {
        return true;
    }

    const int minEventCount = (nonShopCount + 5) / 6;
    const int maxEventCount = nonShopCount / 4;
    return minEventCount <= maxEventCount;
}

template <typename RandomEngine>
NodeTypeAllocation chooseNodeTypeAllocation(int totalNodeCount,
                                            int forcedBattleCount,
                                            RandomEngine& randomEngine)
{
    NodeTypeAllocation allocation;
    const int remainingNodeCount = totalNodeCount - forcedBattleCount;
    if (remainingNodeCount <= 0)
    {
        allocation.battleCount = totalNodeCount;
        return allocation;
    }

    std::vector<NodeTypeAllocation> candidates;
    for (int shopCount = 0;
         shopCount <= std::min(kMaxShopCountPerPath, remainingNodeCount);
         ++shopCount)
    {
        for (int eventCount = 1; eventCount <= remainingNodeCount - shopCount;
             ++eventCount)
        {
            const int battleCount = totalNodeCount - shopCount - eventCount;
            if (battleCount < forcedBattleCount)
            {
                continue;
            }

            if (battleCount >= eventCount * 3 && battleCount <= eventCount * 5)
            {
                candidates.push_back({battleCount, shopCount, eventCount});
            }
        }
    }

    if (candidates.empty())
    {
        allocation.battleCount = totalNodeCount;
        return allocation;
    }

    allocation = candidates[std::uniform_int_distribution<int>(
        0, static_cast<int>(candidates.size()) - 1)(randomEngine)];
    return allocation;
}

TypeCount countNodeTypes(const std::vector<MapNode>& nodeList)
{
    TypeCount count;
    for (const MapNode& node : nodeList)
    {
        if (node.type == MapNodeType::Battle)
        {
            ++count.battleCount;
        }
        else if (node.type == MapNodeType::Shop)
        {
            ++count.shopCount;
        }
        else if (node.type == MapNodeType::Event)
        {
            ++count.eventCount;
        }
    }

    return count;
}

template <typename RandomEngine>
void assignNodeTypes(std::vector<MapNode>& nodeList,
                     const std::vector<int>& forcedBattleIndexes,
                     const std::vector<int>& normalNodeIndexes,
                     RandomEngine& randomEngine)
{
    for (int nodeIndex : forcedBattleIndexes)
    {
        nodeList[nodeIndex].type = MapNodeType::Battle;
    }

    if (normalNodeIndexes.empty())
    {
        return;
    }

    const NodeTypeAllocation allocation = chooseNodeTypeAllocation(
        static_cast<int>(forcedBattleIndexes.size() + normalNodeIndexes.size()),
        static_cast<int>(forcedBattleIndexes.size()), randomEngine);

    std::vector<MapNodeType> nodeTypes;
    nodeTypes.insert(nodeTypes.end(), allocation.shopCount, MapNodeType::Shop);
    nodeTypes.insert(nodeTypes.end(), allocation.eventCount, MapNodeType::Event);
    nodeTypes.insert(nodeTypes.end(),
                      allocation.battleCount - static_cast<int>(forcedBattleIndexes.size()),
                      MapNodeType::Battle);
    std::shuffle(nodeTypes.begin(), nodeTypes.end(), randomEngine);

    for (int index = 0; index < static_cast<int>(normalNodeIndexes.size()); ++index)
    {
        nodeList[normalNodeIndexes[index]].type = nodeTypes[index];
    }
}

void eliminateAdjacentShops(std::vector<MapNode>& nodeList)
{
    std::unordered_map<int, std::size_t> nodeIndexById;
    for (std::size_t index = 0; index < nodeList.size(); ++index)
    {
        nodeIndexById.emplace(nodeList[index].id, index);
    }

    for (const MapNode& node : nodeList)
    {
        if (node.type != MapNodeType::Shop)
        {
            continue;
        }

        for (int targetId : node.nextNodeIds)
        {
            const auto targetIt = nodeIndexById.find(targetId);
            if (targetIt == nodeIndexById.end())
            {
                continue;
            }

            MapNode& targetNode = nodeList[targetIt->second];
            if (targetNode.type == MapNodeType::Shop)
            {
                targetNode.type = MapNodeType::Battle;
            }
        }
    }
}

template <typename RandomEngine>
void rebalanceBattleEventRatio(std::vector<MapNode>& nodeList,
                               RandomEngine& randomEngine)
{
    if (nodeList.empty())
    {
        return;
    }

    const int maxRow = std::max_element(
                           nodeList.begin(), nodeList.end(),
                           [](const MapNode& left, const MapNode& right)
                           {
                               return left.row < right.row;
                           })
                           ->row;

    std::vector<int> battleCandidates;
    std::vector<int> eventCandidates;
    for (std::size_t index = 0; index < nodeList.size(); ++index)
    {
        const MapNode& node = nodeList[index];
        if (node.row == 0 || node.row == maxRow || node.row == maxRow - 1)
        {
            continue;
        }

        if (node.type == MapNodeType::Battle)
        {
            battleCandidates.push_back(static_cast<int>(index));
        }
        else if (node.type == MapNodeType::Event)
        {
            eventCandidates.push_back(static_cast<int>(index));
        }
    }

    auto chooseIndex = [&randomEngine](std::vector<int>& candidates) -> int
    {
        std::uniform_int_distribution<int> distribution(
            0, static_cast<int>(candidates.size()) - 1);
        const int position = distribution(randomEngine);
        const int value = candidates[position];
        candidates.erase(candidates.begin() + position);
        return value;
    };

    TypeCount count = countNodeTypes(nodeList);
    while (count.eventCount > 0 && count.battleCount > count.eventCount * 5 &&
           !battleCandidates.empty())
    {
        const int nodeIndex = chooseIndex(battleCandidates);
        nodeList[nodeIndex].type = MapNodeType::Event;
        --count.battleCount;
        ++count.eventCount;
        eventCandidates.push_back(nodeIndex);
    }

    while (count.eventCount > 0 && count.battleCount < count.eventCount * 3 &&
           !eventCandidates.empty())
    {
        const int nodeIndex = chooseIndex(eventCandidates);
        nodeList[nodeIndex].type = MapNodeType::Battle;
        ++count.battleCount;
        --count.eventCount;
        battleCandidates.push_back(nodeIndex);
    }
}

template <typename RandomEngine>
void connectRows(std::vector<MapNode>& nodeList, int rowCount,
                 RandomEngine& randomEngine)
{
    int tripleConnectionNodeCount = 0;
    for (int row = 0; row < rowCount - 1; ++row)
    {
        const std::vector<int> currentRowIndexes = findRowIndexes(nodeList, row);
        const std::vector<int> nextRowIndexes = findRowIndexes(nodeList, row + 1);

        if (currentRowIndexes.empty() || nextRowIndexes.empty())
        {
            continue;
        }

        std::vector<int> targetIds;
        targetIds.reserve(nextRowIndexes.size());
        for (int nextIndex : nextRowIndexes)
        {
            targetIds.push_back(nodeList[nextIndex].id);
        }
        std::shuffle(targetIds.begin(), targetIds.end(), randomEngine);

        for (int targetId : targetIds)
        {
            addToLeastConnectedNode(nodeList, currentRowIndexes, targetId,
                                    tripleConnectionNodeCount, randomEngine);
        }

        std::uniform_int_distribution<int> targetDistribution(
            0, static_cast<int>(targetIds.size()) - 1);
        for (int currentIndex : currentRowIndexes)
        {
            if (nodeList[currentIndex].nextNodeIds.empty())
            {
                addConnection(nodeList[currentIndex],
                              targetIds[targetDistribution(randomEngine)],
                              tripleConnectionNodeCount);
            }
        }

        for (int currentIndex : currentRowIndexes)
        {
            const int maxConnectionCount =
                std::min(kMaxOutgoingConnectionCount,
                         static_cast<int>(targetIds.size()));
            std::uniform_int_distribution<int> desiredDistribution(
                1, maxConnectionCount);
            const int desiredConnectionCount = desiredDistribution(randomEngine);
            int attemptCount = 0;
            while (static_cast<int>(nodeList[currentIndex].nextNodeIds.size()) <
                       desiredConnectionCount &&
                   attemptCount < maxConnectionCount * 4)
            {
                addConnection(nodeList[currentIndex],
                              targetIds[targetDistribution(randomEngine)],
                              tripleConnectionNodeCount);
                ++attemptCount;
            }

            std::sort(nodeList[currentIndex].nextNodeIds.begin(),
                      nodeList[currentIndex].nextNodeIds.end());
        }
    }
}
} // namespace

std::vector<MapNode> MapGenerator::generateMap(int rowCount)
{
    std::vector<MapNode> nodeList;
    if (rowCount <= 0)
    {
        return nodeList;
    }

    std::random_device randomDevice;
    std::mt19937 randomEngine(randomDevice());
    int globalNodeId = 0;
    std::vector<int> forcedBattleIndexes;
    std::vector<int> normalNodeIndexes;

    for (int row = 0; row < rowCount; ++row)
    {
        int nodeAmount = 1;
        if (row < rowCount - 1)
        {
            std::uniform_int_distribution<int> nodeAmountDistribution(2, 4);
            nodeAmount = nodeAmountDistribution(randomEngine);
        }

        for (int column = 0; column < nodeAmount; ++column)
        {
            MapNode node{};
            node.id = globalNodeId++;
            node.row = row;
            node.column = column;

            if (row == rowCount - 1)
            {
                node.type = MapNodeType::Boss;
            }
            else if (row == rowCount - 2)
            {
                node.type = MapNodeType::Rest;
            }
            else
            {
                if (row == 0)
                {
                    node.type = MapNodeType::Battle;
                    forcedBattleIndexes.push_back(static_cast<int>(nodeList.size()));
                }
                else
                {
                    node.type = MapNodeType::Battle;
                    normalNodeIndexes.push_back(static_cast<int>(nodeList.size()));
                }
            }

            nodeList.push_back(node);
        }
    }

    assignNodeTypes(nodeList, forcedBattleIndexes, normalNodeIndexes,
                    randomEngine);
    connectRows(nodeList, rowCount, randomEngine);
    eliminateAdjacentShops(nodeList);
    rebalanceBattleEventRatio(nodeList, randomEngine);

    return nodeList;
}
