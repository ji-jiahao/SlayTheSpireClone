#include "card/Deck.hpp"

#include <algorithm>
#include <iterator>
#include <utility>

Deck::Deck(std::vector<Card> cards)
    : drawPile(std::move(cards)), randomEngine(0)
{
}

void Deck::shuffle(std::uint32_t seed)
{
    randomEngine.seed(seed);
    std::shuffle(drawPile.begin(), drawPile.end(), randomEngine);
}

std::vector<Card> Deck::drawCards(std::size_t count)
{
    std::vector<Card> drawnCards;
    drawnCards.reserve(count);

    while (drawnCards.size() < count)
    {
        if (drawPile.empty())
        {
            if (discardPile.empty())
            {
                break;
            }

            reshuffleDiscardIntoDraw();
        }

        Card card = std::move(drawPile.back());
        drawPile.pop_back();
        hand.push_back(card);
        drawnCards.push_back(std::move(card));
    }

    return drawnCards;
}

bool Deck::discardCard(std::size_t handIndex)
{
    if (handIndex >= hand.size())
    {
        return false;
    }

    discardPile.push_back(std::move(hand[handIndex]));
    hand.erase(hand.begin() + static_cast<std::ptrdiff_t>(handIndex));
    return true;
}

bool Deck::exhaustCard(std::size_t handIndex)
{
    if (handIndex >= hand.size())
    {
        return false;
    }

    exhaustPile.push_back(std::move(hand[handIndex]));
    hand.erase(hand.begin() + static_cast<std::ptrdiff_t>(handIndex));
    return true;
}

void Deck::discardHand()
{
    discardPile.insert(
        discardPile.end(),
        std::make_move_iterator(hand.begin()),
        std::make_move_iterator(hand.end())
    );
    hand.clear();
}

const std::vector<Card>& Deck::getDrawPile() const
{
    return drawPile;
}

const std::vector<Card>& Deck::getHand() const
{
    return hand;
}

const std::vector<Card>& Deck::getDiscardPile() const
{
    return discardPile;
}

const std::vector<Card>& Deck::getExhaustPile() const
{
    return exhaustPile;
}

void Deck::reshuffleDiscardIntoDraw()
{
    drawPile = std::move(discardPile);
    discardPile.clear();
    std::shuffle(drawPile.begin(), drawPile.end(), randomEngine);
}
