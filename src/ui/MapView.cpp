#include "ui/MapView.hpp"

#include <string>

namespace
{
constexpr float kWindowWidth = 1280.0f;
constexpr float kWindowHeight = 720.0f;

constexpr float kNodeRadius = 24.0f;
constexpr float kRowGap = 96.0f;
constexpr float kColumnGap = 140.0f;
constexpr float kVerticalMargin = 90.0f;

sf::String toSfString(const std::string& text)
{
    return sf::String::fromUtf8(text.begin(), text.end());
}

sf::Text makeText(const sf::Font& font, const std::string& text,
                  unsigned int characterSize, sf::Color color)
{
    sf::Text drawableText(font, toSfString(text), characterSize);
    drawableText.setFillColor(color);
    return drawableText;
}
} // namespace

MapView::MapView() : font_(nullptr) {}

void MapView::setFont(const sf::Font& font)
{
    font_ = &font;
}

const MapNode* MapView::findNodeById(const std::vector<MapNode>& nodes, int id) const
{
    for (const MapNode& node : nodes)
    {
        if (node.id == id)
        {
            return &node;
        }
    }

    return nullptr;
}

sf::Vector2f MapView::nodePosition(const std::vector<MapNode>& nodes,
                                   const MapNode& node) const
{
    // 每行独立水平居中。
    int rowNodeCount = 0;
    for (const MapNode& other : nodes)
    {
        if (other.row == node.row)
        {
            ++rowNodeCount;
        }
    }

    const float rowWidth =
        static_cast<float>(rowNodeCount - 1) * kColumnGap;
    const float startX = (kWindowWidth - rowWidth) / 2.0f;

    return {startX + static_cast<float>(node.column) * kColumnGap,
            kVerticalMargin + static_cast<float>(node.row) * kRowGap};
}

sf::FloatRect MapView::nodeBounds(const std::vector<MapNode>& nodes,
                                  const MapNode& node) const
{
    const sf::Vector2f center = nodePosition(nodes, node);
    return {{center.x - kNodeRadius, center.y - kNodeRadius},
            {kNodeRadius * 2.0f, kNodeRadius * 2.0f}};
}

sf::Color MapView::colorForType(MapNodeType type) const
{
    switch (type)
    {
    case MapNodeType::Battle:
        return sf::Color(196, 84, 70);
    case MapNodeType::Elite:
        return sf::Color(150, 90, 180);
    case MapNodeType::Rest:
        return sf::Color(220, 150, 60);
    case MapNodeType::Shop:
        return sf::Color(80, 140, 200);
    case MapNodeType::Event:
        return sf::Color(150, 150, 160);
    case MapNodeType::Boss:
        return sf::Color(120, 30, 30);
    }

    return sf::Color(150, 150, 150);
}

std::string MapView::labelForType(MapNodeType type) const
{
    switch (type)
    {
    case MapNodeType::Battle:
        return "B";
    case MapNodeType::Elite:
        return "E";
    case MapNodeType::Rest:
        return "R";
    case MapNodeType::Shop:
        return "S";
    case MapNodeType::Event:
        return "?";
    case MapNodeType::Boss:
        return "BOSS";
    }

    return "?";
}

int MapView::handleMouseClick(sf::Vector2f mousePosition,
                              const std::vector<MapNode>& nodes) const
{
    for (const MapNode& node : nodes)
    {
        if (nodeBounds(nodes, node).contains(mousePosition))
        {
            return node.id;
        }
    }

    return -1;
}

void MapView::draw(sf::RenderWindow& window, const std::vector<MapNode>& nodes) const
{
    sf::RectangleShape background({kWindowWidth, kWindowHeight});
    background.setFillColor(sf::Color(28, 31, 37));
    window.draw(background);

    // 连线。
    sf::VertexArray edges(sf::PrimitiveType::Lines);
    for (const MapNode& node : nodes)
    {
        const sf::Vector2f from = nodePosition(nodes, node);
        for (const int nextId : node.nextNodeIds)
        {
            const MapNode* target = findNodeById(nodes, nextId);
            if (target == nullptr)
            {
                continue;
            }

            const sf::Vector2f to = nodePosition(nodes, *target);
            sf::Vertex vertexFrom;
            vertexFrom.position = from;
            vertexFrom.color = sf::Color(120, 120, 130);
            edges.append(vertexFrom);

            sf::Vertex vertexTo;
            vertexTo.position = to;
            vertexTo.color = sf::Color(120, 120, 130);
            edges.append(vertexTo);
        }
    }
    window.draw(edges);

    // 节点。
    for (const MapNode& node : nodes)
    {
        const sf::Vector2f center = nodePosition(nodes, node);

        sf::CircleShape circle(kNodeRadius, 32);
        circle.setOrigin({kNodeRadius, kNodeRadius});
        circle.setPosition(center);
        circle.setFillColor(colorForType(node.type));
        circle.setOutlineColor(sf::Color(20, 20, 24));
        circle.setOutlineThickness(3.0f);
        window.draw(circle);

        if (font_ != nullptr)
        {
            const std::string label = labelForType(node.type);
            const unsigned int size = label == "BOSS" ? 16 : 24;
            sf::Text text = makeText(*font_, label, size, sf::Color(250, 246, 236));
            const sf::FloatRect textBounds = text.getLocalBounds();
            text.setPosition({center.x - textBounds.size.x / 2.0f - textBounds.position.x,
                              center.y - textBounds.size.y / 2.0f - textBounds.position.y});
            window.draw(text);
        }
    }
}
