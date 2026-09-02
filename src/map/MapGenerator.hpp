#pragma once
#include "MapNode.hpp"
#include <vector>

class MapGenerator
{
public:
	std::vector<MapNode*> GenerateMap(int totalLayer);
private:
	bool PathHasShop(MapNode* node);
	void LayoutPosition(std::vector<MapNode*>& allNodes, int totalLayer, float width, float height);
};

