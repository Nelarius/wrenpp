#ifndef WRENLY_H_INCLUDED
#define WRENLY_H_INCLUDED

#include "detail/ForeignMethod.h"
extern "C" {
    #include <wren.h>
}
#include <sstream>
#include <string>
#include <vector>
#include <functional>   // for std::hash
#include <cstdint>
#include <unordered_map>

namespace wrenly {

using LoadModuleFn = std::function<char*( const char*)>;

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
        Method( WrenVM*, WrenMethod* );
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
        /*
         * For parsing arguments and telling Wren their type
         * */
        struct Any {
            Any( bool e )   : type( 'b' ) {}
            Any( double e ) : type( 'd' ) {}
            Any( float e )  : type( 'd' ) {}
            Any( int e )    : type( 'i' ) {}
            Any( const char* ) : type( 's' ) {}
            Any( const std::string& ) : type( 's' ) {}
            
            char type;
        };
        
        void retain_();
        void release_();
    
        WrenVM*     vm_;
        WrenMethod* method_;
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
        
        template< typename F, F&& f >
        ClassContext& registerFunction(bool isStatic, const std::string& signature );
        ModuleContext& endClass();
        
    private:
        Wren*           wren_;
        ModuleContext*  module_;
        std::string     class_;
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
        
        ClassContext beginClass( std::string c );
        void endModule();
        
    private:
        friend ClassContext;
        
        Wren*       wren_;
        std::string module_;
};

/**
 * @class Wren
 * @author Nelarius
 * @date 09/08/2015
 * @file Wren.h
 * @brief Holds an instance of the Wren virtual machine. The instance is uniquely owned.
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
        
        ModuleContext beginModule( std::string mod );
        
        Method method(
            const std::string& module,
            const std::string& variable,
            const std::string& signature
        );
        
        static LoadModuleFn loadModuleFn;
        
    private:
        friend class ModuleContext;
        friend class ClassContext;
        
        void registerFunction_(
            const std::string& module,
            const std::string& className,
            bool isStatic,
            const std::string& signature,
            WrenForeignMethodFn function
        );
        
        
        WrenVM*	    vm_;
        std::unordered_map<std::size_t, WrenForeignMethodFn>    foreignMethods_;
};


/////////////////////////////////////////////////////////////////////////////
// Method implementation
/////////////////////////////////////////////////////////////////////////////
template<typename... Args>
void Method::operator()( Args&&... args ) {
    std::vector<Any> vec = { args... };
    std::stringstream ss;
    for ( auto a: vec ) {
        ss << a.type;
    }
    wrenCall( vm_, method_, ss.str().c_str(), std::forward<Args>( args )... );
}

/////////////////////////////////////////////////////////////////////////////
// ClassContext implementation
/////////////////////////////////////////////////////////////////////////////
template< typename F, F&& f >
ClassContext& ClassContext::registerFunction( bool isStatic, const std::string& s ) {
    wren_->registerFunction_( 
        module_->module_, 
        class_, isStatic, 
        s, 
        detail::ForeignMethodWrapper< std::remove_reference_t<decltype(f)>, f >::call 
    );
    return *this;
}

}   // wrenly


#endif  // WRENLY_H_INCLUDED