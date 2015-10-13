#ifndef WRENLY_H_INCLUDED
#define WRENLY_H_INCLUDED

#include "detail/ForeignMethod.h"
#include "detail/ForeignClass.h"
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

using LoadModuleFn = std::function< char*( const char* ) >;
using WriteFn = std::function< void( WrenVM*, const char* ) >;

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
        
        template<typename... Args>
        void operator()( Args&&... args );
    
    private:        
        void retain_();
        void release_();
    
        WrenVM*     vm_;
        WrenValue*  method_;
        unsigned*   refCount_;
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
        ClassContext( std::string c, Wren* wren, ModuleContext* mod );
        virtual ~ClassContext() = default;
        
        template< typename F, F f >
        ClassContext& registerFunction( bool isStatic, const std::string& signature );
        
        ModuleContext& endClass();
        
    protected:
        Wren*           wren_;
        ModuleContext*  module_;
        std::string     class_;
};

template< typename T >
class RegisteredClassContext: public ClassContext {
    public:
        RegisteredClassContext( std::string c, Wren* wren, ModuleContext* mod )
        :   ClassContext( c, wren, mod )
            {}
        virtual ~RegisteredClassContext() = default;
        
        template< typename F, F f >
        RegisteredClassContext& registerMethod( bool isStatic, const std::string& signature );
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
        ModuleContext( std::string mod, Wren* wren );
        
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
        RegisteredClassContext<T> registerClass( std::string className );
        
        void endModule();
        
    private:
        friend class ClassContext;
        template< typename T > friend class RegisteredClassContext;
        
        Wren*       wren_;
        std::string module_;
};

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
        ModuleContext beginModule( std::string mod );
        
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
        
        void registerFunction_(
            const std::string& module,
            const std::string& className,
            bool isStatic,
            const std::string& signature,
            WrenForeignMethodFn function
        );
        
        void registerClass_(
            const std::string& module,
            const std::string& className,
            WrenForeignClassMethods methods
        );
        
        WrenVM*	    vm_;
        std::unordered_map<std::size_t, WrenForeignMethodFn>        foreignMethods_;
        std::unordered_map<std::size_t, WrenForeignClassMethods>    foreignClasses_;
};

template< typename... Args >
void Method::operator()( Args&&... args ) {
    constexpr const std::size_t Arity = sizeof...( Args );
    detail::ArgumentListString< Arity > arguments{ args... };
    WrenValue* result{ nullptr };
    wrenCall( vm_, method_, &result, arguments.getString(), std::forward<Args>( args )... );
}

template< typename T, typename... Args >
RegisteredClassContext<T> ModuleContext::registerClass( std::string c ) {
    WrenForeignClassMethods wrapper{ &detail::Allocate< T, Args... >, &detail::Finalize< T > };
    wren_->registerClass_( module_, c, wrapper );
    return RegisteredClassContext<T>( c, wren_, this );
}

template< typename F, F f >
ClassContext& ClassContext::registerFunction( bool isStatic, const std::string& s ) {
    wren_->registerFunction_( 
        module_->module_, 
        class_, isStatic, 
        s, 
        detail::ForeignMethodWrapper< decltype(f), f >::call 
    );
    return *this;
}

template< typename T >
template< typename F, F f >
RegisteredClassContext<T>& RegisteredClassContext<T>::registerMethod( bool isStatic, const std::string& s ) {
    wren_->registerFunction_( 
        module_->module_,
        class_, isStatic,
        s,
        detail::ForeignMethodWrapper< decltype(f), f >::call 
    );
    return *this;
}

}   // wrenly


#endif  // WRENLY_H_INCLUDED
