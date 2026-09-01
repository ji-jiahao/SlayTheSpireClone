#include "MapGenerator.hpp"
#include <random>
#include <unordered_set>

std::vector<MapNode> MapGenerator::generateMap(int rowCount)
{
    std::vector<MapNode> nodeList;
    int globalNodeId = 0;
    std::random_device rd;
    std::mt19937 rng(rd());

   
    for (int row = 0; row < rowCount; ++row)
    {
        int nodeAmount;
        if (row == rowCount - 1)
        {
            nodeAmount = 1;
        }
        else
        {
                      std::uniform_int_distribution<int> distNodeNum(2, 4);
            nodeAmount = distNodeNum(rng);
        }

        for (int col = 0; col < nodeAmount; ++col)
        {
            MapNode newNode{};
            newNode.id = globalNodeId;
            newNode.row = row;
            newNode.column = col;

            if (row == rowCount - 1)
            {
                newNode.type = MapNodeType::Boss;
            }
            else
            {
                std::uniform_int_distribution<int> distType(0, 4);
                int typeRand = distType(rng);
                switch (typeRand)
                {
                case 0: newNode.type = MapNodeType::Battle; break;
                case 1: newNode.type = MapNodeType::Elite; break;
                case 2: newNode.type = MapNodeType::Rest; break;
                case 3: newNode.type = MapNodeType::Shop; break;
                case 4: newNode.type = MapNodeType::Event; break;
                }
            }
            nodeList.push_back(newNode);
            globalNodeId++;
        }
    }

    int scanPos = 0;
    for (int row = 0; row < rowCount - 1; ++row)
    {
        int currentRowSize = 0;
        while (scanPos < nodeList.size() && nodeList[scanPos].row == row)
        {
            currentRowSize++;
            scanPos++;
        }
        int nextRowStartIndex = scanPos;
        int nextRowSize = 0;
        while (scanPos < nodeList.size() && nodeList[scanPos].row == row + 1)
        {
            nextRowSize++;
            scanPos++;
        }

        for (int i = 0; i < currentRowSize; ++i)
        {
            int fromIndex = nextRowStartIndex - currentRowSize + i;
            std::unordered_set<int> connectedIds;
            std::uniform_int_distribution<int> connectCountDist(1, nextRowSize);
            int linkNum = connectCountDist(rng);

            for (int k = 0; k < linkNum; ++k)
            {
                std::uniform_int_distribution<int> colDist(0, nextRowSize - 1);
                int targetCol = colDist(rng);
                int targetId = nextRowStartIndex + targetCol;
                if (!connectedIds.count(targetId))
                {
                    connectedIds.insert(targetId);
                    nodeList[fromIndex].nextNodeIds.push_back(targetId);
                }
            }
        }
    }
    return nodeList;
}
