#include "MapGenerator.hpp"

#include <algorithm>
#include <limits>
#include <numeric>
#include <random>
#include <unordered_map>

namespace
{
constexpr int kMaxOutgoingConnectionCount = 3;
constexpr int kMaxTripleConnectionNodeCount = 2;
constexpr int kMaxShopCountPerPath = 2;
// 前两层保留基础战斗，中段固定安排商店和随机事件，保证路线节奏稳定。
constexpr int kGuaranteedBattleRowCount = 2;
constexpr int kGuaranteedShopRow = 2;

bool isProtectedBattleRow(int row)
{
    return row == 0 || row == 1 || row == 4 || row == 5;
}

bool isGuaranteedShopRow(int row, int rowCount)
{
    return rowCount > kGuaranteedShopRow + 2 && row == kGuaranteedShopRow;
}

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

template <typename RandomEngine>
void assignBarycentricColumns(std::vector<MapNode>& nodeList, int rowCount,
                              RandomEngine& randomEngine)
{
    if (nodeList.empty() || rowCount <= 0)
    {
        return;
    }

    const std::vector<int> firstRowIndexes = findRowIndexes(nodeList, 0);
    for (int column = 0; column < static_cast<int>(firstRowIndexes.size()); ++column)
    {
        nodeList[firstRowIndexes[static_cast<std::size_t>(column)]].column = column;
    }

    std::vector<int> previousRowIndexes = firstRowIndexes;
    for (int row = 1; row < rowCount; ++row)
    {
        std::vector<int> currentRowIndexes = findRowIndexes(nodeList, row);
        if (currentRowIndexes.empty())
        {
            previousRowIndexes.clear();
            continue;
        }

        std::unordered_map<int, float> parentAverageByNodeId;
        for (int nodeIndex : currentRowIndexes)
        {
            const MapNode& currentNode = nodeList[static_cast<std::size_t>(nodeIndex)];
            float sum = 0.0f;
            int parentCount = 0;

            for (int parentIndex : previousRowIndexes)
            {
                const MapNode& parentNode = nodeList[static_cast<std::size_t>(parentIndex)];
                if (std::find(parentNode.nextNodeIds.begin(), parentNode.nextNodeIds.end(),
                              currentNode.id) == parentNode.nextNodeIds.end())
                {
                    continue;
                }

                sum += static_cast<float>(parentNode.column);
                ++parentCount;
            }

            const float value = parentCount > 0
                                    ? sum / static_cast<float>(parentCount)
                                    : static_cast<float>(currentNode.column);
            parentAverageByNodeId.emplace(currentNode.id, value);
        }

        std::shuffle(currentRowIndexes.begin(), currentRowIndexes.end(), randomEngine);
        std::stable_sort(currentRowIndexes.begin(), currentRowIndexes.end(),
                         [&nodeList, &parentAverageByNodeId](int leftIndex, int rightIndex)
                         {
                             const MapNode& leftNode =
                                 nodeList[static_cast<std::size_t>(leftIndex)];
                             const MapNode& rightNode =
                                 nodeList[static_cast<std::size_t>(rightIndex)];
                             const float leftScore = parentAverageByNodeId.at(leftNode.id);
                             const float rightScore = parentAverageByNodeId.at(rightNode.id);
                             if (leftScore == rightScore)
                             {
                                 return leftNode.id < rightNode.id;
                             }

                             return leftScore < rightScore;
                         });

        for (int column = 0; column < static_cast<int>(currentRowIndexes.size()); ++column)
        {
            nodeList[static_cast<std::size_t>(
                currentRowIndexes[static_cast<std::size_t>(column)])].column = column;
        }

        previousRowIndexes = currentRowIndexes;
    }
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
    std::vector<int> shopCandidates;
    for (std::size_t index = 0; index < nodeList.size(); ++index)
    {
        const MapNode& node = nodeList[index];
        if (isProtectedBattleRow(node.row) || isGuaranteedShopRow(node.row, maxRow + 1) ||
            node.row == maxRow ||
            node.row == maxRow - 1)
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
        else if (node.type == MapNodeType::Shop)
        {
            shopCandidates.push_back(static_cast<int>(index));
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
    while (count.battleCount > count.eventCount * 5 && !shopCandidates.empty())
    {
        const int nodeIndex = chooseIndex(shopCandidates);
        nodeList[nodeIndex].type = MapNodeType::Event;
        --count.shopCount;
        ++count.eventCount;
        eventCandidates.push_back(nodeIndex);
    }

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

        // Generated branch rows have equal widths; the final row has one Boss.
        // Always connect the nearest node directly above before adding a fork.
        for (std::size_t i = 0; i < currentRowIndexes.size(); ++i)
            addConnection(nodeList[currentRowIndexes[i]],
                          nodeList[nextRowIndexes[nextRowIndexes.size() == 1 ? 0 : i]].id,
                          tripleConnectionNodeCount);

        if (nextRowIndexes.size() == 1) continue;
        // At each boundary allow at most one diagonal, never both sides of an X.
        // All secondary links are adjacent and each node has at most two exits.
        for (std::size_t i = 0; i + 1 < currentRowIndexes.size(); ++i)
        {
            if (!std::bernoulli_distribution(0.5)(randomEngine)) continue;
            const bool rightward = std::bernoulli_distribution(0.5)(randomEngine);
            const auto source = currentRowIndexes[rightward ? i : i + 1];
            const auto target = nextRowIndexes[rightward ? i + 1 : i];
            if (nodeList[source].nextNodeIds.size() < 2)
                addConnection(nodeList[source], nodeList[target].id, tripleConnectionNodeCount);
        }
    }
}
} // namespace

std::vector<MapNode> MapGenerator::generateMap(int rowCount)
{
    return generateMap(rowCount, std::random_device{}());
}

std::vector<MapNode> MapGenerator::generateMap(int rowCount, std::uint32_t seed)
{
    std::vector<MapNode> nodeList;
    if (rowCount <= 0)
    {
        return nodeList;
    }

    std::mt19937 randomEngine(seed);
    int globalNodeId = 0;
    const int branchCount = rowCount > 1
                                ? std::uniform_int_distribution<int>(2, 4)(randomEngine)
                                : 1;
    const int bossRow = rowCount - 1;
    const int restRow = rowCount - 2;
    const int eliteRow = rowCount - 3;
    const int mandatoryEventRow = rowCount > 6 ? 3 : -1;
    const int optionalEventRow =
        rowCount > 7 && eliteRow - 1 > mandatoryEventRow ? eliteRow - 1 : -1;

    std::vector<int> optionalEventColumns;
    if (optionalEventRow >= 0 && branchCount > 1)
    {
        optionalEventColumns.resize(static_cast<std::size_t>(branchCount));
        std::iota(optionalEventColumns.begin(), optionalEventColumns.end(), 0);
        std::shuffle(optionalEventColumns.begin(), optionalEventColumns.end(),
                     randomEngine);
        const int optionalEventCount =
            std::uniform_int_distribution<int>(1, branchCount - 1)(randomEngine);
        optionalEventColumns.resize(static_cast<std::size_t>(optionalEventCount));
    }

    const auto isOptionalEventColumn = [&optionalEventColumns](int column)
    {
        return std::find(optionalEventColumns.begin(), optionalEventColumns.end(),
                         column) != optionalEventColumns.end();
    };

    for (int row = 0; row < rowCount; ++row)
    {
        const int nodeAmount = row == rowCount - 1 ? 1 : branchCount;

        for (int column = 0; column < nodeAmount; ++column)
        {
            MapNode node{};
            node.id = globalNodeId++;
            node.row = row;
            node.column = column;

            if (row == bossRow)
            {
                node.type = MapNodeType::Boss;
            }
            else if (row == restRow)
            {
                node.type = MapNodeType::Rest;
            }
            else if (row == eliteRow)
            {
                node.type = MapNodeType::Elite;
            }
            else if (isGuaranteedShopRow(row, rowCount))
            {
                node.type = MapNodeType::Shop;
            }
            else if (row == mandatoryEventRow ||
                     (row == optionalEventRow && isOptionalEventColumn(column)))
            {
                node.type = MapNodeType::Event;
            }
            else
            {
                node.type = MapNodeType::Battle;
            }

            nodeList.push_back(node);
        }
    }

    connectRows(nodeList, rowCount, randomEngine);
    // Keep the column order used for constructing the non-crossing edges.

    return nodeList;
}
