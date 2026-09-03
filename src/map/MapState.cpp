#include "MapGenerator.hpp"
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <cmath>

std::vector<MapNode*> MapGenerator::GenerateMap(int totalLayer)
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    std::vector<MapNode*> allNodes;
    if (totalLayer <= 0)
        return allNodes;

    const float leftBound = 100.f;
    const float rightBound = 900.f;
    const float layerGap = 120.f;
    std::vector<std::vector<MapNode*>> layers(totalLayer);

    //起点
    MapNode* start = new MapNode();
    start->position = {(leftBound + rightBound) / 2.0f, 30.f};
    layers[0].push_back(start);
    allNodes.push_back(start);

    for (int level = 1; level < totalLayer; level++)
    {
        int roomNum = 2 + std::rand() % 3;
        std::vector<MapNode*> currLayer;
        float segWidth = (rightBound - leftBound) / (roomNum - 1);

        for (int i = 0; i < roomNum; i++)
        {
            MapNode* node = new MapNode();
            node->position.y = 30.f + level * layerGap;
            node->position.x = leftBound + i * segWidth;
            node->position.x += float(std::rand() % 21 - 10);
            currLayer.push_back(node);
            allNodes.push_back(node);
        }
        layers[level] = currLayer;

        auto& prevLayer = layers[level - 1];
        std::vector<int> childConnectCount(currLayer.size(),0);
        //上层从左向右遍历，下层保持左右顺序，不跨序连接
        for (int upIdx = 0; upIdx < prevLayer.size(); upIdx++)
        {
            MapNode* upNode = prevLayer[upIdx];
            int maxChild = 1 + std::rand() % 2;
            int linkCount = 0;

            //从和上层序号接近的下层开始选，不向很远的左右两端连接
            int startDown = std::max(0, upIdx - 1);
            int endDown = std::min((int)currLayer.size()-1, upIdx + 1);

            for(int downIdx = startDown; downIdx <= endDown; downIdx++)
            {
                if(linkCount >= maxChild) break;
                //限制每个下层节点最多接收2条连线，防止线条扎堆
                if(childConnectCount[downIdx] >=2)
                    continue;

                MapNode* downNode = currLayer[downIdx];
                bool duplicate = false;
                for(auto ch : upNode->children)
                {
                    if(ch == downNode)
                    {
                        duplicate = true;
                        break;
                    }
                }
                if(!duplicate)
                {
                    upNode->children.push_back(downNode);
                    childConnectCount[downIdx]++;
                    linkCount++;
                }
            }
        }
    }
    return allNodes;
}
