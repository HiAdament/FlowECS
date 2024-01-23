#pragma once
#include "Common.hpp"
#include "Template.hpp"
#include <type_traits>

namespace Flow {

template <typename... Components>
    requires(... && (IsPure<Components>::value))
struct All : TypeList<Components...> { };

template <typename Filter>
constexpr bool IsAllFilterType = IsInstanceOf<Filter, All>::value;

template <typename... Components>
    requires(... && (IsPure<Components>::value))
struct Any : TypeList<Components...> { };

template <typename Filter>
constexpr bool IsAnyFilterType = IsInstanceOf<Filter, Any>::value;

template <typename... Components>
    requires(... && (IsPure<Components>::value))
struct None : TypeList<Components...> { };

template <typename Filter>
constexpr bool IsNoneFilterType = IsInstanceOf<Filter, None>::value;

template <typename... Components>
    requires(... && (IsPure<Components>::value))
struct Write : TypeList<Components...> { };

template <typename Filter>
constexpr bool IsWriteFilterType = IsInstanceOf<Filter, Write>::value;

template <typename... Filter>
constexpr bool IsQueryFilterType = (... && (IsAllFilterType<Filter> || IsAnyFilterType<Filter> || IsNoneFilterType<Filter> || IsWriteFilterType<Filter>));

template <template <typename...> typename FilterType, typename FisrtTypeList, typename... Remains>
static void UnpakQueryFilterToTypeIDs(std::vector<uint32>& outResult)
{
    if constexpr (IsInstanceOf<FisrtTypeList, FilterType>::value) {
        auto typeIDs = UnpakToTypeIDs<TypeGroup::Component, FisrtTypeList>::values();
        for (size_t i = 0; i < NumTypes<FisrtTypeList>::value; ++i) {
            outResult.push_back(typeIDs[i]);
        }
    }

    if constexpr (sizeof...(Remains) != 0) {
        UnpakQueryFilterToTypeIDs<FilterType, Remains...>(outResult);
    } else {
        std::sort(outResult.begin(), outResult.end());
        const auto ret = std::ranges::unique(outResult);
        outResult.erase(ret.begin(), ret.end());
    }
}

template <typename... _QueryFilters>
    requires IsQueryFilterType<_QueryFilters...>
struct Query {
    static_assert(IsQueryFilterType<_QueryFilters...>, "IsValidQueryFilter! Query<All<Component1>>");

    Query(World& inWorld)
        : world(inWorld) {};

    template <template <typename...> typename QueryFilterType>
        requires IsQueryFilterType<_QueryFilters...>
    static const std::vector<TypeID>& GetQueryFilterToTypeIDs()
    {
        auto get = []() {
            std::vector<TypeID> r;
            UnpakQueryFilterToTypeIDs<QueryFilterType, _QueryFilters...>(r);
            return r;
        };
        static std::vector<TypeID> cache = get();
        return cache;
    }

    template <template <typename...> typename FilterType>
    static constexpr bool HasQueryFilter = (... || (IsInstanceOf<_QueryFilters, FilterType>::value && NumTypes<_QueryFilters>::value > 0));

    World& world;
};

template <typename _Params>
struct IsValidIterateParam {
    static constexpr bool value = (std::is_same_v<EntityID, _Params> && !std::is_pointer_v<_Params> && !std::is_reference_v<_Params>) || (!std::is_pointer_v<_Params> && std::is_class_v<std::remove_reference_t<_Params>>);
};

template <typename _ParamList>
    requires IsTypeList<_ParamList>
struct IsValidIterateParams;

template <template <typename...> typename _ParamList, typename... _Params>
struct IsValidIterateParams<_ParamList<_Params...>> {
    static constexpr bool value = (... && IsValidIterateParam<_Params>::value);
};

template <typename _Params>
struct IsWritableIterateParam {
    static constexpr bool value = std::is_reference_v<_Params> && !std::is_const_v<std::remove_reference_t<_Params>>;
};

}