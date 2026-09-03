#pragma once

#include <SFML/Graphics.hpp>

#include <string>
#include <vector>

namespace UiHelpers
{
sf::String toSfString(const std::string& text);
sf::Text makeText(const sf::Font& font, const std::string& text, unsigned int size,
                  sf::Color color);
std::vector<std::string> wrapText(const sf::Font& font, const std::string& text,
                                  unsigned int characterSize, float maxWidth);
void drawText(sf::RenderWindow& window, const sf::Font& font, const std::string& text,
              unsigned int size, sf::Vector2f position, sf::Color color);
void drawCenteredText(sf::RenderWindow& window, const sf::Font& font,
                      const std::string& text, unsigned int size,
                      const sf::FloatRect& bounds, sf::Color color);
void drawButton(sf::RenderWindow& window, const sf::Font& font,
                const sf::FloatRect& bounds, const std::string& label,
                bool enabled = true, bool highlighted = false);
bool contains(const sf::FloatRect& bounds, sf::Vector2f point);
}
