
#include "Wrenly.h"
#include "File.h"
#include <cstdlib>  // for malloc
#include <cstring>  // for strcmp
#include <iostream>

namespace {
    
    std::unordered_map<std::size_t, WrenForeignMethodFn>* boundForeignMethods{ nullptr };
    
    /*
     * This function is going to use a global pointer to a bound method tree.
     * 
     * When the correspodning wren vm executes a module, it has to bind its instances
     * of the bound method/class tree to the global variable.
     * 
     * There is no way to avoid this static/global function
     * */
    WrenForeignMethodFn ForeignMethodFnWrapper( WrenVM* vm,
                                 const char* module,
                                 const char* className,
                                 bool isStatic,
                                 const char* signature ) {
        if ( !boundForeignMethods ) {
            return NULL;
        }
        auto it = boundForeignMethods->find( wrenly::detail::HashMethodSignature( module, className, isStatic, signature ) );
        if ( it == boundForeignMethods->end() ) {
            return NULL;
        }
        
        return it->second;
    }
    
    char* LoadModuleFnWrapper( WrenVM* vm, const char* mod ) {
        return wrenly::Wren::loadModuleFn( mod );
    }
}

namespace wrenly {

Method::Method( WrenVM* vm, WrenMethod* method )
:   vm_( vm ),
    method_( method ),
    refCount_( nullptr ) {
    refCount_ = new unsigned;
    *refCount_ = 1u;
}

Method::Method( const Method& other )
:   vm_( other.vm_ ),
    method_( other.method_ ),
    refCount_( other.refCount_ ) {
    retain_();
}

Method::Method( Method&& other )
:   vm_( other.vm_ ),
    method_( other.method_ ),
    refCount_( other.refCount_ ) {
    other.vm_ = nullptr;
    other.method_ = nullptr;
    other.refCount_ = nullptr;
}

Method::~Method() {
    release_();
}

Method& Method::operator=( Method rhs ) {
    release_();
    vm_ = rhs.vm_;
    method_ = rhs.method_;
    refCount_ = rhs.refCount_;
    retain_();
    return *this;
}

void Method::retain_() {
    *refCount_ += 1u;
}

void Method::release_() {
    if ( refCount_ ) {
        *refCount_ -= 1u;
        if ( *refCount_ == 0u ) {
            wrenReleaseMethod( vm_, method_ );
            delete refCount_;
            refCount_ = nullptr;
        }
    }
}

ModuleContext::ModuleContext( std::string module, Wren* wren )
:   wren_( wren ),
    module_( module )
    {}
    
ClassContext ModuleContext::beginClass( std::string c ) {
    return ClassContext( c, wren_, this );
}

void ModuleContext::endModule() {}

    
ClassContext::ClassContext( std::string c, Wren* wren, ModuleContext* mod )
:   wren_( wren ),
    module_( mod ),
    class_( c )
    {}
    
ModuleContext& ClassContext::endClass() {
    return *module_;
}

/*
 * Returns the source as a heap-allocated string.
 * Uses malloc, because our reallocateFn is set to default:
 * it uses malloc, realloc and free.
 * */
LoadModuleFn Wren::loadModuleFn = []( const char* mod ) -> char* {
    std::string path( mod );
    path += ".wren";
    std::string source;
    try {
        source = wrenly::FileToString( path );
    } catch( const std::exception& e ) {
        return NULL;
    }
    char* buffer = (char*) malloc( source.size() );
    memcpy( buffer, source.c_str(), source.size() );
    return buffer;
};

Wren::Wren()
:   vm_( nullptr ),
    foreignMethods_() {
    
    WrenConfiguration configuration{};
    configuration.bindForeignMethodFn = ForeignMethodFnWrapper;
    configuration.loadModuleFn = LoadModuleFnWrapper;
    vm_ = wrenNewVM( &configuration );
}

Wren::Wren( Wren&& other )
:   vm_( other.vm_ ),
    foreignMethods_( std::move( other.foreignMethods_ ) ) {
    other.vm_ = nullptr;
}

Wren& Wren::operator=( Wren&& rhs ) {
    vm_             = rhs.vm_;
    foreignMethods_ = std::move( rhs.foreignMethods_ );
    rhs.vm_         = nullptr;
    return *this;
}

Wren::~Wren() {
    wrenFreeVM( vm_ );
}

WrenVM* Wren::vm() {
    return vm_;
}

void Wren::executeModule( const std::string& mod ) {
    // set global variables for the C-callbacks
    boundForeignMethods = &foreignMethods_;
    
    std::string file = mod;
    file += ".wren";
    auto source = FileToString( file );
    auto res = wrenInterpret( vm_, file.c_str(), source.c_str() );
    
    if ( res == WrenInterpretResult::WREN_RESULT_COMPILE_ERROR ) {
        std::cerr << "WREN_RESULT_COMPILE_ERROR in module " << mod << std::endl;
    }
    
    if ( res == WrenInterpretResult::WREN_RESULT_RUNTIME_ERROR ) {
        std::cerr << "WREN_RESULT_RUNTIME_ERROR in module " << mod << std::endl;
    }
}

void Wren::executeString( const std::string& code ) {
    // set global variables for the C-callbacks
    boundForeignMethods = &foreignMethods_;
    
    auto res = wrenInterpret( vm_, "string", code.c_str() );
    
    if ( res == WrenInterpretResult::WREN_RESULT_COMPILE_ERROR ) {
        std::cerr << "WREN_RESULT_COMPILE_ERROR in string: " << code << std::endl;
    }
    
    if ( res == WrenInterpretResult::WREN_RESULT_RUNTIME_ERROR ) {
        std::cerr << "WREN_RESULT_RUNTIME_ERROR in string: " << code << std::endl;
    }
}

void Wren::gc() {
    //vm_->collectGarbage( vm_ );
}

ModuleContext Wren::beginModule( std::string mod ) {
    return ModuleContext( mod, this );
}

Method Wren::method( 
    const std::string& mod,
    const std::string& var,
    const std::string& sig
) {
    return Method( vm_, wrenGetMethod( vm_, mod.c_str(), var.c_str(), sig.c_str() ) );
}

void Wren::registerFunction_(
    const std::string& mod,
    const std::string& cName,
    bool isStatic,
    const std::string& sig,
    WrenForeignMethodFn function
) {
    std::size_t hash = detail::HashMethodSignature( mod.c_str(), cName.c_str(), isStatic, sig.c_str() );
    foreignMethods_.insert( std::make_pair( hash, function ) );
}

}   // wrenly
