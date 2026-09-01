#ifndef MAPGENERATOR_HPP
#define MAPGENERATOR_HPP

#include "MapNode.hpp"
#include <vector>

class MapGenerator
{
public:
    std::vector<MapNode> generateMap(int rowCount);
};

#endif
