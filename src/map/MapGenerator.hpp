#pragma once

#include "map/MapNode.hpp"

#include <cstdint>
#include <vector>

class MapGenerator
{
public:
    std::vector<MapNode> generateActOne(std::uint32_t seed) const;
};
