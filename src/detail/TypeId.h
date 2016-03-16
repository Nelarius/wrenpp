#ifndef TYPEID_H_INCLUDED
#define TYPEID_H_INCLUDED

#include <type_traits>
#include <cstdint>

namespace wrenpp {
namespace detail {

inline uint32_t& typeId() {
    static uint32_t id{ 0u };
    return id;
}

template<typename T>
uint32_t getTypeIdImpl() {
    static uint32_t id = typeId()++;
    return id;
}

template<typename T>
uint32_t getTypeId() {
    return getTypeIdImpl<std::decay_t<T>>();
}

}
}

#endif  // TYPEID_H_INCLUDED
