#include "room/ShopSystem.hpp"

#include "card/CardDatabase.hpp"

#include <algorithm>
#include <random>

namespace
{
bool isSellableCard(const Card& card)
{
    return card.rarity != CardRarity::Starter &&
           card.rarity != CardRarity::Status &&
           card.rarity != CardRarity::Curse;
}

template <typename T>
std::vector<T> uniqueShufflePick(std::vector<T> source, std::size_t count,
                                 std::mt19937& engine)
{
    std::shuffle(source.begin(), source.end(), engine);
    std::vector<T> destination;
    destination.reserve(std::min(count, source.size()));
    for (const T& item : source)
    {
        destination.push_back(item);
        if (destination.size() >= count)
        {
            break;
        }
    }

    return destination;
}
} // namespace

void ShopSystem::open(unsigned int seed, int roomId)
{
    lastError_.clear();
    cardOffers_.clear();

    std::mt19937 engine(seed ^ (static_cast<unsigned int>(roomId) * 2654435761u + 0x9e3779b9u));

    std::vector<Card> cards;
    for (const Card& card : CardDatabase::createIroncladCardPool())
    {
        if (isSellableCard(card))
        {
            cards.push_back(card);
        }
    }

    cards = uniqueShufflePick(cards, 8, engine);
    for (const Card& card : cards)
    {
        cardOffers_.push_back({card, priceForCard(card), false});
    }
}

const std::vector<ShopCardOffer>& ShopSystem::getCardOffers() const
{
    return cardOffers_;
}

int ShopSystem::getRemoveCardPrice() const
{
    return 50;
}

bool ShopSystem::buyCard(std::size_t offerIndex, GameState& state)
{
    if (offerIndex >= cardOffers_.size())
    {
        lastError_ = "卡牌索引越界";
        return false;
    }

    ShopCardOffer& offer = cardOffers_[offerIndex];
    if (offer.sold)
    {
        lastError_ = "这张卡牌已经卖出";
        return false;
    }

    if (!state.spendGold(offer.price))
    {
        lastError_ = "金币不足";
        return false;
    }

    state.addCard(offer.card.id);
    offer.sold = true;
    lastError_.clear();
    return true;
}

bool ShopSystem::removeCard(GameState& state, std::size_t cardIndex)
{
    if (cardIndex >= state.deck.size())
    {
        lastError_ = "卡牌索引越界";
        return false;
    }

    if (!state.spendGold(getRemoveCardPrice()))
    {
        lastError_ = "金币不足";
        return false;
    }

    state.removeCardAt(cardIndex);
    lastError_.clear();
    return true;
}

const std::string& ShopSystem::getLastError() const
{
    return lastError_;
}

int ShopSystem::priceForCard(const Card& card) const
{
    switch (card.rarity)
    {
    case CardRarity::Common:
        return 30;
    case CardRarity::Uncommon:
        return 50;
    case CardRarity::Rare:
        return 70;
    case CardRarity::Starter:
        return 30;
    case CardRarity::Status:
    case CardRarity::Curse:
        return 999;
    }

    return 100;
}
