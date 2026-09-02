#include "core/GameState.hpp"
#include "room/RestSystem.hpp"
#include "room/ShopSystem.hpp"

#include <cassert>
#include <iostream>

int main()
{
    {
        GameState state;
        assert(state.gold == 0);
        state.currentHealth = 40;
        RestSystem restSystem;
        const int healed = restSystem.rest(state);
        assert(healed == 24);
        assert(state.currentHealth == 64);

        state.currentHealth = 78;
        assert(restSystem.rest(state) == 2);
        assert(state.currentHealth == 80);
    }

    {
        GameState state;
        state.gold = 999;
        RelicSystem relicSystem;
        ShopSystem shopSystem;
        shopSystem.open(state.seed, 7);

        assert(shopSystem.getCardOffers().size() == 6);
        assert(!shopSystem.getRelicOffers().empty());

        const int oldDeckSize = static_cast<int>(state.deck.size());
        const int cardPrice = shopSystem.getCardOffers()[0].price;
        assert(shopSystem.buyCard(0, state));
        assert(static_cast<int>(state.deck.size()) == oldDeckSize + 1);
        assert(state.gold == 999 - cardPrice);
        assert(!shopSystem.buyCard(0, state));

        const int goldBeforeRelic = state.gold;
        const int relicPrice = shopSystem.getRelicOffers()[0].price;
        const std::string relicId = shopSystem.getRelicOffers()[0].relic.id;
        assert(shopSystem.buyRelic(0, state, relicSystem));
        assert(state.gold == goldBeforeRelic - relicPrice);
        assert(state.hasVisitedEvent("") == false);
        bool hasRelic = false;
        for (const std::string& ownedId : state.relicIds)
        {
            if (ownedId == relicId)
            {
                hasRelic = true;
            }
        }
        assert(hasRelic);

        const int goldBeforeRemove = state.gold;
        const int deckSizeBeforeRemove = static_cast<int>(state.deck.size());
        assert(shopSystem.removeCard(state, 0));
        assert(state.gold == goldBeforeRemove - shopSystem.getRemoveCardPrice());
        assert(static_cast<int>(state.deck.size()) == deckSizeBeforeRemove - 1);
    }

    {
        GameState state;
        state.gold = 0;
        ShopSystem shopSystem;
        shopSystem.open(state.seed, 3);
        assert(!shopSystem.buyCard(0, state));
        assert(!shopSystem.removeCard(state, 0));
    }

    std::cout << "Room tests passed.\n";
    return 0;
}
