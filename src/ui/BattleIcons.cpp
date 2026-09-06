#include "ui/BattleIcons.hpp"

#include <initializer_list>

namespace BattleIcons
{
void draw(sf::RenderTarget& target, Kind kind, sf::Vector2f position, float size)
{
    const sf::Color ink(22, 24, 29), light(247, 238, 209);
    const sf::Color red(241, 87, 84), green(118, 198, 110), blue(96, 188, 226);
    const float scale = size / 48.0f;
    auto polygon = [&](std::initializer_list<sf::Vector2f> points, sf::Color fill) {
        sf::ConvexShape shape(points.size());
        std::size_t i = 0;
        for (auto p : points) shape.setPoint(i++, p);
        shape.setPosition(position);
        shape.setScale({scale, scale});
        shape.setFillColor(fill);
        shape.setOutlineColor(ink);
        shape.setOutlineThickness(1.5f);
        target.draw(shape);
    };
    auto rect = [&](float x, float y, float w, float h, sf::Color fill) {
        polygon({{x,y},{x+w,y},{x+w,y+h},{x,y+h}}, fill);
    };
    auto sword = [&](bool broken) {
        polygon({{15,31},{32,5},{42,3},{42,13},{24,36}}, broken ? green : light);
        polygon({{10,25},{29,36},{25,41},{6,30}}, broken ? green : red);
        polygon({{12,32},{19,36},{10,46},{4,41}}, sf::Color(216,169,81));
        if (broken) polygon({{26,14},{37,22},{27,25},{34,29},{22,23},{30,21}}, ink);
        else polygon({{33,8},{38,7},{22,31},{19,30}}, sf::Color(255,255,249));
    };
    switch (kind)
    {
    case Kind::Draw:
    case Kind::Discard:
    case Kind::Exhaust:
    {
        const sf::Color color = kind == Kind::Draw ? blue : kind == Kind::Discard ? sf::Color(226,174,91) : sf::Color(178,160,202);
        rect(4,12,27,33, sf::Color(64,69,80));
        rect(9,8,27,33, color);
        rect(14,4,27,33, light);
        rect(18,8,19,25, color);
        if (kind == Kind::Exhaust)
            polygon({{20,28},{18,21},{25,23},{26,11},{31,21},{35,18},{35,28},{29,32}}, red);
        else if (kind == Kind::Draw)
            polygon({{27,11},{19,20},{24,20},{24,29},{30,29},{30,20},{35,20}}, light);
        else
            polygon({{24,12},{30,12},{30,21},{35,21},{27,30},{19,21},{24,21}}, light);
        break;
    }
    case Kind::Attack: sword(false); break;
    case Kind::Weak: sword(true); break;
    case Kind::Strength:
        polygon({{8,29},{5,17},{10,13},{15,16},{14,8},{20,6},{24,10},{28,6},{33,9},{34,15},{39,14},{43,20},{37,33},{31,39},{15,39}}, red);
        rect(14,38,19,7,sf::Color(218,165,75));
        polygon({{12,18},{18,21},{22,18},{28,22},{35,18},{34,24},{28,27},{19,25},{15,30}}, sf::Color(255,179,149));
        break;
    case Kind::Vulnerable:
    case Kind::Guard:
        polygon({{5,9},{24,3},{43,9},{39,29},{32,39},{24,46},{16,39},{9,29}}, kind == Kind::Guard ? blue : sf::Color(237,159,91));
        polygon({{11,13},{24,9},{37,13},{33,29},{24,38},{15,29}}, kind == Kind::Guard ? sf::Color(163,223,240) : sf::Color(255,209,138));
        if (kind == Kind::Vulnerable)
            polygon({{27,5},{21,20},{29,25},{20,42},{24,28},{16,23}}, ink);
        break;
    case Kind::Buff:
        polygon({{24,3},{43,22},{32,22},{32,43},{16,43},{16,22},{5,22}}, green);
        break;
    case Kind::Unknown:
        polygon({{14,8},{33,8},{41,17},{37,26},{28,31},{28,34},{20,34},{20,26},{30,21},{30,17},{18,17},{14,22},{7,17}}, light);
        rect(20,38,8,8,light);
        break;
    }
}
}
