#pragma once
#include "map/MapNode.hpp"
#include <SFML/Graphics.hpp>
#include <cmath>
#include <initializer_list>

namespace MapIcons
{
// Original monochrome line art, shared by map nodes and their legend.
inline void draw(sf::RenderTarget& target, MapNodeType type, sf::Vector2f position,
                 float size, sf::Color ink)
{
    sf::RenderStates states;
    states.transform.translate(position).scale({size / 64, size / 64});
    auto line = [&](std::initializer_list<sf::Vector2f> points, bool close = false) {
        if (points.size() < 2) return;
        auto segment = [&](sf::Vector2f a, sf::Vector2f b) {
            const auto d = b-a;
            sf::RectangleShape s({std::sqrt(d.x*d.x+d.y*d.y),3});
            s.setOrigin({0,1.5f}); s.setPosition(a);
            s.setRotation(sf::radians(std::atan2(d.y,d.x))); s.setFillColor(ink);
            target.draw(s,states);
            sf::CircleShape dot(1.5f,12); dot.setOrigin({1.5f,1.5f}); dot.setFillColor(ink);
            dot.setPosition(a); target.draw(dot,states); dot.setPosition(b); target.draw(dot,states);
        };
        auto prev = points.begin();
        for (auto it=prev+1;it!=points.end();++it) { segment(*prev,*it); prev=it; }
        if(close) segment(*prev,*points.begin());
    };
    auto eye = [&](sf::Vector2f p) {
        sf::CircleShape s(3.5f,16); s.setFillColor(ink); s.setPosition(p); target.draw(s,states);
    };
    switch(type)
    {
    case MapNodeType::Battle:
    case MapNodeType::Elite:
        line({{15,22},{22,15},{42,15},{49,22},{50,37},{43,43},{42,53},{22,53},{21,43},{14,37}},true);
        eye({21,29}); eye({36,29});
        line({{30,41},{32,38},{34,41}});
        line({{28,48},{28,53}}); line({{36,48},{36,53}});
        if(type==MapNodeType::Elite) {
            line({{19,18},{10,8},{10,25},{15,29}});
            line({{45,18},{54,8},{54,25},{49,29}});
        }
        break;
    case MapNodeType::Boss:
        line({{12,22},{8,7},{24,15},{32,4},{40,15},{56,7},{52,22}},true);
        line({{14,27},{50,27},{47,45},{32,57},{17,45}},true);
        line({{21,34},{28,37},{21,39}}); line({{43,34},{36,37},{43,39}});
        line({{26,47},{32,44},{38,47}});
        break;
    case MapNodeType::Shop:
        line({{24,15},{20,5},{32,8},{44,5},{40,15}},true);
        line({{22,19},{14,29},{11,46},{18,55},{46,55},{53,46},{50,29},{42,19}},true);
        line({{24,17},{40,17}});
        line({{38,30},{27,28},{24,33},{39,41},{36,47},{25,45}});
        line({{32,25},{32,50}});
        break;
    case MapNodeType::Event:
        line({{18,18},{23,10},{39,10},{47,18},{45,27},{33,35},{31,42}});
        eye({28.5f,50});
        break;
    case MapNodeType::Rest:
        line({{17,43},{13,34},{17,24},{24,30},{24,18},{34,6},{35,21},{43,16},{50,32},{47,43}},false);
        line({{27,42},{24,35},{32,26},{37,36},{35,43}});
        line({{12,49},{51,58}}); line({{13,58},{51,49}});
        break;
    }
}
}
