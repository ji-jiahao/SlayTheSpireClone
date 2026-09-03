#ifndef MAPSTATE_HPP
#define MAPSTATE_HPP
#include <SFML/Graphics.hpp>
#include <vector>
#include "MapGenerator.hpp"

class MapState
{
public:
    MapState();
    ~MapState();
    void Update(float dt);
    //窗口由外部调用时传入，不需要SetRenderWindow
    void Render(sf::RenderWindow& window);
    void GenerateNewMap();
private:
    std::vector<MapNode*> m_nodes;
};
#endif

    sf::RenderWindow* m_window;
public:
    sf::RenderWindow* GetRenderWindow() { return m_window; }
    void SetRenderWindow(sf::RenderWindow* win) { m_window = win; }
};


