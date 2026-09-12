#include "room/TowerBottomSystem.hpp"
#include "card/CardDatabase.hpp"

#include <random>

bool TowerBottomSystem::chooseBlessing(GameState& state, int choice, unsigned int seed)
{
    if (state.towerBlessingChosen || choice < 0 || choice > 2)
    {
        return false;
    }
    if (choice == 0)
    {
        state.roomClearHealing = 8;
    }
    else if (choice == 1)
    {
        std::vector<Card> pool;
        for (const Card& card : CardDatabase::createIroncladCardPool())
        {
            if (card.rarity != CardRarity::Starter && card.rarity != CardRarity::Status &&
                card.rarity != CardRarity::Curse)
            {
                pool.push_back(card);
            }
        }
        if (pool.empty()) return false;
        std::mt19937 engine(seed);
        std::uniform_int_distribution<std::size_t> distribution(0, pool.size() - 1);
        state.addCard(pool[distribution(engine)].id);
    }
    else
    {
        state.battleStartDexterity = 2;
    }
    state.towerBlessingChosen = true;
    return true;
}

int TowerBottomSystem::completeRoom(GameState& state)
{
    // 返回地图与胜利结算可能先后触发，同一节点只结算一次。
    if (state.currentNodeId < 0 || state.lastCompletedNodeId == state.currentNodeId ||
        state.isDead()) return 0;
    state.lastCompletedNodeId = state.currentNodeId;
    return state.heal(state.roomClearHealing);
}
