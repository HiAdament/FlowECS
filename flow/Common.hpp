#pragma once
#include "city.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace Flow {
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using byte = uint8;
using wchar = wchar_t;

constexpr size_t SIZE_K = 1024;
constexpr size_t BLOCK_MEMORY_4K = SIZE_K * 4;
constexpr size_t BLOCK_MEMORY_8K = SIZE_K * 8;
constexpr size_t BLOCK_MEMORY_16K = SIZE_K * 16;
constexpr size_t BLOCK_MEMORY_32K = SIZE_K * 32;
constexpr size_t BLOCK_MEMORY_64K = SIZE_K * 64;
constexpr uint32 InvalidValue = -1;

struct World;
struct Component;
struct Archetype;
struct ArchetypeBuilder;
struct DataChunk;
struct DataChunkHeader;
struct EntityStorageInfo;
struct CopyComponentCommand;
struct MetaInfo;
struct SystemData;
struct EntityLocation;

// used for runtime typeid generate
namespace TypeGroup {
    struct Query { };
    struct Component { };
    struct System { };
}

template <typename _TypeName>
struct TypeIDBase {
    using _My = TypeIDBase<_TypeName>;

    static constexpr uint32 Invalid = -1;

    uint32 value;

    TypeIDBase()
        : value(0)
    {
    }
    TypeIDBase(const TypeIDBase& o) noexcept
        : value(o.value)
    {
    }

    TypeIDBase(uint32 v) noexcept
        : value(v)
    {
    }

    // don't move explicit!
    explicit operator uint32() noexcept { return value; }

    // prefix increment
    _My& operator++() noexcept
    {
        ++value;
        return *this;
    }

    // postfix increment
    _My operator++(int) noexcept
    {
        _My old = *this;
        ++value;
        return old;
    }

    // prefix decrement
    _My& operator--() noexcept
    {
        --value;
        return *this;
    }

    // postfix decrement
    _My operator--(int) noexcept
    {
        _My old = *this; // copy old value
        --value;
        return old;
    }

    auto operator<=>(const _My&) const = default;

    friend inline bool operator==(const _My& lhs, const _My& rhs) { return lhs.value == rhs.value; }
    friend inline bool operator!=(const _My& lhs, const _My& rhs) { return lhs.value != rhs.value; }
    friend inline bool operator<(const _My& lhs, const _My& rhs) { return lhs.value < rhs.value; }
    friend inline bool operator>(const _My& lhs, const _My& rhs) { return lhs.value > rhs.value; }
    friend inline bool operator<=(const _My& lhs, const _My& rhs) { return lhs.value <= rhs.value; }
    friend inline bool operator>=(const _My& lhs, const _My& rhs) { return lhs.value >= rhs.value; }

    _My& operator=(_My other)
    {
        value = other.value;
        return *this;
    }

    _My& operator=(const uint32& b)
    {
        value = b;
        return *this;
    }
};

// template <typename _TypeName>
// struct TypeIDBase {

namespace detail {
    // help struct to gen TypeID type
    struct _EntityID { };
    struct _ArchetypeID { };
    struct _QueryID { };
    struct _ComponentID { };
    struct _SystemID { };
}

using EntityID = TypeIDBase<detail::_EntityID>;
using ArchetypeID = TypeIDBase<detail::_ArchetypeID>;
using QueryID = TypeIDBase<detail::_QueryID>;
using ComponentID = TypeIDBase<detail::_ComponentID>;
using SystemID = TypeIDBase<detail::_SystemID>;
using Generation = uint32;
using TypeID = uint32_t;

}

#define TYPE_ID_HASH_FUNCTAION(Type)                          \
    template <>                                               \
    struct std::hash<Type> {                                  \
        uint64 operator()(const Type& s) const noexcept       \
        {                                                     \
            static_assert(sizeof(Type) < 16, "");             \
            return CityHash64((const char*)&s, sizeof(Type)); \
        }                                                     \
    };

TYPE_ID_HASH_FUNCTAION(Flow::EntityID)
TYPE_ID_HASH_FUNCTAION(Flow::ArchetypeID)
TYPE_ID_HASH_FUNCTAION(Flow::QueryID)
TYPE_ID_HASH_FUNCTAION(Flow::ComponentID)

#define ONLY_RUN_ONCE                                                      \
    static std::atomic_bool __the_following_code_runs_only_once { false }; \
    if (__the_following_code_runs_only_once.exchange(true)) {              \
        return;                                                            \
    }
