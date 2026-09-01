#pragma once

#include "map/MapNode.hpp"

#include <SFML/Graphics.hpp>

#include <string>
#include <vector>

// 地图视图：绘制地图节点与连线，并把鼠标点击转换为节点 id。
// 不决定节点类型、事件或可达性，只负责显示。
class MapView
{
public:
    MapView();

    void setFont(const sf::Font& font);

    // 返回被点击的节点 id，未命中返回 -1。
    int handleMouseClick(sf::Vector2f mousePosition,
                         const std::vector<MapNode>& nodes) const;
    void draw(sf::RenderWindow& window, const std::vector<MapNode>& nodes) const;

private:
    sf::Vector2f nodePosition(const std::vector<MapNode>& nodes,
                              const MapNode& node) const;
    sf::FloatRect nodeBounds(const std::vector<MapNode>& nodes,
                             const MapNode& node) const;
    const MapNode* findNodeById(const std::vector<MapNode>& nodes, int id) const;
    sf::Color colorForType(MapNodeType type) const;
    std::string labelForType(MapNodeType type) const;

    const sf::Font* font_;
};
