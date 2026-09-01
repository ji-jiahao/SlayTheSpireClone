#include "ui/MapView.hpp"

#include "ui/UiHelpers.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace
{
constexpr float kNodeRadius = 17.0f;
constexpr float kFloorGap = 105.0f;
constexpr float kMapBottom = 620.0f;
constexpr float kMapTop = 82.0f;
constexpr float kMaxScroll = 16.0f * kFloorGap - (kMapBottom - kMapTop);
constexpr float kDragThreshold = 6.0f;

std::string nodeLabel(MapNodeType type)
{
    switch (type)
    {
    case MapNodeType::Battle: return "M";
    case MapNodeType::Elite: return "E";
    case MapNodeType::Rest: return "R";
    case MapNodeType::Shop: return "$";
    case MapNodeType::Treasure: return "T";
    case MapNodeType::Unknown: return "?";
    case MapNodeType::Boss: return "B";
    }
    return "?";
}

sf::Color nodeColor(MapNodeType type)
{
    switch (type)
    {
    case MapNodeType::Battle: return sf::Color(179, 68, 55);
    case MapNodeType::Elite: return sf::Color(126, 76, 151);
    case MapNodeType::Rest: return sf::Color(224, 140, 55);
    case MapNodeType::Shop: return sf::Color(63, 132, 150);
    case MapNodeType::Treasure: return sf::Color(189, 154, 62);
    case MapNodeType::Unknown: return sf::Color(92, 96, 105);
    case MapNodeType::Boss: return sf::Color(118, 28, 30);
    }
    return sf::Color::White;
}
}

void MapView::setFont(const sf::Font& font) { font_ = &font; }

void MapView::resetScroll()
{
    scrollOffset_ = 0.0f;
    pointerDown_ = false;
    pointerDragged_ = false;
}

void MapView::beginPointer(sf::Vector2f position)
{
    pointerDown_ = position.x >= 340.0f && position.x <= 940.0f;
    pointerDragged_ = false;
    pointerStart_ = position;
    pointerLast_ = position;
}

void MapView::updatePointer(sf::Vector2f position)
{
    if (!pointerDown_) return;
    const sf::Vector2f fromStart = position - pointerStart_;
    if (std::abs(fromStart.x) >= kDragThreshold || std::abs(fromStart.y) >= kDragThreshold)
    {
        pointerDragged_ = true;
    }
    scrollOffset_ += position.y - pointerLast_.y;
    pointerLast_ = position;
    clampScroll();
}

int MapView::endPointer(sf::Vector2f position, const MapState& map)
{
    if (!pointerDown_) return -1;
    pointerDown_ = false;
    if (pointerDragged_)
    {
        pointerDragged_ = false;
        return -1;
    }

    for (const MapNode& node : map.getNodes())
    {
        const sf::Vector2f center = nodePosition(node);
        const float dx = position.x - center.x;
        const float dy = position.y - center.y;
        if (map.isReachable(node.id) && dx * dx + dy * dy <= 26.0f * 26.0f)
        {
            return node.id;
        }
    }
    return -1;
}

void MapView::scroll(float wheelDelta)
{
    scrollOffset_ += wheelDelta * 80.0f;
    clampScroll();
}

void MapView::clampScroll()
{
    scrollOffset_ = std::clamp(scrollOffset_, 0.0f, kMaxScroll);
}

sf::Vector2f MapView::nodePosition(const MapNode& node) const
{
    constexpr std::array<float, 3> columns{520.0f, 640.0f, 760.0f};
    const float x = node.column < 3 ? columns[static_cast<std::size_t>(node.column)] : 640.0f;
    const float y = kMapBottom - static_cast<float>(node.row) * kFloorGap + scrollOffset_;
    return {x, y};
}

void MapView::draw(sf::RenderWindow& window, const MapState& map, const GameState& state) const
{
    sf::RectangleShape background({1280.0f, 720.0f});
    background.setFillColor(sf::Color(26, 25, 29));
    window.draw(background);

    sf::RectangleShape leftBand({340.0f, 720.0f});
    leftBand.setFillColor(sf::Color(38, 35, 38));
    window.draw(leftBand);
    sf::RectangleShape rightBand({340.0f, 720.0f});
    rightBand.setPosition({940.0f, 0.0f});
    rightBand.setFillColor(sf::Color(38, 35, 38));
    window.draw(rightBand);

    for (const MapNode& node : map.getNodes())
    {
        for (int targetId : node.nextNodeIds)
        {
            const auto& nodes = map.getNodes();
            const auto target = std::find_if(nodes.begin(), nodes.end(), [targetId](const MapNode& item) {
                return item.id == targetId;
            });
            if (target == nodes.end()) continue;
            const sf::Vector2f from = nodePosition(node);
            const sf::Vector2f to = nodePosition(*target);
            if ((from.y < 60.0f && to.y < 60.0f) || (from.y > 700.0f && to.y > 700.0f))
            {
                continue;
            }
            const sf::Vector2f delta = to - from;
            const float length = std::sqrt(delta.x * delta.x + delta.y * delta.y);
            sf::RectangleShape edge({length, 2.0f});
            edge.setOrigin({0.0f, 1.0f});
            edge.setPosition(from);
            edge.setRotation(sf::radians(std::atan2(delta.y, delta.x)));
            edge.setFillColor(sf::Color(99, 91, 82));
            window.draw(edge);
        }
    }

    for (const MapNode& node : map.getNodes())
    {
        const bool reachable = map.isReachable(node.id);
        const bool completed = map.isCompleted(node.id);
        const sf::Vector2f position = nodePosition(node);
        if (position.y < 62.0f || position.y > 700.0f) continue;
        sf::CircleShape circle(kNodeRadius, 28);
        circle.setOrigin({kNodeRadius, kNodeRadius});
        circle.setPosition(position);
        circle.setFillColor(completed ? sf::Color(55, 55, 58) : nodeColor(node.type));
        circle.setOutlineColor(reachable ? sf::Color(250, 213, 112) : sf::Color(25, 24, 26));
        circle.setOutlineThickness(reachable ? 5.0f : 2.0f);
        window.draw(circle);
        if (font_ != nullptr)
        {
            UiHelpers::drawCenteredText(window, *font_, completed ? "✓" : nodeLabel(node.type),
                                        17, {{position.x - kNodeRadius,
                                              position.y - kNodeRadius},
                                             {kNodeRadius * 2.0f, kNodeRadius * 2.0f}},
                                        sf::Color(248, 239, 219));
        }
    }

    if (font_ != nullptr)
    {
        UiHelpers::drawText(window, *font_, "第一幕路线", 34, {55.0f, 52.0f},
                            sf::Color(236, 202, 126));
        UiHelpers::drawText(window, *font_, "生命  " + std::to_string(state.currentHealth) + "/" +
                                                   std::to_string(state.maxHealth),
                            22, {55.0f, 120.0f}, sf::Color(224, 105, 91));
        UiHelpers::drawText(window, *font_, "金币  " + std::to_string(state.gold), 22,
                            {55.0f, 158.0f}, sf::Color(238, 199, 90));
        UiHelpers::drawText(window, *font_, "牌组  " + std::to_string(state.deck.size()) + " 张", 22,
                            {55.0f, 196.0f}, sf::Color(190, 205, 215));
        UiHelpers::drawText(window, *font_, "遗物  " + std::to_string(state.relicIds.size()) + " 件", 22,
                            {55.0f, 234.0f}, sf::Color(184, 173, 229));
        UiHelpers::drawText(window, *font_, "按住地图上下拖动", 18, {55.0f, 598.0f},
                            sf::Color(185, 180, 173));
        UiHelpers::drawText(window, *font_, "滚轮也可浏览楼层", 18, {55.0f, 630.0f},
                            sf::Color(185, 180, 173));
        UiHelpers::drawText(window, *font_, "发光节点可前往", 18, {55.0f, 662.0f},
                            sf::Color(185, 180, 173));

        const float progress = kMaxScroll > 0.0f ? scrollOffset_ / kMaxScroll : 0.0f;
        sf::RectangleShape track({4.0f, 520.0f});
        track.setPosition({910.0f, 92.0f});
        track.setFillColor(sf::Color(75, 72, 72));
        window.draw(track);
        sf::RectangleShape thumb({8.0f, 70.0f});
        thumb.setPosition({908.0f, 542.0f - progress * 450.0f});
        thumb.setFillColor(sf::Color(218, 180, 101));
        window.draw(thumb);
    }
}
