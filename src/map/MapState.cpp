#include "map/MapState.hpp"

#include <algorithm>
#include <utility>

void MapState::reset(std::vector<MapNode> nodes)
{
    nodes_ = std::move(nodes);
    completedNodeIds_.clear();
    currentNodeId_ = -1;
    currentNodeCompleted_ = true;
}

bool MapState::chooseNode(int nodeId)
{
    if (!isReachable(nodeId)) return false;
    currentNodeId_ = nodeId;
    currentNodeCompleted_ = false;
    return true;
}

bool MapState::completeCurrentNode()
{
    if (currentNodeId_ < 0 || currentNodeCompleted_) return false;
    completedNodeIds_.insert(currentNodeId_);
    currentNodeCompleted_ = true;
    return true;
}

const std::vector<MapNode>& MapState::getNodes() const { return nodes_; }

const MapNode* MapState::getCurrentNode() const
{
    return findNode(currentNodeId_);
}

bool MapState::isReachable(int nodeId) const
{
    const MapNode* candidate = findNode(nodeId);
    if (candidate == nullptr || isCompleted(nodeId) || !currentNodeCompleted_) return false;
    if (currentNodeId_ < 0) return candidate->row == 0;

    const MapNode* current = findNode(currentNodeId_);
    if (current == nullptr) return false;
    return std::find(current->nextNodeIds.begin(), current->nextNodeIds.end(), nodeId) !=
           current->nextNodeIds.end();
}

bool MapState::isCompleted(int nodeId) const
{
    return completedNodeIds_.find(nodeId) != completedNodeIds_.end();
}

bool MapState::isActComplete() const
{
    const MapNode* current = getCurrentNode();
    return current != nullptr && current->row == 16 && currentNodeCompleted_;
}

int MapState::getCurrentFloor() const
{
    const MapNode* current = getCurrentNode();
    return current == nullptr ? 0 : current->row + 1;
}

const MapNode* MapState::findNode(int nodeId) const
{
    const auto found = std::find_if(nodes_.begin(), nodes_.end(), [nodeId](const MapNode& node) {
        return node.id == nodeId;
    });
    return found == nodes_.end() ? nullptr : &*found;
}
