
#include "Wren++.h"
#include "File.h"
#include "detail/ChunkAllocator.h"
#include <cstdlib>  // for malloc
#include <cstring>  // for strcmp, memcpy
#include <cstdio>
#include <cassert>

namespace wrenpp {
AllocStats stats{};
}

namespace {

std::unordered_map< std::size_t, WrenForeignMethodFn > boundForeignMethods{};
std::unordered_map< std::size_t, WrenForeignClassMethods > boundForeignClasses{};

WrenForeignMethodFn foreignMethodProvider(WrenVM* vm,
    const char* module,
    const char* className,
    bool isStatic,
    const char* signature) {
    auto it = boundForeignMethods.find(wrenpp::detail::hashMethodSignature(module, className, isStatic, signature));
    if (it == boundForeignMethods.end()) {
        return NULL;
    }

    return it->second;
}

WrenForeignClassMethods foreignClassProvider(WrenVM* vm, const char* m, const char* c) {
    auto it = boundForeignClasses.find(wrenpp::detail::hashClassSignature(m, c));
    if (it == boundForeignClasses.end()) {
        return WrenForeignClassMethods{ nullptr, nullptr };
    }

    return it->second;
}

char* loadModuleFnWrapper(WrenVM* vm, const char* mod) {
    return wrenpp::VM::loadModuleFn(mod);
}

void writeFnWrapper(WrenVM* vm, const char* text) {
    wrenpp::VM::writeFn(vm, text);
}

// memory management
// this class contains an std::vector in its private state
wrenpp::detail::ChunkAllocator allocator{};

void* reallocateFnWrapper(void* memory, std::size_t newSize) {
    if (newSize > wrenpp::stats.largestByte) {
        wrenpp::stats.largestByte = newSize;
    }
    else if (newSize < wrenpp::stats.smallestByte) {
        wrenpp::stats.smallestByte = newSize;
    }
    wrenpp::stats.count++;
    wrenpp::stats.accumulation += newSize;
    return realloc(memory, newSize);
}

}

namespace wrenpp {

namespace detail {
    void registerFunction( const std::string& mod, const std::string& cName, bool isStatic, std::string sig, FunctionPtr function ) {
        std::size_t hash = detail::hashMethodSignature( mod.c_str(), cName.c_str(), isStatic, sig.c_str() );
        boundForeignMethods.insert( std::make_pair( hash, function ) );
    }

    void registerClass( const std::string& mod, std::string cName, WrenForeignClassMethods methods ) {
        std::size_t hash = detail::hashClassSignature( mod.c_str(), cName.c_str() );
        boundForeignClasses.insert( std::make_pair( hash, methods ) );
    }
}

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

Value& Value::operator=( const Value& rhs ) {
    release_();
    vm_ = rhs.vm_;
    value_ = rhs.value_;
    refCount_ = rhs.refCount_;
    retain_();
    return *this;
}

Value& Value::operator=(Value&& rhs) {
    release_();
    vm_ = rhs.vm_;
    value_ = rhs.value_;
    refCount_ = rhs.refCount_;
    rhs.vm_ = nullptr;
    rhs.value_ = nullptr;
    rhs.refCount_ = nullptr;
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

Method::Method( WrenVM* vm, WrenValue* variable, WrenValue* method )
:   vm_( vm ),
    method_( method ),
    variable_( variable ),
    refCount_( nullptr ) {
    refCount_ = new unsigned;
    *refCount_ = 1u;
}

Method::Method( const Method& other )
:   vm_( other.vm_ ),
    method_( other.method_ ),
    variable_( other.variable_ ),
    refCount_( other.refCount_ ) {
    retain_();
}

Method::Method( Method&& other )
:   vm_( other.vm_ ),
    method_( other.method_ ),
    variable_( other.variable_ ),
    refCount_( other.refCount_ ) {
    other.vm_ = nullptr;
    other.method_ = nullptr;
    other.variable_ = nullptr;
    other.refCount_ = nullptr;
}

Method::~Method() {
    release_();
}

Method& Method::operator=( const Method& rhs ) {
    release_();
    vm_ = rhs.vm_;
    method_ = rhs.method_;
    variable_ = rhs.variable_;
    refCount_ = rhs.refCount_;
    retain_();
    return *this;
}

Method& Method::operator=(Method&& rhs) {
    release_();
    vm_ = rhs.vm_;
    method_ = rhs.method_;
    variable_ = rhs.variable_;
    refCount_ = rhs.refCount_;
    rhs.vm_ = nullptr;
    rhs.method_ = nullptr;
    rhs.variable_ = nullptr;
    rhs.refCount_ = nullptr;
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
            wrenReleaseValue( vm_, variable_ );
            delete refCount_;
            refCount_ = nullptr;
        }
    }
}

ModuleContext::ModuleContext( std::string module )
:   name_( module )
    {}

ClassContext ModuleContext::beginClass( std::string c ) {
    return ClassContext( c, this );
}

void ModuleContext::endModule() {}

ModuleContext beginModule( std::string mod ) {
    return ModuleContext{ mod };
}

ModuleContext& ClassContext::endClass() {
    return *module_;
}

ClassContext::ClassContext( std::string c, ModuleContext* mod )
:   module_( mod ),
    class_( c )
    {}

ClassContext& ClassContext::bindCFunction( bool isStatic, std::string signature, FunctionPtr function ) {
    detail::registerFunction( module_->name_, class_, isStatic, signature, function );
    return *this;
}

/*
 * Returns the source as a heap-allocated string.
 * Uses malloc, because our reallocateFn is set to default:
 * it uses malloc, realloc and free.
 * */
LoadModuleFn VM::loadModuleFn = []( const char* mod ) -> char* {
    std::string path( mod );
    path += ".wren";
    std::string source;
    try {
        source = wrenpp::fileToString( path );
    } catch( const std::exception& ) {
        return NULL;
    }
    char* buffer = (char*) malloc( source.size() );
    assert(buffer != nullptr);
    memcpy( buffer, source.c_str(), source.size() );
    return buffer;
};

WriteFn VM::writeFn = []( WrenVM* vm, const char* text ) -> void {
    printf( "%s", text );
    fflush(stdout);
};

AllocateFn VM::allocateFn = [](std::size_t bytes) -> void* {
    printf("Allocating %u bytes\n", bytes);
    return malloc(bytes);
};

FreeFn VM::freeFn = [](void* memory) -> void {
    free(memory);
};

std::size_t VM::initialHeapSize = 0xA00000u;

std::size_t VM::minHeapSize = 0x100000u;

int VM::heapGrowthPercent = 50;

std::size_t VM::chunkSize = 0x500000u;;

VM::VM()
:   vm_( nullptr ) {
    
    WrenConfiguration configuration{};
    wrenInitConfiguration( &configuration );
    configuration.reallocateFn = reallocateFnWrapper;
    configuration.bindForeignMethodFn = foreignMethodProvider;
    configuration.loadModuleFn = loadModuleFnWrapper;
    configuration.bindForeignClassFn = foreignClassProvider;
    configuration.writeFn = writeFnWrapper;
    vm_ = wrenNewVM( &configuration );
}

VM::VM(VM&& other )
:   vm_( other.vm_ ) {
    other.vm_ = nullptr;
}

VM& VM::operator=(VM&& rhs ) {
    vm_             = rhs.vm_;
    rhs.vm_         = nullptr;
    return *this;
}

VM::~VM() {
    if ( vm_ != nullptr ) {
        wrenFreeVM( vm_ );
    }
}

WrenVM* VM::vm() {
    return vm_;
}

Result VM::executeModule( const std::string& mod ) {
    const std::string source( loadModuleFn( mod.c_str() ) );
    auto res = wrenInterpret( vm_, source.c_str() );
    
    if ( res == WrenInterpretResult::WREN_RESULT_COMPILE_ERROR ) {
        return Result::CompileError;
    }

    if ( res == WrenInterpretResult::WREN_RESULT_RUNTIME_ERROR ) {
        return Result::RuntimeError;
    }

    return Result::Success;
}

Result VM::executeString( const std::string& code ) {

    auto res = wrenInterpret( vm_, code.c_str() );

    if ( res == WrenInterpretResult::WREN_RESULT_COMPILE_ERROR ) {
        return Result::CompileError;
    }

    if ( res == WrenInterpretResult::WREN_RESULT_RUNTIME_ERROR ) {
        return Result::RuntimeError;
    }
    
    return Result::Success;
}

void VM::collectGarbage() {
    wrenCollectGarbage( vm_ );
}

Method VM::method( 
    const std::string& mod,
    const std::string& var,
    const std::string& sig
) {
    wrenEnsureSlots( vm_, 1 );
    wrenGetVariable( vm_, mod.c_str(), var.c_str(), 0 );
    WrenValue* variable = wrenGetSlotValue( vm_, 0 );
    WrenValue* handle = wrenMakeCallHandle(vm_, sig.c_str());
    return Method( vm_, variable, handle );
}

}