#include "MapGenerator.hpp"
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>

bool MapGenerator::PathHasShop(MapNode* node)
{
    MapNode* cur = node;
    while (cur != nullptr)
    {
        if (cur->type == RoomType::Shop)
        {
            return true;
        }
        cur = cur->parent;
    }
    return false;
}

void MapGenerator::LayoutPosition(std::vector<MapNode*>& allNodes, int totalLayer, float width, float height)
{
    std::vector<std::vector<MapNode*>> layers(totalLayer);
    for (auto n : allNodes)
    {
        layers[n->layer].push_back(n);
    }
    float layerStep = height / static_cast<float>(totalLayer - 1);
    for (int l = 0; l < totalLayer; l++)
    {
        auto& list = layers[l];
        int count = static_cast<int>(list.size());
        float gap = width / static_cast<float>(count + 1);
        for (int i = 0; i < count; i++)
        {
            list[i]->posY = l * layerStep;
            list[i]->posX = gap * static_cast<float>(i + 1);
        }
    }
}

std::vector<MapNode*> MapGenerator::GenerateMap(int totalLayer)
{
    srand(static_cast<unsigned int>(time(nullptr)));
    std::vector<MapNode*> allNodes;
    if (totalLayer <= 0)
    {
        return allNodes;
    }
    MapNode* startNode = new MapNode();
    startNode->layer = 0;
    startNode->type = RoomType::Battle;
    startNode->visited = false;
    startNode->parent = nullptr;
    startNode->reachable = true;
    allNodes.push_back(startNode);

    const int maxBranch = 2;
    std::vector<std::vector<MapNode*>> layers(totalLayer);
    layers[0].push_back(startNode);

    for (int l = 0; l < totalLayer - 1; l++)
    {
        auto& currentLayer = layers[l];
        auto& nextLayer = layers[l+1];
        // 1.先给下一层创建空房间，数量 = 当前层节点数量 + 随机少量分支
        int nextRoomCount = static_cast<int>(currentLayer.size());
        if(rand()%3 == 0 && nextRoomCount <= static_cast<int>(currentLayer.size()) + maxBranch)
        {
            nextRoomCount +=1;
        }
        for(int i=0;i<nextRoomCount;i++)
        {
            MapNode* child = new MapNode();
            child->layer = l+1;
            child->visited = false;
            child->reachable = false;
            nextLayer.push_back(child);
            allNodes.push_back(child);
        }

        // =====杀戮尖塔核心逻辑：就近匹配父子节点，子节点尽量找X坐标最近的上层节点作为父，杜绝远距离连线=====
        for(auto child : nextLayer)
        {
            float minDist = 1e9;
            MapNode* bestParent = nullptr;
            for(auto parent : currentLayer)
            {
                float dist = fabs(parent->posX - child->posX);
                if(dist < minDist)
                {
                    minDist = dist;
                    bestParent = parent;
                }
            }
            if(bestParent)
            {
                child->parent = bestParent;
                bestParent->children.push_back(child);
            }

            //设置房间类型
            if (PathHasShop(child->parent))
            {
                int r = rand() % 4;
                if (r == 0) child->type = RoomType::Battle;
                else if (r == 1) child->type = RoomType::Elite;
                else if (r == 2) child->type = RoomType::Event;
                else child->type = RoomType::Rest;
            }
            else
            {
                int r = rand() % 5;
                if (r == 0) child->type = RoomType::Battle;
                else if (r == 1) child->type = RoomType::Elite;
                else if (r == 2) child->type = RoomType::Event;
                else if (r == 3) child->type = RoomType::Shop;
                else child->type = RoomType::Rest;
            }
        }
    }

    LayoutPosition(allNodes, totalLayer, 800.f, 600.f);
    return allNodes;
}

//优化折线：参考杀戮尖塔，先短向下，小幅横向偏移，再向下，减少长横线段
std::vector<sf::Vector2f> MapGenerator::GetPolyLinePoints(float px, float py, float cx, float cy)
{
    std::vector<sf::Vector2f> pts;
    pts.emplace_back(px, py);
    float segmentY1 = py + (cy-py)*0.25f;
    float segmentY2 = py + (cy-py)*0.75f;
    pts.emplace_back(px, segmentY1);
    pts.emplace_back(cx, segmentY1);
    pts.emplace_back(cx, segmentY2);
    pts.emplace_back(cx, cy);
    return pts;
}
