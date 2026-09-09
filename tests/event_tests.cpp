#include "event/EventDatabase.hpp"
#include "event/EventSystem.hpp"

#include <cassert>
#include <iostream>

int main(int argc, char** argv)
{
    const std::string eventDataPath =
        argc > 1 ? argv[1] : "assets/data/events.json";

    EventDatabase database;
    assert(database.loadFromFile(eventDataPath));
    assert(database.hasEvent("university_choice"));
    assert(database.hasEvent("sacred_nailong"));

    const std::string expectedBackground =
        "assets/images/event/torch_stone_event_background.png";
    assert(database.getEvent("university_choice").backgroundPath ==
           expectedBackground);
    assert(database.getEvent("sacred_nailong").backgroundPath ==
           expectedBackground);
    assert(database.getEvent("university_choice").states[0].soundPath ==
           "assets/sounds/university_event.mp3");
    assert(database.getEvent("university_choice").states[2].soundPath ==
           "assets/sounds/laoda_theme.ogg");

    for (int choice = 0; choice < 2; ++choice)
    {
        GameState state;
        state.currentHealth = 37;
        state.gold = 123;
        EventSystem events(database);
        assert(events.startEvent("university_choice"));
        assert(events.chooseOption(choice, state));
        assert(events.getCurrentStateIndex() == static_cast<std::size_t>(choice + 1));
        assert(!events.isFinished());
        assert(state.currentHealth == 37 && state.gold == 123);
        assert(state.battleStartStrength == (choice == 0 ? 1 : 0));
        assert(state.battleStartEnemyWeak == (choice == 1 ? 1 : 0));
        assert(!events.chooseOption(choice, state));
        assert(!events.chooseOption(1 - choice, state));
        assert(events.finishEvent(state));
        assert(state.hasVisitedEvent("university_choice"));
        assert(!events.chooseOption(choice, state));
        assert(state.battleStartStrength == (choice == 0 ? 1 : 0));
        assert(state.battleStartEnemyWeak == (choice == 1 ? 1 : 0));

        const GameState retryState = state;
        assert(retryState.battleStartStrength == state.battleStartStrength);
        assert(retryState.battleStartEnemyWeak == state.battleStartEnemyWeak);
        state.reset();
        assert(state.battleStartStrength == 0 && state.battleStartEnemyWeak == 0);
    }
    for (const auto type : {EventEffectType::GainBattleStartStrength,
                            EventEffectType::GainBattleStartEnemyWeak})
    {
        assert(eventEffectTypeFromString(eventEffectTypeToString(type)) == type);
    }

    std::cout << "事件背景和大学加成测试通过。\n";
    return 0;
}
