
#include "Wrenly.h"
#include "File.h"
#include <cstdlib>  // for malloc
#include <cstring>  // for strcmp
#include <iostream>
#include <cstdio>

namespace {

std::unordered_map< std::size_t, WrenForeignMethodFn > boundForeignMethods{};
std::unordered_map< std::size_t, WrenForeignClassMethods > boundForeignClasses{};

/*
    * This function is going to use a global pointer to a bound method tree.
    *
    * When the correspodning wren vm executes a module, it has to bind its instances
    * of the bound method/class tree to the global variable.
    *
    * There is no way to avoid this static/global function
    * */
WrenForeignMethodFn ForeignMethodProvider( WrenVM* vm,
                                const char* module,
                                const char* className,
                                bool isStatic,
                                const char* signature ) {
    auto it = boundForeignMethods.find( wrenly::detail::HashMethodSignature( module, className, isStatic, signature ) );
    if ( it == boundForeignMethods.end() ) {
        return NULL;
    }

    return it->second;
}

WrenForeignClassMethods ForeignClassProvider( WrenVM* vm, const char* m, const char* c ) {
    auto it = boundForeignClasses.find( wrenly::detail::HashClassSignature( m, c ) );
    if ( it == boundForeignClasses.end() ) {
        return WrenForeignClassMethods{ nullptr, nullptr };
    }

    return it->second;
}

char* LoadModuleFnWrapper( WrenVM* vm, const char* mod ) {
    return wrenly::Wren::loadModuleFn( mod );
}

void WriteFnWrapper( WrenVM* vm, const char* text ) {
    wrenly::Wren::writeFn( vm, text );
}

}

namespace wrenly {

Value::Value( WrenVM* vm, WrenValue* val )
:   vm_{ vm },
    value_{ val },
    refCount_{ nullptr } {
    refCount_ = new unsigned;
    *refCount_ = 1u;
}

Value::Value( const Value& other )
:   vm_{ other.vm_ },
    value_{ other.value_ },
    refCount_{ other.refCount_ } {
    retain_();
}

Value::Value( Value&& other )
:   vm_{ other.vm_ },
    value_{ other.value_ },
    refCount_{ other.refCount_ } {
    other.vm_ = nullptr;
    other.value_ = nullptr;
    other.refCount_ = nullptr;
}

Value& Value::operator=( Value rhs ) {
    release_();
    vm_ = rhs.vm_;
    value_ = rhs.value_;
    refCount_ = rhs.refCount_;
    retain_();
    return *this;
}

Value::~Value() {
    release_();
}

bool Value::isNull() const {
    return value_ == nullptr;
}

WrenValue* Value::pointer() {
    return value_;
}

void Value::retain_() {
    *refCount_ += 1u;
}

void Value::release_() {
    if ( refCount_ ) {
        *refCount_ -= 1u;
        if ( *refCount_ == 0u ) {
            wrenReleaseValue( vm_, value_ );
            delete refCount_;
            refCount_ = nullptr;
        }
    }
}

Method::Method( WrenVM* vm, WrenValue* method )
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
            wrenReleaseValue( vm_, method_ );
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

ModuleContext& ClassContext::endClass() {
    return *module_;
}

ClassContext::ClassContext( std::string c, Wren* wren, ModuleContext* mod )
:   wren_( wren ),
    module_( mod ),
    class_( c )
    {}

ClassContext& ClassContext::registerCFunction( bool isStatic, std::string signature, FunctionPtr function ) {
    wren_->registerFunction_(
        module_->module_,
        class_, isStatic,
        signature, function
    );
    return *this;
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

WriteFn Wren::writeFn = []( WrenVM* vm, const char* text ) -> void {
    printf( "%s", text );
};

Wren::Wren()
:   vm_( nullptr ) {
    
    WrenConfiguration configuration{};
    wrenInitConfiguration( &configuration );
    configuration.bindForeignMethodFn = ForeignMethodProvider;
    configuration.loadModuleFn = LoadModuleFnWrapper;
    configuration.bindForeignClassFn = ForeignClassProvider;
    configuration.writeFn = WriteFnWrapper;
    vm_ = wrenNewVM( &configuration );
}

Wren::Wren( Wren&& other )
:   vm_( other.vm_ ) {
    other.vm_ = nullptr;
}

Wren& Wren::operator=( Wren&& rhs ) {
    vm_             = rhs.vm_;
    rhs.vm_         = nullptr;
    return *this;
}

Wren::~Wren() {
    if ( vm_ != nullptr ) {
        wrenFreeVM( vm_ );
    }
}

WrenVM* Wren::vm() {
    return vm_;
}

void Wren::executeModule( const std::string& mod ) {
    std::string file = mod;
    file += ".wren";
    auto source = FileToString( file );
    auto res = wrenInterpret( vm_, source.c_str() );
    
    if ( res == WrenInterpretResult::WREN_RESULT_COMPILE_ERROR ) {
        std::cerr << "WREN_RESULT_COMPILE_ERROR in module " << mod << std::endl;
    }
    
    if ( res == WrenInterpretResult::WREN_RESULT_RUNTIME_ERROR ) {
        std::cerr << "WREN_RESULT_RUNTIME_ERROR in module " << mod << std::endl;
    }
}

void Wren::executeString( const std::string& code ) {

    auto res = wrenInterpret( vm_, code.c_str() );

    if ( res == WrenInterpretResult::WREN_RESULT_COMPILE_ERROR ) {
        std::cerr << "WREN_RESULT_COMPILE_ERROR in string: " << code << std::endl;
    }

    if ( res == WrenInterpretResult::WREN_RESULT_RUNTIME_ERROR ) {
        std::cerr << "WREN_RESULT_RUNTIME_ERROR in string: " << code << std::endl;
    }
}

void Wren::collectGarbage() {
    wrenCollectGarbage( vm_ );
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
    boundForeignMethods.insert( std::make_pair( hash, function ) );
}

void Wren::registerClass_(
    const std::string& m,
    const std::string& c,
    WrenForeignClassMethods methods
) {
    std::size_t hash = detail::HashClassSignature( m.c_str(), c.c_str() );
    boundForeignClasses.insert( std::make_pair( hash, methods ) );
}

}   // wrenly