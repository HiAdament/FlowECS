#pragma once
#include "Common.hpp"
#include "Meta.hpp"
#include "Template.hpp"
#include "World.hpp"
#include "robin_hood.h"
#include <algorithm>
#include <cassert>
#include <ranges>
#include <unordered_map>
#include <vector>

namespace Flow {

struct DataChunkHeader {
    uint32 DataChunkIndex = 0;
    uint32 used = 0;
    uint32 maxCapcity = 0;
};

struct alignas(32) DataChunk {
    byte storage[BLOCK_MEMORY_16K - sizeof(DataChunkHeader)];
    DataChunkHeader header;
};

template <typename T>
struct ComponentsView {

    ComponentsView() = default;

    ComponentsView(void* pointer, DataChunkHeader* header)
        : data((T*)pointer)
        , chunkHeader(header)
    {
    }

    inline const T& operator[](uint32 index) const noexcept
    {
        return data[index];
    }
    inline T& operator[](uint32 index) noexcept
    {
        return data[index];
    }
    bool valid() const noexcept
    {
        return data != nullptr;
    }
    T* begin() noexcept
    {
        return data;
    }
    int16_t size() noexcept
    {
        return chunkHeader->used;
    }

    T* data = nullptr;
    DataChunkHeader* chunkHeader = nullptr;
};

struct Archetype {
    Archetype(World& inWorld)
        : world(inWorld) {};

    World& world;

    ArchetypeID archetypeID;
    uint64 archetypeHash;
    std::vector<ComponentID> componentIDs;
    std::vector<MetaInfo*> componentMetaInfos;
    std::vector<uint32> componentChunkOffsets;

    robin_hood::unordered_flat_map<ComponentID, uint32> componentIDToLocalIndex;
    robin_hood::unordered_flat_map<ComponentID, uint32> componentIDToChunkOffsets;

    uint32 maxEntitiesPerChunk;

    uint32 numEntities = 0;
    uint32 maxChunk = 0;
    std::vector<DataChunk*> chunks;

    DataChunk* operator[](uint32 index) const noexcept
    {
        return chunks[index];
    }

    template <typename _Component>
    bool HasComponent()
    {
        ComponentID testComponentID = TypeIDGenerator<TypeGroup::Component>::Get<_Component>();
        auto res = std::find(componentIDs.begin(), componentIDs.end(), testComponentID);
        return res != componentIDs.end();
    }

    template <typename T>
        requires IsPure<T>::value
    inline auto GetChunkComponentsView(DataChunk* pChunk) const
    {
        using ActualT = ::std::remove_reference_t<T>;

        const ComponentID componentTypeID = TypeIDGenerator<TypeGroup::Component>::Get<T>();

        DataChunkHeader& chunkHeader = pChunk->header;

        if constexpr (std::is_same<ActualT, EntityID>::value) {
            return ComponentsView<EntityID>(pChunk, &chunkHeader);
        } else {
            const auto result = componentIDToChunkOffsets.find(componentTypeID.value);
            assert(result != componentIDToChunkOffsets.end());
            return ComponentsView<ActualT>((byte*)pChunk + result->second, &(chunkHeader));
        }
    }

    EntityLocation SetArchetype(EntityID newEntity);

    bool UnSetArchetype(EntityID entity);
};

struct ArchetypeBuilder {
    ArchetypeBuilder(World& inWorld)
        : world(inWorld) {};

    World& world;
    std::vector<ComponentID> componentIDs;
    std::vector<MetaInfo*> componentMetaInfos;
    std::unordered_map<ComponentID, MetaInfo*> componentMetaInfoMap;

    template <typename _Component>
    bool HasComponent()
    {
        ComponentID testComponentID = TypeIDGenerator<TypeGroup::Component>::Get<_Component>();
        auto res = std::find(componentIDs.begin(), componentIDs.end(), testComponentID);
        return res != componentIDs.end();
    }

    template <typename... _Components>
    ArchetypeBuilder& AddComponent()
    {
        auto newComponentIDs = UnpakToTypeIDs<TypeGroup::Component, TypeList<_Components...>>::values();
        auto newComponentMetaInfos = UnpakToTypeMetaInfo<TypeList<_Components...>>::values();

        assert(newComponentIDs.size() == newComponentMetaInfos.size());
        size_t numNewComponents = newComponentIDs.size();
        for (size_t i = 0; i < numNewComponents; ++i) {
            componentIDs.push_back(newComponentIDs[i]);
            componentMetaInfoMap.try_emplace(newComponentIDs[i], newComponentMetaInfos[i]);
        }
        return *this;
    };

    template <typename... _Components>
    ArchetypeBuilder& RemoveComponent()
    {
        auto deleteComponentIDs = UnpakToTypeIDs<TypeGroup::Component, TypeList<_Components...>>::values();

        auto contains = [](uint32 id) { return deleteComponentIDs.find(id) != deleteComponentIDs.end(); };
        componentIDs | std::views::filter(contains);

        return *this;
    };

    Archetype& BuildArchetype();
};

// todo add check archetype at debug
struct EntityLocation {
    EntityLocation(const Archetype& inArchetype, uint32 inDataChunkIndex, uint32 inIndex)
        : archeType(inArchetype)
        , dataChunkIndex(inDataChunkIndex)
        , index(inIndex) {};

    const Archetype& archeType;
    uint32 dataChunkIndex;
    uint32 index;

    template <typename First, typename... Remains>
    const EntityLocation& SetComponentData(First& first, Remains&... remains) const
    {
        archeType.GetChunkComponentsView<First>(archeType[dataChunkIndex])[index] = first;
        if constexpr (sizeof...(Remains) != 0) {
            SetComponentData(remains...);
        }
        return *this;
    }

    template <typename First, typename... Remains>
    const EntityLocation& SetComponentData(First&& first, Remains&&... remains) const
    {
        archeType.GetChunkComponentsView<First>(archeType[dataChunkIndex])[index] = first;
        if constexpr (sizeof...(Remains) != 0) {
            SetComponentData(remains...);
        }
        return *this;
    }

    template <typename C>
    C& GetComponentData() const
    {
        return archeType.GetChunkComponentsView<C>(archeType[dataChunkIndex])[index];
    }
};

struct CopyComponentCommand {
    byte* pSrcDataChunk;
    byte* pDstDataChunk;
    uint32 srcComponentOffset;
    uint32 dstComponentOffset;
    uint32 srcIndexInChunk;
    uint32 dstIndexInChunk;
    uint32 numBytesPerElement;
    uint32 numElement;

    void Excute()
    {
        byte* pDst = pDstDataChunk + dstComponentOffset + dstIndexInChunk * numBytesPerElement;
        byte* psrc = pSrcDataChunk + srcComponentOffset + srcIndexInChunk * numBytesPerElement;
        memcpy(pDst, psrc, numBytesPerElement * numElement);
    };
};
}
