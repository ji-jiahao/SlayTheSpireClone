#pragma once

#include "card/Card.hpp"

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

class Deck
{
public:
    explicit Deck(std::vector<Card> cards = {});

    void shuffle(std::uint32_t seed);
    std::vector<Card> drawCards(std::size_t count);
    bool discardCard(std::size_t handIndex);
    bool exhaustCard(std::size_t handIndex);
    void discardHand();
    void addToDiscardPile(const Card& card, std::size_t count = 1);

    const std::vector<Card>& getDrawPile() const;
    const std::vector<Card>& getHand() const;
    const std::vector<Card>& getDiscardPile() const;
    const std::vector<Card>& getExhaustPile() const;

private:
    void reshuffleDiscardIntoDraw();

    std::vector<Card> drawPile;
    std::vector<Card> hand;
    std::vector<Card> discardPile;
    std::vector<Card> exhaustPile;
    std::mt19937 randomEngine;
};
