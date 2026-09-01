#ifndef MAPNODE_HPP
#define MAPNODE_HPP

#include <vector>

enum class MapNodeType
{
    Battle,
    Elite,
    Rest,
    Shop,
    Event,
    Boss
};

struct MapNode
{
    int id;
    int row;
    int column;
    MapNodeType type;
    std::vector<int> nextNodeIds;
};

#endif
