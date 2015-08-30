
#pragma once

extern "C" {
    #include <wren.h>
}
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>  // for index_sequence

namespace wrenly {
namespace detail {
    
template< typename F >
struct FunctionTraits;

template< typename R, typename... Args >
struct FunctionTraits< R( Args... ) > {
    using ReturnType = R;
    
    constexpr static const uint32_t arity = sizeof...( Args );
    
    
};

template< typename R, typename... Args >
struct FunctionTraits< R(*)( Args... ) >: public FunctionTraits< R( Args... ) > {};

template< typename T >
T GetArgument( WrenVM* vm, int index ) {}

template<>
float GetArgument( WrenVM* vm, int index ) {
    return wrenGetArgumentDouble( vm, index );
}

template<>
double GetArgument( WrenVM* vm, int index ) {
    return wrenGetArgumentDouble( vm, index );
}

template<>
int GetArgument( WrenVM* vm, int index ) {
    return int( wrenGetArgumentDouble( vm, index ) );
}

template<>
unsigned GetArgument( WrenVM* vm, int index ) {
    return unsigned( wrenGetArgumentDouble( vm, index ) );
}

template<>
bool GetArgument( WrenVM* vm, int index ) {
    return wrenGetArgumentDouble( vm, index );
}

template<>
const char* GetArgument( WrenVM* vm, int index ) {
    return wrenGetArgumentString( vm, index );
}

template<>
std::string GetArgument( WrenVM* vm, int index ) {
    return std::string( wrenGetArgumentString( vm, index ) );
}

template< typename T >
void Return( Wrenvm* vm, T val ) {}

template<>
void Return( WrenVM* vm, float val ) {
    wrenReturnDouble( vm, val );
}

template<>
void Return( WrenVM* vm, double val ) {
    wrenReturnDouble( vm, val );
}

template< typename Func, typename Tuple, std::size_t... index>
decltype( auto ) InvokeWithSequence( Func&& f, Tuple&& tup, std::index_sequence<index...> ) {
    return f( std::get<index>( std::forward<Tup>( tup ) )... );
}

template< typename Func, typename Tuple >
decltype( auto ) Invoke( Func&& f, Tuple&& tup ) {
    constexpr auto Arity = FunctionTraits<decltype(f)>::arity;
    return InvokeWithSequence(
        std::forward<Func>( f ),
        std::forward<Tuple>( tup ),
        std::make_index_sequence<Arity>{}
    );
}

template<typename F, F f>
void ForeignMethodWrapper( WrenVM* vm ) {
    // tuple has to be built here
    //Return( vm, Invoke( f, tup ) );
}

}   // detail
}   // wrenly
