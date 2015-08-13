
#include "Wrenly.h"
#include "File.h"
#include <cstdlib>  // for malloc
#include <cstring>  // for strcmp
#include <iostream>

namespace {
    
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
        //
        return NULL;
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



/*
 * Returns the source as a heap-allocated string.
 * Uses malloc, because our reallocateFn is set to default:
 * it uses malloc, realloc and free.
 * */
LoadModuleFn Wren::loadModuleFn = []( const char* mod ) -> char* {
    std::string path( mod );
    path += ".wren";
    auto source = wrenly::FileToString( path );
    char* buffer = (char*) malloc( source.size() );
    memcpy( buffer, source.c_str(), source.size() );
    return buffer;
};

Wren::Wren()
:   vm_( nullptr ),
    refCount_( nullptr ) {
    refCount_ = new unsigned;
    *refCount_ = 1u;
    
    WrenConfiguration configuration{};
    configuration.bindForeignMethodFn = ForeignMethodFnWrapper;
    configuration.loadModuleFn = LoadModuleFnWrapper;
    vm_ = wrenNewVM( &configuration );
}

Wren::Wren( const Wren& other )
:   vm_( other.vm_ ),
    refCount_( other.refCount_ ) {
    retain_();
}

Wren& Wren::operator=( const Wren& rhs ) {
    release_();
    vm_         = rhs.vm_;
    refCount_   = rhs.refCount_;
    retain_();
    return *this;
}

Wren::Wren( Wren&& other )
:   vm_( other.vm_ ),
    refCount_( other.refCount_ ) {
    other.vm_ = nullptr;
    other.refCount_ = nullptr;
}

Wren& Wren::operator=( Wren&& rhs ) {
    release_();
    vm_         = rhs.vm_;
    refCount_   = rhs.refCount_;
    rhs.vm_       = nullptr;
    rhs.refCount_ = nullptr;
    return *this;
}

Wren::~Wren() {
    release_();
}

void Wren::retain_() {
    *refCount_ += 1u;
}

void Wren::release_() {
    if ( refCount_ ) {
        *refCount_ -= 1u;
        if ( *refCount_ == 0u ) {
            wrenFreeVM( vm_ );
            delete refCount_;
            refCount_ = nullptr;
        }
    }
}

void Wren::executeModule( const std::string& mod ) {
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

Method Wren::method( 
    const std::string& mod,
    const std::string& var,
    const std::string& sig
) {
    return Method( vm_, wrenGetMethod( vm_, mod.c_str(), var.c_str(), sig.c_str() ) );
}

}   // wrenly
