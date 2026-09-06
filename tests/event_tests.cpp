#include "event/EventDatabase.hpp"

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

    std::cout << "事件背景测试通过。\n";
    return 0;
}
