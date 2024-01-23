#include "World.hpp"
#include "Archetype.hpp"
#include <cassert>

namespace Flow {
World::World()
{
    ArchetypeBuilder builder { *this };
    Archetype& onlyEntityArchetype = builder.BuildArchetype();
    assert(onlyEntityArchetype.archetypeID == 0);
}

World::~World()
{
    for (auto& archetype : archeTypes) {
        for (auto pDataChunk : archetype.chunks) {
            delete pDataChunk;
        }
    }
}

EntityID World::CreateEntity()
{
    if (freeEntities.size()) {
        EntityID freeEntity = freeEntities.back();
        freeEntities.pop_back();

        entitiesStorageInfo[freeEntity.value].archetypeID = 0;
        entitiesStorageInfo[freeEntity.value].dataChunkIndex = InvalidValue;
        entitiesStorageInfo[freeEntity.value].indexInDataChunk = InvalidValue;
        entitiesStorageInfo[freeEntity.value].generation += 1;

        return freeEntity;
    } else {
        EntityStorageInfo info;
        info.archetypeID = 0;
        info.dataChunkIndex = InvalidValue;
        info.indexInDataChunk = InvalidValue;
        info.generation = 0;

        entitiesStorageInfo.push_back(info);
        return entitiesStorageInfo.size() - 1;
    }
}

void World::DestoryEntity(EntityID& entity)
{
    freeEntities.push_back(entity);
    entitiesStorageInfo[entity.value].archetypeID = 0;
}

Archetype& World::GetArcheType(ArchetypeID archetypeID)
{
    return archeTypes[archetypeID.value];
}

const Archetype& World::GetArcheType(ArchetypeID archetypeID) const
{
    return archeTypes[archetypeID.value];
}

EntityStorageInfo& World::GetEntityStorageInfo(EntityID& entity)
{
    return entitiesStorageInfo[entity.value];
}

void World::UpdateWorld()
{
    size_t numSys = sysFunctions.size();
    for (size_t i = 0; i < numSys; ++i) {
        sysFunctions[i](&sysDatas[i]);
    }
}

}
