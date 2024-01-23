#pragma once

#include "Archetype.hpp"
#include "Common.hpp"
#include "Meta.hpp"
#include "Query.hpp"
#include "System.hpp"
#include "Template.hpp"
#include "World.hpp"
#include <cassert>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace Flow {

struct ECS final {

    static ArchetypeBuilder CreateArchetypeBuilder(World& world)
    {
        return std::move(ArchetypeBuilder { world });
    };

    static ArchetypeBuilder CreateArchetypeBuilder(World& world, ArchetypeID archeTypeID)
    {
        ArchetypeBuilder builder { world };
        Archetype& archeType = world.archeTypes[archeTypeID.value];
        builder.componentIDs = archeType.componentIDs;
        builder.componentMetaInfos = archeType.componentMetaInfos;
        return std::move(builder);
    };

    static bool ConvertEntityArchetype(EntityID entity, Archetype& oldArcheType, Archetype& newArcheType)
    {
        oldArcheType.UnSetArchetype(entity);
        newArcheType.SetArchetype(entity);
        return true;
    }

    static EntityLocation GetEntityLocation(const World& world, EntityID entity)
    {
        const EntityStorageInfo& entityInfo = world.entitiesStorageInfo[entity.value];
        const Archetype& archeType = world.GetArcheType(entityInfo.archetypeID);

        return { archeType, entityInfo.dataChunkIndex, entityInfo.indexInDataChunk };
    }

    template <typename QueryType, typename Func, typename... Args>
    static void IterateEntityChunk(TypeList<Args...> params, const QueryType& query, Archetype& archetype, uint32 chunkIndex, Func&& function)
    {
        DataChunk* pChunk = archetype.chunks[chunkIndex];
        auto tup = std::make_tuple(archetype.GetChunkComponentsView<Args>(pChunk)...);

        size_t numEntities = pChunk->header.used;
        for (size_t i = 0; i < numEntities; ++i) {
            function(std::get<decltype(archetype.GetChunkComponentsView<Args>(pChunk))>(tup)[i]...);
        }
    }

    template <typename Class, typename Ret, typename... Args>
    static TypeList<Args...> args(Ret (Class::*)(Args...) const);

    template <typename Class, typename Ret, typename... Args>
    static TypeList<std::decay_t<Args>...> decay_args(Ret (Class::*)(Args...) const);

    template <typename QueryType, typename... FuncParams>
    static void CheckRW(const QueryType& query, TypeList<FuncParams...> params)
    {
        ONLY_RUN_ONCE

        const std::vector<TypeID>& allTypeIDs = QueryType::template GetQueryFilterToTypeIDs<All>();
        const size_t numAllTypes = allTypeIDs.size();

        const std::vector<TypeID>& iterateParamsTypeIDs = UnpakToTypeIDsWithFilterWrapper<TypeGroup::Component, NotEntityIDType, decltype(params)>::values();
        const std::vector<TypeID>& iterateParamsNeedWriteTypeIDs = UnpakToTypeIDsWith2FilterWrapper<TypeGroup::Component, IsWritableIterateParam, NotEntityIDType, decltype(params)>::values();
        const std::vector<TypeID>& writeTypeIDs = QueryType::template GetQueryFilterToTypeIDs<Write>();
        const size_t numIterateParamsTypeIDs = iterateParamsTypeIDs.size();
        const size_t numIterateParamsNeedWriteTypeIDs = iterateParamsNeedWriteTypeIDs.size();
        const size_t numWriteTypes = writeTypeIDs.size();
        // test iterate params
        {
            size_t testiterateParamIndex = 0;
            for (size_t i = 0; i < numAllTypes && testiterateParamIndex < numIterateParamsTypeIDs; ++i) {
                testiterateParamIndex += allTypeIDs[i] == iterateParamsTypeIDs[testiterateParamIndex];
            }
            // 迭代函数的参数必须出现在AllFIlter中
            assert(testiterateParamIndex == numIterateParamsTypeIDs);
        }

        // check RW or RO
        {
            uint32 testterateParamsNeedWriteIndex = 0;
            for (size_t i = 0; i < numWriteTypes && testterateParamsNeedWriteIndex < numIterateParamsNeedWriteTypeIDs; ++i) {
                testterateParamsNeedWriteIndex += writeTypeIDs[i] == iterateParamsNeedWriteTypeIDs[testterateParamsNeedWriteIndex];
            }
            // 迭代函数的非const &参数必须预先在WriteFIlter中声明
            assert(testterateParamsNeedWriteIndex == numIterateParamsNeedWriteTypeIDs);
        }
    }

    template <typename QueryType, typename Func>
    static std::vector<ArchetypeID> GetMatchingArchetypes(const QueryType& query)
    {
        using params = decltype(args(&Func::operator()));
        using decay_params = decltype(decay_args(&Func::operator()));

        static_assert(IsValidIterateParams<params>::value, "invalid function params type!");

        const std::vector<TypeID>& allTypeIDs = QueryType::template GetQueryFilterToTypeIDs<All>();
        const size_t numAllTypes = allTypeIDs.size();

        const std::vector<TypeID>& anyTypeIDs = QueryType::template GetQueryFilterToTypeIDs<Any>();
        const std::vector<TypeID>& noneTypeIDs = QueryType::template GetQueryFilterToTypeIDs<None>();
        const size_t numAnyTypes = anyTypeIDs.size();
        const size_t numNoneTypes = noneTypeIDs.size();

        CheckRW(query, params {});

        std::vector<ArchetypeID> matchingArchetypes;
        const size_t numArcheTypes = query.world.archeTypes.size();
        for (ArchetypeID archeTypeIndex = 0; archeTypeIndex < numArcheTypes; archeTypeIndex++) {
            Archetype& archeType = query.world.GetArcheType(archeTypeIndex);

            size_t testAllTypeIndex = 0;
            size_t testAnyTypeIndex = 0;
            size_t testNoneTypeIndex = 0;

            bool anyMatched = numAnyTypes == 0;
            bool noneMatched = true;
            const size_t numComponents = archeType.componentIDs.size();
            for (size_t testTypeIndex = 0; testTypeIndex < numComponents; ++testTypeIndex) {
                TypeID testTypeID = archeType.componentIDs[testTypeIndex].value;

                if (testAllTypeIndex < numAllTypes) {
                    if (testTypeID == allTypeIDs[testAllTypeIndex]) {
                        testAllTypeIndex++;
                    } else if (testTypeID > allTypeIDs[testAllTypeIndex]) {
                        break;
                    }
                }

                if (!anyMatched) {
                    while (testAnyTypeIndex < numAnyTypes && testTypeID > anyTypeIDs[testAnyTypeIndex]) {
                        testAnyTypeIndex++;
                    }
                    anyMatched |= testTypeID == anyTypeIDs[testAnyTypeIndex];
                }

                if (testNoneTypeIndex < numNoneTypes) {
                    while (testNoneTypeIndex < numNoneTypes && testTypeID > noneTypeIDs[testNoneTypeIndex]) {
                        testNoneTypeIndex++;
                    }
                    if (testNoneTypeIndex < numNoneTypes && testTypeID == noneTypeIDs[testNoneTypeIndex]) {
                        noneMatched = false;
                        break;
                    }
                }
            }

            bool allMatched = testAllTypeIndex == numAllTypes;
            if (allMatched && anyMatched && noneMatched) {
                matchingArchetypes.push_back(archeTypeIndex);
            }
        }
        return matchingArchetypes;
    }

    template <typename QueryType, typename Func>
    static void ForEach(const QueryType& query, Func&& function)
    {
        using decay_params = decltype(decay_args(&Func::operator()));

        std::vector<ArchetypeID> matchingArchetypes = GetMatchingArchetypes<QueryType, Func>(query);
        size_t numArcheTypes = matchingArchetypes.size();
        for (size_t archetypeIndex = 0; archetypeIndex < numArcheTypes; ++archetypeIndex) {
            Archetype& archeType = query.world.GetArcheType(matchingArchetypes[archetypeIndex]);
            for (size_t chunkIndex = 0; chunkIndex < archeType.maxChunk; ++chunkIndex) {

                IterateEntityChunk(decay_params {}, query, archeType, chunkIndex, function);
            }
        }
    }
};
}