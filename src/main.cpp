#include <SFML/Graphics.hpp>

int main()
{
    sf::RenderWindow window(
        sf::VideoMode({1280, 720}),
        "Slay the Spire Clone"
    );
    window.setFramerateLimit(60);

    sf::RectangleShape card({160.0f, 220.0f});
    card.setPosition({100.0f, 420.0f});
    card.setFillColor(sf::Color(230, 220, 190));
    card.setOutlineColor(sf::Color(60, 45, 30));
    card.setOutlineThickness(4.0f);

    sf::RectangleShape enemy({140.0f, 180.0f});
    enemy.setPosition({900.0f, 220.0f});
    enemy.setFillColor(sf::Color(170, 60, 60));

    while (window.isOpen())
    {
        while (const auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
        }

        window.clear(sf::Color(35, 38, 42));
        window.draw(card);
        window.draw(enemy);
        window.display();
    }

    return 0;
}
