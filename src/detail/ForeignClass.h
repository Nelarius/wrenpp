#ifndef FOREIGNCLASS_H_INCLUDED
#define FOREIGNCLASS_H_INCLUDED

#include "detail/ForeignMethod.h"
#include <type_traits>
#include <functional>   // for std::hash
#include <utility>  // for index_sequence
#include <iostream>

namespace wrenly {
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
    new ( memory ) T{ WrenArgument< typename Traits::template ParameterType<index> >::get( vm, index + 1 )... };
}

template< typename T, typename... Args >
void allocate( WrenVM* vm ) {
    // this is the function which handles the object construction
    // it should get arguments passed to the constructor
    void* memory = wrenSetSlotNewForeign( vm, 0, 0, sizeof(T) );
    construct< T, Args... >( vm, memory, std::make_index_sequence< ParameterPackTraits< Args... >::size > {} );
}

template< typename T >
void finalize( void* bytes ) {
    T* instance = static_cast<T*>( bytes );
    instance->~T();
}

}   // detail
}   // wrenly

#endif  // FOREIGNCLASS_H_INCLUDED
