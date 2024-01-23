#pragma once

#include "Common.hpp"
#include <algorithm>
#include <type_traits>

namespace Flow {
template <typename TypeGroup>
class TypeIDGenerator final {
public:
    template <typename T>
    static TypeID Get()
    {
        static TypeID id = curIdx_++;
        return id;
    }

private:
    inline static TypeID curIdx_ = 0;
};

const TypeID GEntityComponentID = TypeIDGenerator<TypeGroup::Component>::Get<EntityID>();

template <typename T>
struct IsEntityIDType {
    static constexpr bool value = std::is_same_v<T, EntityID>;
};

template <typename T>
struct NotEntityIDType {
    static constexpr bool value = !std::is_same_v<T, EntityID>;
};

template <typename T, typename = std::enable_if<std::is_integral_v<T>>>
struct IDGenerator final {
public:
    static T Gen() { return curId_++; }

private:
    inline static T curId_ = {};
};

template <typename... _Ts>
struct TypeList {
};

template <typename _Instance, template <typename...> class _T>
struct IsInstanceOf : std::false_type { };

template <typename... _Args, template <typename...> class _T>
struct IsInstanceOf<_T<_Args...>, _T> : std::true_type { };

template <typename List>
constexpr bool IsTypeList = IsInstanceOf<List, TypeList> {};

template <typename _List>
struct NumTypes { };

template <template <typename...> typename _List, typename... _Ts>
struct NumTypes<_List<_Ts...>> {
    static constexpr int value = sizeof...(_Ts);
};

template <typename _TypeGroup, typename _List>
struct UnpakToTypeIDs { };

template <typename _TypeGroup, template <typename...> typename _List, typename... _Ts>
struct UnpakToTypeIDs<_TypeGroup, _List<_Ts...>> {

    static std::vector<TypeID> values()
    {
        auto get = []() {
            std::vector<TypeID> r = { (TypeIDGenerator<_TypeGroup>::template Get<std::remove_pointer_t<std::decay_t<_Ts>>>())... };

            std::ranges::sort(r);
            const auto ret = std::ranges::unique(r);
            r.erase(ret.begin(), ret.end());

            return std::move(r);
        };
        static auto cache = get();
        return cache;
    }
};

template <typename _TypeGroup, template <typename> class _Filter, typename _List>
struct UnpakToTypeIDsWithFilterWrapper;

template <typename _TypeGroup, template <typename> class _Filter, template <typename...> typename _List, typename _First, typename... _Remains>
struct UnpakToTypeIDsWithFilterWrapper<_TypeGroup, _Filter, _List<_First, _Remains...>> {
    static std::vector<TypeID> values()
    {
        auto get = []() {
            std::vector<TypeID> r;
            UnpakToTypeIDsWithFilter<_TypeGroup, _Filter>(_List<_First, _Remains...> {}, r);
            return r;
        };
        static auto cache = get();
        return cache;
    }
};

template <typename _TypeGroup, template <typename> class _Filter, template <typename...> typename _List, typename _First, typename... _Remains>
static void UnpakToTypeIDsWithFilter(_List<_First, _Remains...> ps, std::vector<TypeID>& outTypeIDs)
{
    if constexpr (_Filter<_First>::value) {
        outTypeIDs.push_back(TypeIDGenerator<_TypeGroup>::template Get<std::remove_pointer_t<std::decay_t<_First>>>());
    }

    if constexpr (sizeof...(_Remains) != 0) {
        using TS_Remains = _List<_Remains...>;
        UnpakToTypeIDsWithFilter<_TypeGroup, _Filter>(TS_Remains {}, outTypeIDs);
    } else {
        std::ranges::sort(outTypeIDs);
        const auto ret = std::ranges::unique(outTypeIDs);
        outTypeIDs.erase(ret.begin(), ret.end());
    }
}

template <typename _TypeGroup, template <typename> class _Filter1, template <typename> class _Filter2, typename _List>
struct UnpakToTypeIDsWith2FilterWrapper;

template <typename _TypeGroup, template <typename> class _Filter1, template <typename> class _Filter2, template <typename...> typename _List, typename _First, typename... _Remains>
struct UnpakToTypeIDsWith2FilterWrapper<_TypeGroup, _Filter1, _Filter2, _List<_First, _Remains...>> {
    static std::vector<TypeID> values()
    {
        auto get = []() {
            std::vector<TypeID> r;
            UnpakToTypeIDsWith2Filter<_TypeGroup, _Filter1, _Filter2>(_List<_First, _Remains...> {}, r);
            return r;
        };
        static auto cache = get();
        return cache;
    }
};

template <typename _TypeGroup, template <typename> class _Filter1, template <typename> class _Filter2, template <typename...> typename _List, typename _First, typename... _Remains>
static void UnpakToTypeIDsWith2Filter(_List<_First, _Remains...> ps, std::vector<TypeID>& outTypeIDs)
{
    if constexpr (_Filter1<_First>::value && _Filter2<_First>::value) {
        outTypeIDs.push_back(TypeIDGenerator<_TypeGroup>::template Get<std::remove_pointer_t<std::decay_t<_First>>>());
    }

    if constexpr (sizeof...(_Remains) != 0) {
        using TS_Remains = _List<_Remains...>;
        UnpakToTypeIDsWith2Filter<_TypeGroup, _Filter1, _Filter2>(TS_Remains {}, outTypeIDs);
    } else {
        std::ranges::sort(outTypeIDs);
        const auto ret = std::ranges::unique(outTypeIDs);
        outTypeIDs.erase(ret.begin(), ret.end());
    }
}

template <typename _T>
struct IsPure {
    static constexpr bool value = std::is_class_v<_T> && !std::is_pointer_v<_T> && !std::is_const_v<_T> && !std::is_reference_v<_T>;
};

}