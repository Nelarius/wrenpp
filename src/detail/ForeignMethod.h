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
#include <cstdlib>

namespace wrenpp {
namespace detail {

// given a Wren method signature, this returns a unique value 
inline std::size_t hashMethodSignature(
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
        static_assert( N < Arity, "FunctionTraits error: invalid argument index parameter" );
        using Type = std::tuple_element_t< N, std::tuple< Args... > >;
    };

    template< std::size_t N >
    using ArgumentType = typename Argument<N>::Type;
};

template< typename... Args >
struct ParameterPackTraits {

    constexpr static const std::size_t size = sizeof...(Args);

    template< std::size_t N >
    struct Parameter {
        static_assert(N < size, "ParameterPackTraits error: invalid parameter index");
        using Type = std::tuple_element_t< N, std::tuple< Args... > >;
    };

    template< std::size_t N >
    using ParameterType = typename Parameter< N >::Type;
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
    static T get( WrenVM* vm, int slot ) {
        ForeignObject* obj = static_cast<ForeignObject*>(wrenGetSlotForeign(vm, slot));
        return *static_cast< T* >(obj->objectPtr());
    }
};

template < typename T >
struct WrenArgument< T& > {
    static T& get( WrenVM* vm, int slot ) {
        ForeignObject* obj = static_cast<ForeignObject*>(wrenGetSlotForeign( vm, slot ));
        return *static_cast< T* >(obj->objectPtr());
    }
};

template< typename T >
struct WrenArgument< const T& > {
    static const T& get( WrenVM* vm, int slot ) {
        ForeignObject* obj = static_cast<ForeignObject*>(wrenGetSlotForeign( vm, slot ));
        return *static_cast< T* >(obj->objectPtr());
    }
};

template< typename T >
struct WrenArgument< T* > {
    static T* get(WrenVM* vm, int slot) {
        ForeignObject* obj = static_cast<ForeignObject*>(wrenGetSlotForeign(vm, slot));
        return static_cast<T*>(obj->objectPtr());
    }
};

template< typename T >
struct WrenArgument< const T* > {
    static const T* get( WrenVM* vm, int slot ) {
        ForeignObject* obj = static_cast<ForeignObject*>(wrenGetSlotForeign( vm, slot ));
        return static_cast< const T* >(obj->objectPtr());
    }
};

template<>
struct WrenArgument< float > {
    static float get( WrenVM* vm, int slot ) {
        return float(wrenGetSlotDouble( vm, slot ));
    }

    static void set(WrenVM* vm, int slot, float val) {
        wrenSetSlotDouble( vm, slot, double(val) );
    }
};

template<>
struct WrenArgument< double > {
    static double get( WrenVM* vm, int slot ) {
        return wrenGetSlotDouble( vm, slot );
    }

    static void set(WrenVM* vm, int slot, double val ) {
        wrenSetSlotDouble( vm, slot, val );
    }
};

template<>
struct WrenArgument< int > {
    static int get( WrenVM* vm, int slot ) {
        return int(wrenGetSlotDouble( vm, slot ));
    }

    static void set( WrenVM* vm, int slot, int val ) {
        wrenSetSlotDouble( vm, slot, double(val) );
    }
};

template<>
struct WrenArgument< unsigned > {
    static unsigned get( WrenVM* vm, int slot ) {
        return unsigned(wrenGetSlotDouble( vm, slot ));
    }

    static void set( WrenVM* vm, int slot, unsigned val ) {
        wrenSetSlotDouble( vm, slot, double(val) );
    }
};

template<>
struct WrenArgument< bool > {
    static bool get( WrenVM* vm, int slot ) {
        return wrenGetSlotBool( vm, slot );
    }

    static void set( WrenVM* vm, int slot, bool val ) {
        wrenSetSlotBool( vm, slot, val );
    }
};

template<>
struct WrenArgument< const char* > {
    static const char* get( WrenVM* vm, int slot ) {
        return wrenGetSlotString( vm, slot );
    }

    static void set( WrenVM* vm, int slot, const char* val ) {
        wrenSetSlotString( vm, slot, val );
    }
};

template<>
struct WrenArgument< std::string > {
    static std::string get( WrenVM* vm, int slot ) {
        return std::string( wrenGetSlotString( vm, slot ) );
    }

    static void set( WrenVM* vm, int slot, const std::string& str ) {
        wrenSetSlotString( vm, slot, str.c_str() );
    }
};

struct ExpandType {
    template< typename... T >
    ExpandType( T&&... ) {}
};

// a helper for passing arguments to Wren
// explained here:
// http://stackoverflow.com/questions/17339789/how-to-call-a-function-on-all-variadic-template-args
template<typename... Args, std::size_t... index>
void passArgumentsToWren( WrenVM* vm, const std::tuple<Args...>& tuple, std::index_sequence< index... > ) {
    using Traits = ParameterPackTraits<Args...>;
    ExpandType{
        0,
        (WrenArgument<typename Traits::template ParameterType<index>>::set(
            vm,
            index + 1,
            std::get<index>(tuple)
        ), 0)...
    };
}

template< typename T >
struct WrenReturnValue {
    static void set(WrenVM* vm, T val) {
        ForeignObjectValue<T>::setInSlot<0>(vm, val);
    }

    static T& get(WrenVM* vm) {
        return *getSlotForeign<T, 0>(vm);
    }
};

template<typename T>
struct WrenReturnValue<T&> {
    static void set(WrenVM* vm, T& ref) {
        ForeignObjectPtr<T>::setInSlot<0>(vm, &ref);
    }
};

template<typename T>
struct WrenReturnValue<const T&> {
    static void set(WrenVM* vm, const T& ref) {
        ForeignObjectPtr<T>::setInSlot<0>(vm, const_cast<T*>(&ref));
    }
};

template<typename T>
struct WrenReturnValue<T*> {
    static void set(WrenVM* vm, T* ptr) {
        ForeignObjectPtr<T>::setInSlot<0>(vm, ptr);
    }
};

template<typename T>
struct WrenReturnValue<const T*> {
    static void set(WrenVM* vm, const T* ptr) {
        ForeignObjectPtr<T>::setInSlot<0>(vm, const_cast<T*>(ptr));
    }
};

template<>
struct WrenReturnValue< float > {
    static void set( WrenVM* vm, float val ) {
        wrenSetSlotDouble( vm, 0, double ( val ) );
    }

    static float get(WrenVM* vm) {
        return float(wrenGetSlotDouble(vm, 0));
    }
};

template<>
struct WrenReturnValue< float& > {
    static void set( WrenVM* vm, float val ) {
        wrenSetSlotDouble( vm, 0, double( val ) );
    }
};

template<>
struct WrenReturnValue< const float& > {
    static void set( WrenVM* vm, float val ) {
        wrenSetSlotDouble( vm, 0, double( val ) );
    }
};

template<>
struct WrenReturnValue< double > {
    static void set( WrenVM* vm, double val ) {
        wrenSetSlotDouble( vm, 0, val );
    }

    static double get(WrenVM* vm) {
        return wrenGetSlotDouble(vm, 0);
    }
};

template<>
struct WrenReturnValue< double& > {
    static void set(WrenVM* vm, double val) {
        wrenSetSlotDouble(vm, 0, val);
    }
};

template<>
struct WrenReturnValue< const double& > {
    static void set( WrenVM* vm, double val ) {
        wrenSetSlotDouble( vm, 0, val );
    }
};

template<>
struct WrenReturnValue< int > {
    static void set( WrenVM* vm, int val ) {
        wrenSetSlotDouble( vm, 0, double ( val ) );
    }

    static int get(WrenVM* vm) {
        return int(wrenGetSlotDouble(vm, 0));
    }
};

template<>
struct WrenReturnValue< int& > {
    static void set( WrenVM* vm, int val ) {
        wrenSetSlotDouble( vm, 0, double ( val ) );
    }
};

template<>
struct WrenReturnValue< const int& > {
    static void set( WrenVM* vm, int val ) {
        wrenSetSlotDouble( vm, 0, double ( val ) );
    }
};

template<>
struct WrenReturnValue< std::size_t > {
    static void set( WrenVM* vm, std::size_t val ) {
        wrenSetSlotDouble( vm, 0, double( val ) );
    }

    static std::size_t get(WrenVM* vm) {
        return std::size_t(wrenGetSlotDouble(vm, 0));
    }
};

template<>
struct WrenReturnValue< std::size_t& > {
    static void ret( WrenVM* vm, std::size_t val ) {
        wrenSetSlotDouble( vm, 0, double( val ) );
    }
};

template<>
struct WrenReturnValue< const std::size_t& > {
    static void set( WrenVM* vm, std::size_t val ) {
        wrenSetSlotDouble( vm, 0, double( val ) );
    }
};

template<>
struct WrenReturnValue< bool > {
    static void set( WrenVM* vm, bool val ) {
        wrenSetSlotBool( vm, 0, val );
    }

    static bool get(WrenVM* vm) {
        return wrenGetSlotBool(vm, 0);
    }
};

template<>
struct WrenReturnValue< std::string > {
    static void set( WrenVM* vm, std::string val ) {
        wrenSetSlotString( vm, 0, val.c_str() );
    }
};

template<>
struct WrenReturnValue< const char* > {
    static void set( WrenVM* vm, const char* val ) {
        wrenSetSlotString( vm, 0, val );
    }

    static const char* get(WrenVM* vm) {
        return wrenGetSlotString(vm, 0);
    }
};

template< typename Function, std::size_t... index>
decltype( auto ) invokeHelper( WrenVM* vm, Function&& f, std::index_sequence< index... > ) {
    using Traits = FunctionTraits< std::remove_reference_t<decltype(f)> >;
    return f( WrenArgument< typename Traits::template ArgumentType<index> >::get( vm, index + 1 )... );
}

template< typename Function >
decltype( auto ) invokeWithWrenArguments( WrenVM* vm, Function&& f ) {
    constexpr auto Arity = FunctionTraits< std::remove_reference_t<decltype(f)> >::Arity;
    return invokeHelper< Function >( vm, std::forward<Function>( f ), std::make_index_sequence<Arity> {} );
}

template< typename R, typename C, typename... Args, std::size_t... index >
decltype( auto ) invokeHelper( WrenVM* vm, R( C::*f )( Args... ), std::index_sequence< index... > ) {
    using Traits = FunctionTraits< decltype(f) >;
    ForeignObject* objWrapper = static_cast<ForeignObject*>(wrenGetSlotForeign(vm, 0));
    C* obj = static_cast<C*>(objWrapper->objectPtr());
    return (obj->*f)( WrenArgument< typename Traits::template ArgumentType<index> >::get( vm, index + 1 )... );
}

// const variant
template< typename R, typename C, typename... Args, std::size_t... index >
decltype( auto ) invokeHelper( WrenVM* vm, R( C::*f )( Args... ) const, std::index_sequence< index... > ) {
    using Traits = FunctionTraits< decltype(f) >;
    ForeignObject* objWrapper = static_cast<ForeignObject*>(wrenGetSlotForeign(vm, 0));
    const C* obj = static_cast<const C*>(objWrapper->objectPtr());
    return (obj->*f)( WrenArgument< typename Traits::template ArgumentType<index> >::get( vm, index + 1 )... );
}

template< typename R, typename C, typename... Args >
decltype( auto ) invokeWithWrenArguments( WrenVM* vm, R( C::*f )( Args... ) ) {
    constexpr auto Arity = FunctionTraits< decltype(f) >::Arity;
    return invokeHelper( vm, f, std::make_index_sequence<Arity>{} );
}

// const variant
template< typename R, typename C, typename... Args >
decltype( auto ) invokeWithWrenArguments( WrenVM* vm, R( C::*f )( Args... ) const ) {
    constexpr auto Arity = FunctionTraits< decltype(f) >::Arity;
    return invokeHelper( vm, f, std::make_index_sequence<Arity>{} );
}

// invokes plain invokeWithWrenArguments if true
// invokes invokeWithWrenArguments within WrenReturn if false
// to be used with std::is_void as the predicate
template<bool predicate>
struct InvokeWithoutReturningIf {

    template< typename Function >
    static void invoke( WrenVM* vm, Function&& f ) {
        invokeWithWrenArguments( vm, std::forward<Function>( f ) );
    }

    template< typename R, typename C, typename... Args >
    static void invoke( WrenVM* vm, R( C::*f )( Args... ) ) {
        invokeWithWrenArguments( vm, std::forward< R(C::*)( Args... ) >( f ) );
    }
};

template<>
struct InvokeWithoutReturningIf<false> {

    template< typename Function >
    static void invoke( WrenVM* vm, Function&& f ) {
        using ReturnType = typename FunctionTraits< std::remove_reference_t<decltype(f)> >::ReturnType;
        WrenReturnValue< ReturnType >::set( vm, invokeWithWrenArguments( vm, std::forward<Function>( f ) ) );
    }

    template< typename R, typename C, typename... Args >
    static void invoke( WrenVM* vm, R ( C::*f )( Args... ) ) {
        WrenReturnValue< R >::set( vm, invokeWithWrenArguments( vm, f ) );
    }

    template< typename R, typename C, typename... Args >
    static void invoke( WrenVM* vm, R ( C::*f )( Args... ) const ) {
        WrenReturnValue< R >::set( vm, invokeWithWrenArguments( vm, f ) );
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

// const method variant
template< typename R, typename C, typename... Args, R( C::*m )( Args... ) const >
struct ForeignMethodWrapper< R( C::* )( Args... ) const, m > {

    static void call( WrenVM* vm ) {
        InvokeWithoutReturningIf< std::is_void<R>::value >::invoke( vm, m );
    }

};

}   // detail
}   // wrenly

#endif  // FOREIGNMETHOD_H_INCLUDED
