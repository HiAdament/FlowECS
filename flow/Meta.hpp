#pragma once

#include "Common.hpp"
#include "city.h"
#include <unordered_map>

namespace Flow {

struct MetaInfo {
    uint64 hash;
    uint16 size;
    uint16 align;
    const char* name { "none" };
};

template <typename _T>
struct MetaInfoBuilder {

    static constexpr const char* name_detail()
    {
        return __PRETTY_FUNCTION__;
    }

    static constexpr MetaInfo Build()
    {
        MetaInfo meta;
        meta.name = name_detail();
        meta.hash = CityHash64(meta.name, strlen(meta.name));
        meta.align = std::is_empty_v<_T> ? 0 : alignof(_T);
        meta.size = std::is_empty_v<_T> ? 0 : sizeof(_T);

        return meta;
    }
};

static std::unordered_map<uint64, MetaInfo> typeMetaInfos;

template <typename _List>
struct UnpakToTypeMetaInfo { };

template <template <typename...> typename _List, typename... _Ts>
struct UnpakToTypeMetaInfo<_List<_Ts...>> {

    static std::array<MetaInfo*, sizeof...(_Ts)> values()
    {
        std::array<MetaInfo, sizeof...(_Ts)> metaInfos = { (MetaInfoBuilder<_Ts>::Build())... };
        std::array<MetaInfo*, sizeof...(_Ts)> result;

        for (size_t i = 0; i < sizeof...(_Ts); ++i) {
            MetaInfo& meta = metaInfos[i];
            auto res = typeMetaInfos.find(meta.hash);
            if (res == typeMetaInfos.end()) {
                typeMetaInfos.emplace(meta.hash, meta);
            }
            result[i] = &typeMetaInfos[meta.hash];
        }

        return result;
    }
};

}