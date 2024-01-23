#pragma once
#include "Archetype.hpp"
#include "Common.hpp"
#include "System.hpp"
#include "robin_hood.h"
#include <vector>

namespace Flow {

struct EntityStorageInfo {
    ArchetypeID archetypeID;
    uint32 dataChunkIndex;
    uint32 indexInDataChunk;
    Generation generation;
};

struct World {
    World();
    ~World();
    // per entities data
    std::vector<EntityStorageInfo> entitiesStorageInfo;
    std::vector<EntityID> freeEntities;

    // per archeTypes Data
    std::vector<Archetype> archeTypes;
    robin_hood::unordered_flat_map<uint64, ArchetypeID> archetypeMap;

    // per system data
    std::vector<SystemData> sysDatas;
    std::vector<std::function<void(SystemData*)>> sysFunctions;

    EntityID CreateEntity();

    void DestoryEntity(EntityID& entity);

    Archetype& GetArcheType(ArchetypeID archetypeID);

    const Archetype& GetArcheType(ArchetypeID archetypeID) const;

    EntityStorageInfo& GetEntityStorageInfo(EntityID& entity);

    void UpdateWorld();

    template <typename _System, typename... Qs>
    void CallSystem(_System& sysInstance, TypeList<Qs...> qs)
    {
        auto tup = std::make_tuple(Qs { sysInstance.sysData->world }...);
        sysInstance.Update(std::get<Qs>(tup)...);
    };

    template <typename _System>
    void RegisterSystem()
    {
        sysDatas.emplace_back(SystemData { *this });
        sysFunctions.emplace_back([&](SystemData* sysData) {
            _System sysInstance {};
            sysInstance.sysData = sysData;
            CallSystem(sysInstance, typename _System::Queries {});
        });
    }
};

}