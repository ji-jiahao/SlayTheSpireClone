#pragma once

#include "card/Card.hpp"
#include "core/GameState.hpp"

#include <cstddef>
#include <string>
#include <vector>

struct ShopCardOffer
{
    Card card;
    int price = 0;
    bool sold = false;
};

class ShopSystem
{
public:
    void open(unsigned int seed, int roomId);
    const std::vector<ShopCardOffer>& getCardOffers() const;
    int getRemoveCardPrice() const;
    bool buyCard(std::size_t offerIndex, GameState& state);
    bool removeCard(GameState& state, std::size_t cardIndex);
    const std::string& getLastError() const;

private:
    int priceForCard(const Card& card) const;

    std::vector<ShopCardOffer> cardOffers_;
    std::string lastError_;
};
