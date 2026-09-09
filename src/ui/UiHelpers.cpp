#include "ui/UiHelpers.hpp"

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
    const sf::String characters = toSfString(text);
    sf::String line;
    sf::Text measure(font, "", characterSize);
    const auto fits = [&](const sf::String& value)
    {
        measure.setString(value);
        return measure.getLocalBounds().size.x <= maxWidth;
    };
    const auto flush = [&]()
    {
        const auto utf8 = line.toUtf8();
        lines.emplace_back(utf8.begin(), utf8.end());
        line.clear();
    };

    for (std::size_t index = 0; index < characters.getSize();)
    {
        const char32_t character = characters[index++];
        if (character == U'\r')
        {
            continue;
        }
        if (character == U'\n')
        {
            flush();
            continue;
        }

        // Keep ASCII words together when possible; Chinese may wrap per code point.
        sf::String token(character == U'\t' ? U' ' : character);
        if (character > U' ' && character < 127)
        {
            while (index < characters.getSize() && characters[index] > U' ' &&
                   characters[index] < 127)
            {
                token += characters[index++];
            }
        }
        if (!line.isEmpty() && !fits(line + token))
        {
            flush();
            if (token == sf::String(U' '))
            {
                continue;
            }
        }
        // Also break an unusually long word rather than letting it overflow.
        for (char32_t codePoint : token)
        {
            if (!line.isEmpty() && !fits(line + sf::String(codePoint)))
            {
                flush();
            }
            line += codePoint;
        }
    }

    if (!line.isEmpty() || (!characters.isEmpty() &&
                            characters[characters.getSize() - 1] == U'\n'))
    {
        flush();
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
