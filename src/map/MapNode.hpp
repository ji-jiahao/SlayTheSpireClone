#pragma once

#include <vector>

enum class MapNodeType
{
    Battle,
    Elite,
    Rest,
    Shop,
    Treasure,
    Unknown,
    Boss
};

struct MapNode
{
    int id = -1;
    int row = 0;
    int column = 0;
    MapNodeType type = MapNodeType::Battle;
    std::vector<int> nextNodeIds;
};
