#pragma once

#include "map/MapNode.hpp"

#include <unordered_set>
#include <vector>

class MapState
{
public:
    void reset(std::vector<MapNode> nodes);
    bool chooseNode(int nodeId);
    bool completeCurrentNode();

    const std::vector<MapNode>& getNodes() const;
    const MapNode* getCurrentNode() const;
    bool isReachable(int nodeId) const;
    bool isCompleted(int nodeId) const;
    bool isActComplete() const;
    int getCurrentFloor() const;

private:
    const MapNode* findNode(int nodeId) const;

    std::vector<MapNode> nodes_;
    std::unordered_set<int> completedNodeIds_;
    int currentNodeId_ = -1;
    bool currentNodeCompleted_ = true;
};
