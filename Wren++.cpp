#include "Wren++.h"
#include <cstdlib>  // for malloc
#include <cstring>  // for strcmp, memcpy
#include <cstdio>
#include <cassert>

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

inline const char* errorTypeToString(WrenErrorType type) {
    switch (type) {
    case WREN_ERROR_COMPILE: return "WREN_ERROR_COMPILE";
    case WREN_ERROR_RUNTIME: return "WREN_ERROR_RUNTIME";
    case WREN_ERROR_STACK_TRACE: return "WREN_ERROR_STACK_TRACE";
    default: assert(false); return "";
    }
}

char* loadModuleFnWrapper(WrenVM* vm, const char* mod) {
    return wrenpp::VM::loadModuleFn(mod);
}

void writeFnWrapper(WrenVM* vm, const char* text) {
    wrenpp::VM::writeFn(vm, text);
}

void errorFnWrapper(WrenErrorType type, const char* module, int line, const char* message) {
    wrenpp::VM::errorFn(type, module, line, message);
}

void* reallocateFnWrapper(void* memory, std::size_t newSize) {
    return wrenpp::VM::reallocateFn(memory, newSize);
}

}

namespace wrenpp {

namespace detail {
void registerFunction(const std::string& mod, const std::string& cName, bool isStatic, std::string sig, FunctionPtr function) {
    std::size_t hash = detail::hashMethodSignature(mod.c_str(), cName.c_str(), isStatic, sig.c_str());
    boundForeignMethods.insert(std::make_pair(hash, function));
}

void registerClass(const std::string& mod, std::string cName, WrenForeignClassMethods methods) {
    std::size_t hash = detail::hashClassSignature(mod.c_str(), cName.c_str());
    boundForeignClasses.insert(std::make_pair(hash, methods));
}

}

Value::Value(bool val)
    :   valueType_{ WREN_TYPE_BOOL }, string_{ nullptr } {
    set_(val);
}

Value::Value(float val)
    :   valueType_{ WREN_TYPE_NUM }, string_{ nullptr } {
    set_(val);
}

Value::Value(double val)
    :   valueType_{ WREN_TYPE_NUM }, string_{ nullptr } {
    set_(val);
}

Value::Value(int val)
    :   valueType_{ WREN_TYPE_NUM }, string_{ nullptr } {
    set_(val);
}

Value::Value(unsigned int val)
    :   valueType_{ WREN_TYPE_NUM }, string_{ nullptr } {
    set_(val);
}

Value::Value(const char* str)
    :   valueType_{ WREN_TYPE_STRING }, string_{ nullptr } {
    string_ = (char*)VM::reallocateFn(nullptr, std::strlen(str) + 1);
    std::strcpy(string_, str);
}

Value::~Value() {
    if (string_) {
        VM::reallocateFn(string_, 0u);
    }
}

Method::Method(VM* vm, WrenHandle* variable, WrenHandle* method)
    : vm_(vm),
    method_(method),
    variable_(variable),
    refCount_(nullptr) {
    refCount_ = new unsigned;
    *refCount_ = 1u;
}

Method::Method(const Method& other)
    : vm_(other.vm_),
    method_(other.method_),
    variable_(other.variable_),
    refCount_(other.refCount_) {
    retain_();
}

Method::Method(Method&& other)
    : vm_(other.vm_),
    method_(other.method_),
    variable_(other.variable_),
    refCount_(other.refCount_) {
    other.vm_ = nullptr;
    other.method_ = nullptr;
    other.variable_ = nullptr;
    other.refCount_ = nullptr;
}

Method::~Method() {
    release_();
}

Method& Method::operator=(const Method& rhs) {
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
    if (refCount_) {
        *refCount_ -= 1u;
        if (*refCount_ == 0u) {
            vm_->setState_();   // wrenReelaseValue will cause wren to free memory
            wrenReleaseHandle(vm_->vm(), method_);
            wrenReleaseHandle(vm_->vm(), variable_);
            delete refCount_;
            refCount_ = nullptr;
        }
    }
}

ModuleContext::ModuleContext(std::string module)
    : name_(module)
{}

ClassContext ModuleContext::beginClass(std::string c) {
    return ClassContext(c, this);
}

void ModuleContext::endModule() {}

ModuleContext beginModule(std::string mod) {
    return ModuleContext{ mod };
}

ModuleContext& ClassContext::endClass() {
    return *module_;
}

ClassContext::ClassContext(std::string c, ModuleContext* mod)
    : module_(mod),
    class_(c)
{}

ClassContext& ClassContext::bindCFunction(bool isStatic, std::string signature, FunctionPtr function) {
    detail::registerFunction(module_->name_, class_, isStatic, signature, function);
    return *this;
}

/*
 * Returns the source as a heap-allocated string.
 * Uses malloc, because our reallocateFn is set to default:
 * it uses malloc, realloc and free.
 * */
LoadModuleFn VM::loadModuleFn = [](const char* mod) -> char* {
    std::string path(mod);
    path += ".wren";
    std::string source;
    try {
        source = wrenpp::detail::fileToString(path);
    }
    catch (const std::exception&) {
        return NULL;
    }
    char* buffer = (char*)malloc(source.size());
    assert(buffer != nullptr);
    memcpy(buffer, source.c_str(), source.size());
    return buffer;
};

WriteFn VM::writeFn = [](WrenVM* vm, const char* text) -> void {
    printf("%s", text);
    fflush(stdout);
};

ErrorFn VM::errorFn = [](WrenErrorType type, const char* module, int line, const char* message) -> void {
    const char* typeStr = errorTypeToString(type);
    printf("%s in %s:%i > %s\n", typeStr, module, line, message);
    fflush(stdout);
};

ReallocateFn VM::reallocateFn = std::realloc;

std::size_t VM::initialHeapSize = 0xA00000u;

std::size_t VM::minHeapSize = 0x100000u;

int VM::heapGrowthPercent = 50;

std::size_t VM::chunkSize = 0x500000u;

VM::VM()
    : vm_{ nullptr } {
    setState_();

    WrenConfiguration configuration{};
    wrenInitConfiguration( &configuration );
    configuration.reallocateFn = reallocateFnWrapper;
    configuration.initialHeapSize = initialHeapSize;
    configuration.minHeapSize = minHeapSize;
    configuration.heapGrowthPercent = heapGrowthPercent;
    configuration.bindForeignMethodFn = foreignMethodProvider;
    configuration.loadModuleFn = loadModuleFnWrapper;
    configuration.bindForeignClassFn = foreignClassProvider;
    configuration.writeFn = writeFnWrapper;
    configuration.errorFn = errorFnWrapper;
    vm_ = wrenNewVM( &configuration );
}

VM::VM(VM&& other )
    : vm_{ other.vm_ } {
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
    setState_();
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
    setState_();
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
    setState_();
    wrenEnsureSlots( vm_, 1 );
    wrenGetVariable( vm_, mod.c_str(), var.c_str(), 0 );
    WrenHandle* variable = wrenGetSlotHandle( vm_, 0 );
    WrenHandle* handle = wrenMakeCallHandle(vm_, sig.c_str());
    return Method( this, variable, handle );
}

void VM::setState_() {
    // TODO: keep this method for now
}

}
