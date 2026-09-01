#include "MapGenerator.hpp"

#include <algorithm>
#include <random>
#include <unordered_set>

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
            else
            {
                std::uniform_int_distribution<int> typeDistribution(0, 4);
                switch (typeDistribution(randomEngine))
                {
                case 0:
                    node.type = MapNodeType::Battle;
                    break;
                case 1:
                    node.type = MapNodeType::Elite;
                    break;
                case 2:
                    node.type = MapNodeType::Rest;
                    break;
                case 3:
                    node.type = MapNodeType::Shop;
                    break;
                default:
                    node.type = MapNodeType::Event;
                    break;
                }
            }

            nodeList.push_back(node);
        }
    }

    for (int row = 0; row < rowCount - 1; ++row)
    {
        std::vector<int> currentRowIndexes;
        std::vector<int> nextRowIndexes;
        for (int index = 0; index < static_cast<int>(nodeList.size()); ++index)
        {
            if (nodeList[index].row == row)
            {
                currentRowIndexes.push_back(index);
            }
            else if (nodeList[index].row == row + 1)
            {
                nextRowIndexes.push_back(index);
            }
        }

        if (nextRowIndexes.empty())
        {
            continue;
        }

        for (int currentIndex : currentRowIndexes)
        {
            const int maxConnectionCount =
                std::min(2, static_cast<int>(nextRowIndexes.size()));
            std::uniform_int_distribution<int> connectionCountDistribution(
                1, maxConnectionCount);
            std::uniform_int_distribution<int> targetDistribution(
                0, static_cast<int>(nextRowIndexes.size()) - 1);

            const int connectionCount = connectionCountDistribution(randomEngine);
            std::unordered_set<int> connectedIds;
            while (static_cast<int>(connectedIds.size()) < connectionCount)
            {
                const int targetIndex = nextRowIndexes[targetDistribution(randomEngine)];
                connectedIds.insert(nodeList[targetIndex].id);
            }

            nodeList[currentIndex].nextNodeIds.assign(connectedIds.begin(),
                                                      connectedIds.end());
            std::sort(nodeList[currentIndex].nextNodeIds.begin(),
                      nodeList[currentIndex].nextNodeIds.end());
        }
    }

    return nodeList;
}
