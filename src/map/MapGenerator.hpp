#ifndef MAPGENERATOR_HPP
#define MAPGENERATOR_HPP

#include "MapNode.hpp"
#include <cstdint>
#include <vector>

class MapGenerator
{
public:
    std::vector<MapNode> generateMap(int rowCount);
    std::vector<MapNode> generateMap(int rowCount, std::uint32_t seed);
};

#endif
