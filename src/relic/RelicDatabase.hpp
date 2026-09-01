#pragma once

#include "relic/Relic.hpp"

#include <string>
#include <vector>

class RelicDatabase
{
public:
    static Relic createBurningBlood();
    static Relic createAnchor();
    static Relic createVajra();
    static Relic createById(const std::string& id);
    static std::vector<Relic> createFoundationRelics();
};
