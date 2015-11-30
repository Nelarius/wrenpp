#ifndef WRENLY_H_INCLUDED
#define WRENLY_H_INCLUDED

#include "detail/ForeignMethod.h"
#include "detail/ForeignClass.h"
#include "detail/ForeignProperty.h"
#include "detail/ArgumentListString.h"
extern "C" {
    #include <wren.h>
}
#include <sstream>
#include <string>
#include <vector>
#include <functional>   // for std::hash
#include <cstdint>
#include <cstdlib>      // for std::size_t
#include <unordered_map>
#include <set>

namespace wrenly {

// this is just void (*FunctionPtr)(void)
using FunctionPtr = WrenForeignMethodFn;
using LoadModuleFn = std::function< char*( const char* ) >;
using WriteFn = std::function< void( WrenVM*, const char* ) >;

namespace detail {
    void registerFunction(
        const std::string& mod, const std::string& clss, bool isStatic,
        std::string sig, FunctionPtr function
    );
    void registerClass(
        const std::string& mod, std::string clss, WrenForeignClassMethods methods
    );
}

/**
 * @class Value
 * @date 11/07/15
 * @brief A reference-counted RAII wrapper for a WrenValue.
 * Use this to obtain references to object instances living in Wren. The contained object
 * will not be garbage collected while held in an instance of this wrapper.
 */
class Value {
    public:
        Value( WrenVM*, WrenValue* );
        Value() = delete;
        Value( const Value& );
        Value( Value&& );
        Value& operator=( const Value& )    = delete;
        Value& operator=( Value&& )         = delete;
        Value& operator=( Value );
        ~Value();

        bool isNull() const;

        WrenValue* pointer();

    private:
        void retain_();
        void release_();

        WrenVM*     vm_;    // this pointer is not managed here
        WrenValue*  value_;
        unsigned*   refCount_;
};

/**
 * @class Method
 * @author Muszynski Johann M
 * @date 13/08/15
 * @file Wrenly.h
 * @brief Use this to call a wren method from C++.
 * Make sure that the Wren instance which dispensed this object does not go out of scope
 * before this instance does. A segfault will occur otherwise.
 */
class Method {
    public:
        Method( WrenVM*, WrenValue* );
        Method() = delete;
        Method( const Method& );
        Method( Method&& );
        Method& operator=( const Method& )  = delete;
        Method& operator=( Method&& )       = delete;
        Method& operator=( Method );
        ~Method();

        /**
        * @brief Call a wren method and pass it any arguments
        * This method is const, because it is intentended to be pass around
        * like immutable data. In reality, the underlying virtual machine might 
        * change its state, depending on the Wren method being called.
        */
        template<typename... Args>
        Value operator()( Args&&... args ) const;

    private:
        void retain_();
        void release_();

        mutable WrenVM*         vm_;    // this pointer is not managed here
        mutable WrenValue*      method_;
        unsigned*               refCount_;
};

class Wren;
class ModuleContext;

/**
 * @class ClassContext
 * @author Muszynski Johann M
 * @date 31/08/15
 * @file Wrenly.h
 * @brief Stores the class and module name. Gives easier access to function registration.
 */
class ClassContext {
    public:
        ClassContext() = delete;
        ClassContext( std::string c, ModuleContext* mod );
        virtual ~ClassContext() = default;

        /**
         * @brief Register a free function with Wrenly.
         * @param isStatic true, if the wrapping method is static, false otherwise
         * @param signature The Wren signature of the function. Include parenthesized argument list, with an underscore in place of each argument, or no parenthesis if the method doesn't contain any.
         */
        template< typename F, F f >
        ClassContext& bindFunction( bool isStatic, std::string signature );
        /**
         * @brief Register a function without any automatic wrapping taking place.
         * This function takes as its only parameter a pointer to the virtual machine
         * instance. You can use Wren's own API for interfacing with the VM instance.
         */
        ClassContext& bindCFunction( bool isStatic, std::string signature, FunctionPtr function );

        ModuleContext& endClass();

    protected:
        ModuleContext*  module_;
        std::string     class_;
};

template< typename T >
class RegisteredClassContext: public ClassContext {
    public:
        RegisteredClassContext( std::string c, ModuleContext* mod )
        :   ClassContext( c, mod )
            {}
        virtual ~RegisteredClassContext() = default;

        template< typename F, F f >
        RegisteredClassContext& bindMethod( bool isStatic, std::string signature );
        template< typename U, U T::*Field >
        RegisteredClassContext& bindGetter( bool isStatic, std::string signature );
        template< typename U, U T::*Field >
        RegisteredClassContext& bindSetter( bool isStatic, std::string signature );
        RegisteredClassContext& bindCFunction( bool isStatic, std::string signature, FunctionPtr function );
};

/**
 * @class ModuleContext
 * @author Muszynski Johann M
 * @date 31/08/15
 * @file Wrenly.h
 * @brief Stores the module name. Gives access to class context.
 */
class ModuleContext {
    public:
        ModuleContext() = delete;
        ModuleContext( std::string mod );

        /**
         * @brief Begin a class context.
         * @param className The class name.
         * @return 
         */
        ClassContext beginClass( std::string className );

        /**
         * @brief Register a C++ class with Wren.
         * @param className The name of the class in Wren.
         * @tparam T The C++ class type.
         * @tparam Args... The constructor argument types
         * @return The class context for the registered type.
         */
        template< typename T, typename... Args >
        RegisteredClassContext<T> bindClass( std::string className );

        void endModule();

    private:
        friend class ClassContext;
        template< typename T > friend class RegisteredClassContext;

        std::string name_;
};

ModuleContext beginModule( std::string mod );

/**
 * @class Wren
 * @author Nelarius
 * @date 09/08/2015
 * @file Wren.h
 * @brief Holds an instance of the Wren virtual machine.
 * The Wren virtual machine is held internally as a pointer. This class
 * owns the pointer uniquely. The contents of this class may be moved, 
 * but not copied.
 */
class Wren {

    public:
        Wren();
        Wren( const Wren& )             = delete;
        Wren( Wren&& );
        Wren& operator=( const Wren& )  = delete;
        Wren& operator=( Wren&& );
        ~Wren();

        WrenVM* vm();

        void executeModule( const std::string& );
        void executeString( const std::string& );

        /**
         * @brief Run garbage collection immediately.
         */
        void collectGarbage();

        /**
         * @brief Begin a module context.
         * @param mod The name of the module. The name is "main" if not in an imported module.
         * @return 
         */
        //ModuleContext beginModule( std::string mod );

        /**
         * @brief Get a callable Wren method.
         * @param module The name of the module that the method is located in. "main" if not in an imported module.
         * @param variable The name of the variable the method is attached to. The class name if static.
         * @param signature The name of the method, followed by a parenthesis enclosed list of of underscores representing each argument.
         * @return The callable method object.
         * As long as this object exists, the variable that the method is attached to in Wren will not be
         * garbage collected.
         */
        Method method(
            const std::string& module,
            const std::string& variable,
            const std::string& signature
        );

        static LoadModuleFn loadModuleFn;
        static WriteFn writeFn;

    private:
        friend class ModuleContext;
        friend class ClassContext;
        template< typename T > friend class RegisteredClassContext;

        WrenVM*	    vm_;
};

template< typename... Args >
Value Method::operator()( Args&&... args ) const {
    constexpr const std::size_t Arity = sizeof...( Args );
    detail::ArgumentListString< Arity > arguments{ args... };
    WrenValue* result{ nullptr };
    wrenCall( vm_, method_, &result, arguments.getString(), std::forward<Args>( args )... );

    return Value( vm_, result );
}

template< typename T, typename... Args >
RegisteredClassContext<T> ModuleContext::bindClass( std::string c ) {
    WrenForeignClassMethods wrapper{ &detail::allocate< T, Args... >, &detail::finalize< T > };
    detail::registerClass( name_, c, wrapper );
    return RegisteredClassContext<T>( c, this );
}

template< typename F, F f >
ClassContext& ClassContext::bindFunction( bool isStatic, std::string s ) {
    detail::registerFunction(
        module_->name_,
        class_, isStatic,
        s,
        detail::ForeignMethodWrapper< decltype(f), f >::call
    );
    return *this;
}

template< typename T >
template< typename F, F f >
RegisteredClassContext<T>& RegisteredClassContext<T>::bindMethod( bool isStatic, std::string s ) {
    detail::registerFunction(
        module_->name_,
        class_, isStatic,
        s,
        detail::ForeignMethodWrapper< decltype(f), f >::call
    );
    return *this;
}

template< typename T >
template< typename U, U T::*Field >
RegisteredClassContext<T>& RegisteredClassContext<T>::bindGetter( bool isStatic, std::string s ) {
    detail::registerFunction(
        module_->name_,
        class_, isStatic,
        s,
        detail::propertyGetter< T, U, Field >
    );
    return *this;
}

template< typename T >
template< typename U, U T::*Field >
RegisteredClassContext<T>& RegisteredClassContext<T>::bindSetter( bool isStatic, std::string s ) {
    detail::registerFunction(
        module_->name_,
        class_, isStatic,
        s,
        detail::propertySetter< T, U, Field >
    );
    return *this;
}

template< typename T >
RegisteredClassContext<T>& RegisteredClassContext<T>::bindCFunction( bool isStatic, std::string s, FunctionPtr function ) {
    detail::registerFunction(
        module_->name_,
        class_, isStatic,
        s, function
    );
    return *this;
}

}   // wrenly


#endif  // WRENLY_H_INCLUDED