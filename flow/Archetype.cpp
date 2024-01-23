#include "Archetype.hpp"

namespace Flow {

EntityLocation Archetype::SetArchetype(EntityID entity)
{
    EntityStorageInfo& entityInfo = world.entitiesStorageInfo[entity.value];
    if (entityInfo.archetypeID != archetypeID && entityInfo.archetypeID != 0) {
        Archetype& oldArcheType = world.GetArcheType(entityInfo.archetypeID);
        oldArcheType.UnSetArchetype(entity);
    }

    uint32 localEntityID = numEntities++;
    uint32 chunkIndex = localEntityID / maxEntitiesPerChunk;

    // 确保chunk够用
    while (chunkIndex >= maxChunk) {
        chunks.push_back(new DataChunk);
        maxChunk++;
        chunks[chunkIndex]->header.maxCapcity = maxEntitiesPerChunk;
        chunks[chunkIndex]->header.DataChunkIndex = chunkIndex;
    }
    chunks[chunkIndex]->header.used += 1;

    // update entity infos
    entityInfo.archetypeID = archetypeID;
    entityInfo.dataChunkIndex = chunkIndex;
    entityInfo.indexInDataChunk = localEntityID % maxEntitiesPerChunk;

    // set entityid
    EntityLocation location = { *this, entityInfo.dataChunkIndex, entityInfo.indexInDataChunk };
    location.SetComponentData<EntityID>(entity);

    return location;
}

bool Archetype::UnSetArchetype(EntityID entity)
{
    EntityStorageInfo& entityInfo = world.entitiesStorageInfo[entity.value];
    assert(entityInfo.archetypeID == archetypeID);

    size_t chunkIndex = entityInfo.dataChunkIndex;
    size_t indexInDataChunk = entityInfo.indexInDataChunk;
    size_t lastEntityChunkIndex = maxChunk - 1;
    size_t lastEntityIndexInDataChunk = numEntities % maxEntitiesPerChunk;

    const bool bIsLastEntityInArchetype = chunkIndex == lastEntityChunkIndex && indexInDataChunk == lastEntityIndexInDataChunk;
    // 如果是最后一个entity，直接移除, 否则先将最后一个元素拷贝到被移除的entity的位置
    if (!bIsLastEntityInArchetype) {
        size_t numComponents = componentIDs.size();

        for (size_t index = 0; index < numComponents; ++index) {
            CopyComponentCommand copyCMD;
            copyCMD.pSrcDataChunk = (byte*)chunks[lastEntityChunkIndex];
            copyCMD.pDstDataChunk = (byte*)chunks[chunkIndex];
            copyCMD.srcComponentOffset = componentChunkOffsets[index];
            copyCMD.dstComponentOffset = componentChunkOffsets[index];
            copyCMD.srcIndexInChunk = lastEntityIndexInDataChunk;
            copyCMD.dstIndexInChunk = indexInDataChunk;
            copyCMD.numBytesPerElement = componentMetaInfos[index]->size;
            copyCMD.numElement = 1;
            copyCMD.Excute();
        }
    }

    // remove last entity
    numEntities -= 1;
    DataChunk* pLastChunk = chunks[lastEntityChunkIndex];
    pLastChunk->header.used -= 1;
    if (pLastChunk->header.used == 0) {
        delete pLastChunk;
        chunks.pop_back();
        maxChunk -= 1;
    }

    return true;
}

Archetype& ArchetypeBuilder::BuildArchetype()
{
    size_t numComponents = componentIDs.size();

    ArchetypeID archeTypeID = ArchetypeID::Invalid;
    if (numComponents > 0) {
        std::ranges::sort(componentIDs);
        const auto ret = std::ranges::unique(componentIDs);
        componentIDs.erase(ret.begin(), ret.end());

        auto _r = componentMetaInfos.empty();
        for (auto componentID : componentIDs) {
            componentMetaInfos.push_back(componentMetaInfoMap[componentID.value]);
        }
        assert(componentMetaInfos.size() == componentIDs.size());

        const uint64 hash = CityHash64((const char*)componentIDs.data(), componentIDs.size() * sizeof(ComponentID));
        auto result = world.archetypeMap.find(hash);
        if (result == world.archetypeMap.end()) {
            ArchetypeID archeTypeID = world.archeTypes.size();
            Archetype newArchetype { world };
            newArchetype.archetypeHash = hash;
            newArchetype.archetypeID = archeTypeID;
            newArchetype.componentIDs = componentIDs;
            newArchetype.componentMetaInfos = componentMetaInfos;

            std::vector<uint32> componentOffsets;
            size_t totalComponentBytes = sizeof(EntityID);
            for (size_t i = 0; i < numComponents; i++) {
                totalComponentBytes += componentMetaInfos[i]->size;
            }
            size_t availibleStorage = sizeof(DataChunk::storage);
            // 2 less than the real count to account for sizes and give some slack
            size_t itemCount = (availibleStorage / totalComponentBytes) - 2;

            uint32 offsets = sizeof(DataChunkHeader);
            offsets += sizeof(EntityID) * itemCount;

            newArchetype.componentIDToLocalIndex.reserve(numComponents);
            newArchetype.componentIDToChunkOffsets.reserve(numComponents);
            for (size_t i = 0; i < numComponents; i++) {
                const MetaInfo* type = componentMetaInfos[i];

                if (type->align != 0) {
                    // align properly
                    size_t remainder = offsets % type->align;
                    size_t oset = type->align - remainder;
                    offsets += oset;
                }

                componentOffsets.push_back(offsets);
                newArchetype.componentIDToLocalIndex.emplace(componentIDs[i], i);
                newArchetype.componentIDToChunkOffsets.emplace(componentIDs[i], offsets);

                if (type->align != 0) {
                    offsets += type->size * (itemCount);
                }
            }
            assert(componentOffsets.size() == numComponents);
            newArchetype.componentChunkOffsets = componentOffsets;
            newArchetype.maxEntitiesPerChunk = itemCount;

            world.archeTypes.push_back(newArchetype);
            world.archetypeMap.emplace(hash, archeTypeID);
            return world.archeTypes.back();
        }
        archeTypeID = world.archetypeMap[result->second.value];
    } else {
        archeTypeID = 0;

        if (archeTypeID == world.archeTypes.size()) {
            constexpr size_t totalComponentBytes = sizeof(EntityID);
            constexpr size_t availibleStorage = sizeof(DataChunk::storage);
            constexpr size_t itemCount = (availibleStorage / totalComponentBytes);

            static_assert((availibleStorage % totalComponentBytes) == 0);
            Archetype newArchetype { world };
            newArchetype.archetypeHash = CityHash64("only entity archetype", strlen("only entity archetype"));
            newArchetype.archetypeID = archeTypeID;
            newArchetype.maxEntitiesPerChunk = itemCount;
            world.archeTypes.push_back(newArchetype);
        }
    }

    return world.archeTypes[archeTypeID.value];
};
}
