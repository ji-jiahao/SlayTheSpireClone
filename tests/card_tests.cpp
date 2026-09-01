#include "card/CardDatabase.hpp"
#include "card/Deck.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

int countCards(const std::vector<Card>& cards, const std::string& id)
{
    return static_cast<int>(std::count_if(
        cards.begin(),
        cards.end(),
        [&id](const Card& card) { return card.id == id; }
    ));
}

void testCardDefinitions()
{
    const Card strike = CardDatabase::createStrike();
    require(strike.id == "strike", "Strike id is incorrect");
    require(strike.type == CardType::Attack, "Strike type is incorrect");
    require(strike.cost == 1 && strike.damage == 6 && strike.block == 0,
            "Strike values are incorrect");

    const Card defend = CardDatabase::createDefend();
    require(defend.id == "defend", "Defend id is incorrect");
    require(defend.type == CardType::Skill, "Defend type is incorrect");
    require(defend.cost == 1 && defend.damage == 0 && defend.block == 5,
            "Defend values are incorrect");

    const Card bash = CardDatabase::createBash();
    require(bash.id == "bash", "Bash id is incorrect");
    require(bash.type == CardType::Attack, "Bash type is incorrect");
    require(bash.cost == 2 && bash.damage == 8 && bash.block == 0,
            "Bash values are incorrect");
}

void testStarterDeck()
{
    const std::vector<Card> cards = CardDatabase::createStarterDeck();
    require(cards.size() == 10, "Starter deck must contain 10 cards");
    require(countCards(cards, "strike") == 5, "Starter deck must contain 5 Strikes");
    require(countCards(cards, "defend") == 4, "Starter deck must contain 4 Defends");
    require(countCards(cards, "bash") == 1, "Starter deck must contain 1 Bash");
}

void testDrawAndDiscard()
{
    Deck deck(CardDatabase::createStarterDeck());
    deck.shuffle(42);

    const std::vector<Card> drawnCards = deck.drawCards(5);
    require(drawnCards.size() == 5, "Drawing 5 cards must draw 5 cards");
    require(deck.getHand().size() == 5, "Hand must contain 5 cards after drawing");
    require(deck.getDrawPile().size() == 5, "Draw pile must contain 5 cards after drawing");

    require(deck.discardCard(0), "Valid hand index must be discarded");
    require(deck.getHand().size() == 4, "Hand must shrink after discarding");
    require(deck.getDiscardPile().size() == 1, "Discard pile must receive discarded card");
    require(!deck.discardCard(99), "Invalid hand index must fail");

    deck.discardHand();
    require(deck.getHand().empty(), "Discarding hand must empty hand");
    require(deck.getDiscardPile().size() == 5, "Discard pile must contain all discarded cards");
}

void testReshuffle()
{
    Deck deck(CardDatabase::createStarterDeck());
    deck.shuffle(7);
    deck.drawCards(10);
    deck.discardHand();

    const std::vector<Card> redrawnCards = deck.drawCards(3);
    require(redrawnCards.size() == 3, "Discard pile must be reshuffled when draw pile is empty");
    require(deck.getHand().size() == 3, "Redrawn cards must enter hand");
    require(deck.getDiscardPile().empty(), "Reshuffled discard pile must be empty");
}

void testDeterministicShuffle()
{
    Deck first(CardDatabase::createStarterDeck());
    Deck second(CardDatabase::createStarterDeck());
    first.shuffle(1234);
    second.shuffle(1234);

    const std::vector<Card> firstDraw = first.drawCards(10);
    const std::vector<Card> secondDraw = second.drawCards(10);
    require(firstDraw.size() == secondDraw.size(), "Same seed must draw same count");
    for (std::size_t index = 0; index < firstDraw.size(); ++index)
    {
        require(firstDraw[index].id == secondDraw[index].id,
                "Same seed must produce the same card order");
    }
}
}

int main()
{
    try
    {
        testCardDefinitions();
        testStarterDeck();
        testDrawAndDiscard();
        testReshuffle();
        testDeterministicShuffle();
        std::cout << "Card tests passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Card tests failed: " << error.what() << '\n';
        return 1;
    }
}
