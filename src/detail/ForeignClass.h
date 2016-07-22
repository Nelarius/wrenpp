#ifndef FOREIGNCLASS_H_INCLUDED
#define FOREIGNCLASS_H_INCLUDED

#include "detail/ForeignMethod.h"
#include "detail/ForeignObject.h"
#include <cassert>
#include <cstdint>
#include <functional>   // for std::hash
#include <utility>  // for index_sequence
#include <type_traits>

namespace wrenpp {
namespace detail {

// given a Wren class signature, this returns a unique value 
inline std::size_t hashClassSignature( const char* module, const char* className ) {
    std::hash<std::string> hash;
    std::string qualified( module );
    qualified += className;
    return hash( qualified );
}

template< typename T, typename... Args, std::size_t... index >
void construct( WrenVM* vm, void* memory, std::index_sequence<index...> ) {
    using Traits = ParameterPackTraits< Args... >;
    ForeignObjectValue<T>* obj = new (memory) ForeignObjectValue<T>{};
    new (obj->objectPtr()) T{ WrenSlotAPI< typename Traits::template ParameterType<index> >::get(vm, index + 1)... };
}

template< typename T, typename... Args >
void allocate( WrenVM* vm ) {
    void* memory = wrenSetSlotNewForeign(vm, 0, 0, sizeof(ForeignObjectValue<T>));
    construct< T, Args... >( vm, memory, std::make_index_sequence< ParameterPackTraits< Args... >::size > {} );
}

template< typename T >
void finalize( void* bytes ) {
    // might be a foreign value OR ptr
    ForeignObject* objWrapper = static_cast<ForeignObject*>(bytes);
    objWrapper->~ForeignObject();
}

}
}

#endif  // FOREIGNCLASS_H_INCLUDED
