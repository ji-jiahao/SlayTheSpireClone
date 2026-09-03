#include "ui/UiHelpers.hpp"

#include <sstream>

namespace UiHelpers
{
sf::String toSfString(const std::string& text)
{
    return sf::String::fromUtf8(text.begin(), text.end());
}

sf::Text makeText(const sf::Font& font, const std::string& text, unsigned int size,
                  sf::Color color)
{
    sf::Text result(font, toSfString(text), size);
    result.setFillColor(color);
    return result;
}

std::vector<std::string> wrapText(const sf::Font& font, const std::string& text,
                                  unsigned int characterSize, float maxWidth)
{
    std::vector<std::string> lines;
    std::istringstream words(text);
    std::string word;
    std::string line;

    sf::Text measure(font, "", characterSize);

    while (words >> word)
    {
        const std::string candidate = line.empty() ? word : line + " " + word;
        measure.setString(toSfString(candidate));
        if (!line.empty() && measure.getLocalBounds().size.x > maxWidth)
        {
            lines.push_back(line);
            line = word;
        }
        else
        {
            line = candidate;
        }
    }

    if (!line.empty())
    {
        lines.push_back(line);
    }

    return lines;
}

void drawText(sf::RenderWindow& window, const sf::Font& font, const std::string& text,
              unsigned int size, sf::Vector2f position, sf::Color color)
{
    sf::Text drawable = makeText(font, text, size, color);
    drawable.setPosition(position);
    window.draw(drawable);
}

void drawCenteredText(sf::RenderWindow& window, const sf::Font& font,
                      const std::string& text, unsigned int size,
                      const sf::FloatRect& bounds, sf::Color color)
{
    sf::Text drawable = makeText(font, text, size, color);
    const sf::FloatRect textBounds = drawable.getLocalBounds();
    drawable.setPosition({bounds.position.x + (bounds.size.x - textBounds.size.x) / 2.0f -
                              textBounds.position.x,
                          bounds.position.y + (bounds.size.y - textBounds.size.y) / 2.0f -
                              textBounds.position.y - 2.0f});
    window.draw(drawable);
}

void drawButton(sf::RenderWindow& window, const sf::Font& font,
                const sf::FloatRect& bounds, const std::string& label,
                bool enabled, bool highlighted)
{
    sf::RectangleShape shadow(bounds.size);
    shadow.setPosition({bounds.position.x + 4.0f, bounds.position.y + 5.0f});
    shadow.setFillColor(sf::Color(10, 10, 12, 150));
    window.draw(shadow);

    sf::RectangleShape button(bounds.size);
    button.setPosition(bounds.position);
    button.setFillColor(enabled ? (highlighted ? sf::Color(181, 91, 47) : sf::Color(76, 70, 66))
                                : sf::Color(52, 52, 55));
    button.setOutlineColor(enabled ? sf::Color(224, 185, 111) : sf::Color(96, 96, 100));
    button.setOutlineThickness(2.0f);
    window.draw(button);
    drawCenteredText(window, font, label, 22, bounds,
                     enabled ? sf::Color(246, 236, 215) : sf::Color(140, 140, 144));
}

bool contains(const sf::FloatRect& bounds, sf::Vector2f point)
{
    return bounds.contains(point);
}
}
