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

        Card card = drawPile.back();
        drawPile.pop_back();
        hand.push_back(card);
        drawnCards.push_back(card);
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

void Deck::addToDiscardPile(const Card& card, std::size_t count)
{
    discardPile.insert(discardPile.end(), count, card);
}

void Deck::addToHand(const Card& card)
{
    hand.push_back(card);
}

void Deck::addToDrawPileTop(const Card& card)
{
    drawPile.push_back(card);
}

void Deck::addToExhaustPile(const Card& card)
{
    exhaustPile.push_back(card);
}

bool Deck::upgradeHandCard(std::size_t handIndex)
{
    if (handIndex >= hand.size())
    {
        return false;
    }

    return hand[handIndex].upgrade();
}

std::size_t Deck::upgradeAllHandCards()
{
    std::size_t upgradedCount = 0;
    for (Card& card : hand)
    {
        if (card.upgrade())
        {
            ++upgradedCount;
        }
    }
    return upgradedCount;
}

std::optional<Card> Deck::takeTopDrawCard()
{
    if (drawPile.empty())
    {
        return std::nullopt;
    }

    Card card = std::move(drawPile.back());
    drawPile.pop_back();
    return card;
}

bool Deck::moveTopDiscardToDrawPile()
{
    if (discardPile.empty())
    {
        return false;
    }

    drawPile.push_back(std::move(discardPile.back()));
    discardPile.pop_back();
    return true;
}

bool Deck::moveDiscardCardToDrawPileTop(std::size_t discardIndex)
{
    if (discardIndex >= discardPile.size())
    {
        return false;
    }

    drawPile.push_back(std::move(discardPile[discardIndex]));
    discardPile.erase(discardPile.begin() +
                      static_cast<std::ptrdiff_t>(discardIndex));
    return true;
}

bool Deck::moveHandCardToDrawPileTop(std::size_t handIndex)
{
    if (handIndex >= hand.size())
    {
        return false;
    }

    drawPile.push_back(std::move(hand[handIndex]));
    hand.erase(hand.begin() + static_cast<std::ptrdiff_t>(handIndex));
    return true;
}

bool Deck::moveExhaustCardToHand(std::size_t exhaustIndex)
{
    if (exhaustIndex >= exhaustPile.size())
    {
        return false;
    }

    hand.push_back(std::move(exhaustPile[exhaustIndex]));
    exhaustPile.erase(exhaustPile.begin() +
                      static_cast<std::ptrdiff_t>(exhaustIndex));
    return true;
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
