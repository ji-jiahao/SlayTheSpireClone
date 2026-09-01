#include "map/MapGenerator.hpp"
#include "map/MapState.hpp"

#include <cassert>
#include <iostream>

int main()
{
    MapGenerator generator;
    const auto first = generator.generateActOne(42);
    const auto second = generator.generateActOne(42);
    assert(first.size() == second.size());
    assert(!first.empty());

    int treasureCount = 0;
    int bossCount = 0;
    for (std::size_t index = 0; index < first.size(); ++index)
    {
        assert(first[index].id == second[index].id);
        assert(first[index].type == second[index].type);
        assert(first[index].nextNodeIds == second[index].nextNodeIds);
        if (first[index].row == 0) assert(first[index].type == MapNodeType::Battle);
        if (first[index].row == 8 || first[index].row == 16)
        {
            assert(first[index].type == MapNodeType::Treasure);
            ++treasureCount;
        }
        if (first[index].row == 14) assert(first[index].type == MapNodeType::Rest);
        if (first[index].row == 15)
        {
            assert(first[index].type == MapNodeType::Boss);
            ++bossCount;
        }
    }
    assert(treasureCount == 2);
    assert(bossCount == 1);

    MapState map;
    map.reset(first);
    assert(map.isReachable(0));
    assert(map.isReachable(1));
    assert(map.isReachable(2));
    assert(map.chooseNode(0));
    assert(!map.chooseNode(1));
    assert(map.completeCurrentNode());
    assert(map.isCompleted(0));
    assert(!map.isReachable(0));

    std::cout << "Map tests passed.\n";
    return 0;
}
