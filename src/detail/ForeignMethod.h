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

    constexpr static const std::size_t Arity = sizeof...( Args );

    template< std::size_t N >
    struct Argument {
        static_assert( N < Arity, "FunctionTraits error: invalid argument count parameter" );
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
struct WrenArgument {
    static T get( WrenVM* vm, int index ) {
        return *static_cast< T* >( wrenGetArgumentForeign( vm, index ) );
    }
};

template < typename T >
struct WrenArgument< T& > {
    static T& get( WrenVM* vm, int index ) {
        return *static_cast< T* >( wrenGetArgumentForeign( vm, index ) );
    }
};

template< typename T >
struct WrenArgument< const T& > {
    static const T& get( WrenVM* vm, int index ) {
        return *static_cast< T* >( wrenGetArgumentForeign( vm, index ) );
    }
};

template< typename T >
struct WrenArgument< T* > {
    static T* get( WrenVM* vm, int index ) {
        return static_cast< T* >( wrenGetArgumentForeign( vm, index ) );
    }
};

template< typename T >
struct WrenArgument< const T* > {
    static const T* get( WrenVM* vm, int index ) {
        return static_cast< const T* >( wrenGetArgumentForeign( vm, index ) );
    }
};

template<>
struct WrenArgument< float > {
    static float get( WrenVM* vm, int index ) {
        return wrenGetArgumentDouble( vm, index );
    }
};

template<>
struct WrenArgument< double > {
    static double get( WrenVM* vm, int index ) {
        return wrenGetArgumentDouble( vm, index );
    }
};

template<>
struct WrenArgument< int > {
    static int get( WrenVM* vm, int index ) {
        return wrenGetArgumentDouble( vm, index );
    }
};

template<>
struct WrenArgument< unsigned > {
    static unsigned get( WrenVM* vm, int index ) {
        return wrenGetArgumentDouble( vm, index );
    }
};

template<>
struct WrenArgument< bool > {
    static bool get( WrenVM* vm, int index ) {
        return wrenGetArgumentBool( vm, index );
    }
};

template<>
struct WrenArgument< const char* > {
    static const char* get( WrenVM* vm, int index ) {
        return wrenGetArgumentString( vm, index );
    }
};

template<>
struct WrenArgument< std::string > {
    static std::string get( WrenVM* vm, int index ) {
        return std::string( wrenGetArgumentString( vm, index ) );
    }
};

template< typename T >
struct WrenReturnValue;

template<>
struct WrenReturnValue< float > {
    static void ret( WrenVM* vm, float val ) {
        wrenReturnDouble( vm, double ( val ) );
    }
};

template<>
struct WrenReturnValue< double > {
    static void ret( WrenVM* vm, double val ) {
        wrenReturnDouble( vm, val );
    }
};

template<>
struct WrenReturnValue< int > {
    static void ret( WrenVM* vm, int val ) {
        wrenReturnDouble( vm, double ( val ) );
    }
};

template<>
struct WrenReturnValue< bool > {
    static void ret( WrenVM* vm, bool val ) {
        wrenReturnBool( vm, val );
    }
};

template<>
struct WrenReturnValue< std::string > {
    static void ret( WrenVM* vm, std::string val ) {
        wrenReturnString( vm, val.c_str(), -1 );
    }
};

template< typename Function, std::size_t... index>
decltype( auto ) InvokeHelper( WrenVM* vm, Function&& f, std::index_sequence< index... > ) {
    using Traits = FunctionTraits< std::remove_reference_t<decltype(f)> >;
    return f( WrenArgument< typename Traits::template ArgumentType<index> >::get( vm, index + 1 )... );
}

template< typename Function >
decltype( auto ) InvokeWithWrenArguments( WrenVM* vm, Function&& f ) {
    constexpr auto Arity = FunctionTraits< std::remove_reference_t<decltype(f)> >::Arity;
    return InvokeHelper< Function >( vm, std::forward<Function>( f ), std::make_index_sequence<Arity> {} );
}

template< typename R, typename C, typename... Args, std::size_t... index >
decltype( auto ) InvokeHelper( WrenVM* vm, R( C::*f )( Args... ), std::index_sequence< index... > ) {
    using Traits = FunctionTraits< decltype(f) >;
    C* c = static_cast<C*>( wrenGetArgumentForeign( vm, 0 ) );
    return (c->*f)( WrenArgument< typename Traits::template ArgumentType<index> >::get( vm, index + 1 )... );
}

template< typename R, typename C, typename... Args >
decltype( auto ) InvokeWithWrenArguments( WrenVM* vm, R( C::*f )( Args... ) ) {
    constexpr auto Arity = FunctionTraits< decltype(f) >::Arity;
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
        WrenReturnValue< ReturnType >::ret( vm, InvokeWithWrenArguments( vm, std::forward<Function>( f ) ) );
    }

    template< typename R, typename C, typename... Args >
    static void invoke( WrenVM* vm, R ( C::*f )( Args... ) ) {
        WrenReturnValue< R >::ret( vm, InvokeWithWrenArguments( vm, f ) );
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