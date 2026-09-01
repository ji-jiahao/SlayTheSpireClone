#include "map/MapGenerator.hpp"

#include <array>

namespace
{
constexpr int kFloorCount = 17;

int nodeCountForRow(int row)
{
    return (row == 8 || row >= 14) ? 1 : 3;
}

MapNodeType nodeTypeFor(int row, int column, std::uint32_t seed)
{
    if (row == 0) return MapNodeType::Battle;
    if (row == 8 || row == 16) return MapNodeType::Treasure;
    if (row == 14) return MapNodeType::Rest;
    if (row == 15) return MapNodeType::Boss;

    constexpr std::array<MapNodeType, 12> pattern = {
        MapNodeType::Battle, MapNodeType::Unknown, MapNodeType::Battle,
        MapNodeType::Rest, MapNodeType::Shop, MapNodeType::Battle,
        MapNodeType::Battle, MapNodeType::Elite, MapNodeType::Unknown,
        MapNodeType::Shop, MapNodeType::Battle, MapNodeType::Rest};
    const std::size_t index = static_cast<std::size_t>((row * 3 + column + seed) % pattern.size());
    return pattern[index];
}
}

std::vector<MapNode> MapGenerator::generateActOne(std::uint32_t seed) const
{
    std::vector<MapNode> nodes;
    std::array<int, kFloorCount> rowStarts{};
    int nextId = 0;

    for (int row = 0; row < kFloorCount; ++row)
    {
        rowStarts[static_cast<std::size_t>(row)] = nextId;
        const int count = nodeCountForRow(row);
        for (int column = 0; column < count; ++column)
        {
            nodes.push_back({nextId++, row, column, nodeTypeFor(row, column, seed), {}});
        }
    }

    for (int row = 0; row < kFloorCount - 1; ++row)
    {
        const int currentCount = nodeCountForRow(row);
        const int nextCount = nodeCountForRow(row + 1);
        const int currentStart = rowStarts[static_cast<std::size_t>(row)];
        const int nextStart = rowStarts[static_cast<std::size_t>(row + 1)];

        for (int column = 0; column < currentCount; ++column)
        {
            MapNode& node = nodes[static_cast<std::size_t>(currentStart + column)];
            if (nextCount == 1)
            {
                node.nextNodeIds.push_back(nextStart);
            }
            else if (currentCount == 1)
            {
                for (int nextColumn = 0; nextColumn < nextCount; ++nextColumn)
                {
                    node.nextNodeIds.push_back(nextStart + nextColumn);
                }
            }
            else
            {
                for (int nextColumn = 0; nextColumn < nextCount; ++nextColumn)
                {
                    if (nextColumn >= column - 1 && nextColumn <= column + 1)
                    {
                        node.nextNodeIds.push_back(nextStart + nextColumn);
                    }
                }
            }
        }
    }
    return nodes;
}
