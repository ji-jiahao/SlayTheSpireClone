#pragma once
#include <vector>
enum class RoomType
{
    Battle,
    Elite,
    Event,
    Shop,
    Rest
};
struct MapNode
{
    int layer;
    RoomType type;
    std::vector<MapNode*> children;
    bool visited;
    MapNode* parent = nullptr;
    float posX;
    float posY;
    bool reachable;
};

