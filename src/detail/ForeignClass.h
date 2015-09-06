#ifndef FOREIGNCLASS_H_INCLUDED
#define FOREIGNCLASS_H_INCLUDED

#include "detail/ForeignMethod.h"
#include <type_traits>
#include <functional>   // for std::hash
#include <utility>  // for index_sequence
#include <iostream>

namespace wrenly {
namespace detail {

template< typename... Args >
struct ParameterPackTraits {
    
    constexpr static const std::size_t size = sizeof...( Args );
    
    template< std::size_t N >
    struct Parameter {
        static_assert( N < size, "ParameterPackTraits error: invalid parameter count" );
        using type = std::tuple_element_t< N, std::tuple< Args... > >;
    };
    
    template< std::size_t N >
    using ParameterType = typename Parameter< N >::type;
};

// given a Wren class signature, this returns a unique value 
inline std::size_t HashClassSignature( const char* module, const char* className ) {
    std::hash<std::string> hash;
    std::string qualified( module );
    qualified += className;
    return hash( qualified );
}

template< typename T, typename... Args, std::size_t... index >
void Construct( WrenVM* vm, void* memory, std::index_sequence<index...> ) {
    using Traits = ParameterPackTraits< Args... >;
    new ( memory ) T( WrenGetArgument< typename Traits::template ParameterType<index> >( vm, index )... );
}

template< typename T, typename... Args >
void Allocate( WrenVM* vm ) {
    // this is the function which handles the object construction
    // it should get arguments passed to the constructor
    void* memory = wrenAllocateForeign( vm, sizeof(T) );
    Construct< T, Args... >( vm, memory, std::make_index_sequence< ParameterPackTraits< Args... >::size > {} );
}

template< typename T >
void Finalize( WrenVM* vm ) {
    T* instance = static_cast<T*>( wrenGetArgumentForeign( vm, 0 ) );
    instance->~T();
}

}   // detail
}   // wrenly

#endif  // FOREIGNCLASS_H_INCLUDED