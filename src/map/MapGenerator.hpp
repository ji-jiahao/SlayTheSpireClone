#pragma once
#include <vector>
#include "MapNode.hpp"
#include <SFML/Graphics.hpp>

class MapGenerator
{
public:
    bool PathHasShop(MapNode* node);
    void LayoutPosition(std::vector<MapNode*>& allNodes, int totalLayer, float width, float height);
    std::vector<MapNode*> GenerateMap(int totalLayer);
    std::vector<sf::Vector2f> GetPolyLinePoints(float px, float py, float cx, float cy);
};


