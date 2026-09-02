#include "room/ShopSystem.hpp"

#include "card/CardDatabase.hpp"
#include "relic/RelicDatabase.hpp"

#include <algorithm>
#include <random>
#include <stdexcept>
#include <unordered_set>

namespace
{
bool isSellableCard(const Card& card)
{
    return card.rarity != CardRarity::Starter &&
           card.rarity != CardRarity::Status &&
           card.rarity != CardRarity::Curse;
}

bool isSellableRelic(const Relic& relic)
{
    return relic.rarity != RelicRarity::Starter;
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
    relicOffers_.clear();

    std::mt19937 engine(seed ^ (static_cast<unsigned int>(roomId) * 2654435761u + 0x9e3779b9u));

    std::vector<Card> cards;
    for (const Card& card : CardDatabase::createIroncladCardPool())
    {
        if (isSellableCard(card))
        {
            cards.push_back(card);
        }
    }

    cards = uniqueShufflePick(cards, 6, engine);
    for (const Card& card : cards)
    {
        cardOffers_.push_back({card, priceForCard(card), false});
    }

    std::vector<Relic> relics;
    for (const Relic& relic : RelicDatabase::createFoundationRelics())
    {
        if (isSellableRelic(relic))
        {
            relics.push_back(relic);
        }
    }

    relics = uniqueShufflePick(relics, 3, engine);
    for (const Relic& relic : relics)
    {
        relicOffers_.push_back({relic, priceForRelic(relic), false});
    }
}

const std::vector<ShopCardOffer>& ShopSystem::getCardOffers() const
{
    return cardOffers_;
}

const std::vector<ShopRelicOffer>& ShopSystem::getRelicOffers() const
{
    return relicOffers_;
}

int ShopSystem::getRemoveCardPrice() const
{
    return 75;
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

bool ShopSystem::buyRelic(std::size_t offerIndex, GameState& state,
                          RelicSystem& relicSystem)
{
    if (offerIndex >= relicOffers_.size())
    {
        lastError_ = "遗物索引越界";
        return false;
    }

    ShopRelicOffer& offer = relicOffers_[offerIndex];
    if (offer.sold)
    {
        lastError_ = "这个遗物已经卖出";
        return false;
    }

    if (!state.spendGold(offer.price))
    {
        lastError_ = "金币不足";
        return false;
    }

    if (!relicSystem.obtainRelic(state, offer.relic.id))
    {
        state.gainGold(offer.price);
        lastError_ = "遗物已拥有，无法再次购买";
        return false;
    }

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
        return 75;
    case CardRarity::Uncommon:
        return 110;
    case CardRarity::Rare:
        return 150;
    case CardRarity::Starter:
        return 50;
    case CardRarity::Status:
    case CardRarity::Curse:
        return 999;
    }

    return 100;
}

int ShopSystem::priceForRelic(const Relic& relic) const
{
    switch (relic.rarity)
    {
    case RelicRarity::Common:
        return 150;
    case RelicRarity::Uncommon:
        return 250;
    case RelicRarity::Rare:
        return 350;
    case RelicRarity::Boss:
        return 400;
    case RelicRarity::Starter:
        return 999;
    }

    return 200;
}
