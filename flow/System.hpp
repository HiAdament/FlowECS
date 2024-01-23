#pragma once
#include "Common.hpp"
#include "Template.hpp"
#include <functional>
#include <vector>

namespace Flow {
struct SystemData {
    SystemData(World& inWorld)
        : world(inWorld)
    {
    }
    World& world;

    std::vector<QueryID> queries;
};

template <typename... _Queries>
struct SystemBase {
    using Queries = TypeList<_Queries...>;

    SystemData* sysData;
};

}