#ifndef FOREIGNMETHOD_H_INCLUDED
#define FOREIGNMETHOD_H_INCLUDED

extern "C" {
    #include <wren.h>
}
#include <string>
#include <tuple>
#include <type_traits>
#include <functional>   // for std::hash
#include <utility>  // for index_sequence
#include <iostream>

namespace wrenly {
namespace detail {

// given a Wren method signature, this returns a unique value 
inline std::size_t HashMethodSignature(
                        const char* module,
                        const char* className,
                        bool isStatic,
                        const char* signature ) {
    std::hash<std::string> hash;
    std::string qualified( module );
    qualified += className;
    qualified += signature;
    if ( isStatic ) {
        qualified += 's';
    }
    return hash( qualified );
}

template< typename F >
struct FunctionTraits;

template< typename R, typename... Args >
struct FunctionTraits< R( Args... ) > {
    using ReturnType = R;
    
    constexpr static const std::size_t arity = sizeof...( Args );
    
    template< std::size_t N >
    struct Argument {
        static_assert( N < arity, "FunctionTraits error: invalid argument count parameter" );
        using type = std::tuple_element_t< N, std::tuple< Args... > >;
    };
    
    template< std::size_t N >
    using ArgumentType = typename Argument<N>::type;
};

template< typename R, typename... Args >
struct FunctionTraits< R(*)( Args... ) >: public FunctionTraits< R( Args... ) > {};

template< typename R, typename... Args > 
struct FunctionTraits< R(&)( Args... ) >: public FunctionTraits< R( Args... ) > {};

// member function pointer
template< typename C, typename R, typename... Args >
struct FunctionTraits< R(C::*)( Args... ) > : public FunctionTraits< R( Args... ) > {};
 
// const member function pointer
template< typename C, typename R, typename... Args >
struct FunctionTraits< R(C::*)( Args... ) const > : public FunctionTraits< R( Args... ) > {};
 
template< typename T >
T WrenGetArgument( WrenVM* vm, int index ) {}

template<>
inline float WrenGetArgument( WrenVM* vm, int index ) {
    return wrenGetArgumentDouble( vm, index );
}

template<>
inline double WrenGetArgument( WrenVM* vm, int index ) {
    return wrenGetArgumentDouble( vm, index );
}

template<>
inline int WrenGetArgument( WrenVM* vm, int index ) {
    return int( wrenGetArgumentDouble( vm, index ) );
}

template<>
inline unsigned WrenGetArgument( WrenVM* vm, int index ) {
    return unsigned( wrenGetArgumentDouble( vm, index ) );
}

template<>
inline bool WrenGetArgument( WrenVM* vm, int index ) {
    return wrenGetArgumentDouble( vm, index );
}

template<>
inline const char* WrenGetArgument( WrenVM* vm, int index ) {
    return wrenGetArgumentString( vm, index );
}

template<>
inline std::string WrenGetArgument( WrenVM* vm, int index ) {
    return std::string( wrenGetArgumentString( vm, index ) );
}

template< typename T >
void WrenReturn( WrenVM* vm, T val ) {}

template<>
inline void WrenReturn( WrenVM* vm, float val ) {
    wrenReturnDouble( vm, val );
}

template<>
inline void WrenReturn( WrenVM* vm, double val ) {
    wrenReturnDouble( vm, val );
}

template<>
inline void WrenReturn( WrenVM* vm, int val ) {
    wrenReturnDouble( vm, double( val ) );
}

template<>
inline void WrenReturn( WrenVM* vm, bool val ) {
    wrenReturnBool( vm, val );
}

template<>
inline void WrenReturn( WrenVM* vm, std::string val ) {
    wrenReturnString( vm, val.c_str(), -1 );
}

template< typename Function, std::size_t... index>
decltype( auto ) InvokeHelper( WrenVM* vm, Function&& f, std::index_sequence< index... > ) {
    using Traits = FunctionTraits< std::remove_reference_t<decltype(f)> >;
    return f( WrenGetArgument< typename Traits::template ArgumentType<index> >( vm, index + 1 )... );
}

template< typename Function >
decltype( auto ) InvokeWithWrenArguments( WrenVM* vm, Function&& f ) {
    constexpr auto Arity = FunctionTraits< std::remove_reference_t<decltype(f)> >::arity;
    return InvokeHelper< Function >( vm, std::forward<Function>( f ), std::make_index_sequence<Arity> {} );
}

template< typename R, typename C, typename... Args, std::size_t... index >
decltype( auto ) InvokeHelper( WrenVM* vm, R( C::*f )( Args... ), std::index_sequence< index... > ) {
    using Traits = FunctionTraits< decltype(f) >;
    C* c = static_cast<C*>( wrenGetArgumentForeign( vm, 0 ) );
    return (c->*f)( WrenGetArgument< typename Traits::template ArgumentType<index> >( vm, index + 1 )... );
}

template< typename R, typename C, typename... Args >
decltype( auto ) InvokeWithWrenArguments( WrenVM* vm, R( C::*f )( Args... ) ) {
    constexpr auto Arity = FunctionTraits< decltype(f) >::arity;
    return InvokeHelper( vm, f, std::make_index_sequence<Arity>{} );
}

// invokes plain InvokeWithWrenArguments if true
// invokes InvokeWithWrenArguments within WrenReturn if false
// to be used with std::is_void as the predicate
template<bool predicate>
struct InvokeWithoutReturningIf {
    
    template< typename Function >
    static void invoke( WrenVM* vm, Function&& f ) {
        InvokeWithWrenArguments( vm, std::forward<Function>( f ) );
    }
    
    template< typename R, typename C, typename... Args >
    static void invoke( WrenVM* vm, R( C::*f )( Args... ) ) {
        InvokeWithWrenArguments( vm, std::forward< R(C::*)( Args... ) >( f ) );
    }
};

template<>
struct InvokeWithoutReturningIf<false> {
    
    template< typename Function >
    static void invoke( WrenVM* vm, Function&& f ) {
        using ReturnType = typename FunctionTraits< std::remove_reference_t<decltype(f)> >::ReturnType;
        WrenReturn< ReturnType >( vm, InvokeWithWrenArguments( vm, std::forward<Function>( f ) ) );
    }
    
    template< typename R, typename C, typename... Args >
    static void invoke( WrenVM* vm, R ( C::*f )( Args... ) ) {
        WrenReturn< R >( vm, InvokeWithWrenArguments( vm, f ) );
    }
};

template< typename Signature, Signature >
struct ForeignMethodWrapper;

// free function variant
template< typename R, typename... Args, R( *f )( Args... ) >
struct ForeignMethodWrapper< R(*)( Args... ), f > {
    
    static void call( WrenVM* vm ) {
        InvokeWithoutReturningIf< std::is_void<R>::value >::invoke( vm, f );
    }
    
};

// method variant
template< typename R, typename C, typename... Args, R( C::*m )( Args... ) >
struct ForeignMethodWrapper< R( C::* )( Args... ), m > {
    
    static void call( WrenVM* vm ) {
        InvokeWithoutReturningIf< std::is_void<R>::value >::invoke( vm, m );
    }
    
};

}   // detail
}   // wrenly

#endif  // FOREIGNMETHOD_H_INCLUDED
