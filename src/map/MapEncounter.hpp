#pragma once

#include "combat/CombatSystem.hpp"
#include "map/MapNode.hpp"

namespace MapEncounter
{
EncounterDefinition forNode(const MapNode& node, unsigned int runSeed,
                             unsigned int completedOrdinaryBattles);
}
