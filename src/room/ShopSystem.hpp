#pragma once

#include "card/Card.hpp"
#include "core/GameState.hpp"
#include "relic/Relic.hpp"
#include "relic/RelicSystem.hpp"

#include <cstddef>
#include <string>
#include <vector>

struct ShopCardOffer
{
    Card card;
    int price = 0;
    bool sold = false;
};

struct ShopRelicOffer
{
    Relic relic;
    int price = 0;
    bool sold = false;
};

class ShopSystem
{
public:
    void open(unsigned int seed, int roomId);
    const std::vector<ShopCardOffer>& getCardOffers() const;
    const std::vector<ShopRelicOffer>& getRelicOffers() const;
    int getRemoveCardPrice() const;
    bool buyCard(std::size_t offerIndex, GameState& state);
    bool buyRelic(std::size_t offerIndex, GameState& state, RelicSystem& relicSystem);
    bool removeCard(GameState& state, std::size_t cardIndex);
    const std::string& getLastError() const;

private:
    int priceForCard(const Card& card) const;
    int priceForRelic(const Relic& relic) const;

    std::vector<ShopCardOffer> cardOffers_;
    std::vector<ShopRelicOffer> relicOffers_;
    std::string lastError_;
};
