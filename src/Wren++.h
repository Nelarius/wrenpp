#ifndef WRENPP_H_INCLUDED
#define WRENPP_H_INCLUDED

#include "detail/ForeignMethod.h"
#include "detail/ForeignClass.h"
#include "detail/ForeignMethod.h"
#include "detail/ForeignObject.h"
#include "detail/ForeignProperty.h"
#include "detail/ChunkAllocator.h"
extern "C" {
    #include <wren.h>
}
#include <string>
#include <functional>   // for std::hash
#include <cassert>
#include <cstdint>
#include <cstdlib>      // for std::size_t
#include <unordered_map>

namespace wrenpp {

using FunctionPtr   = WrenForeignMethodFn;
using LoadModuleFn  = std::function< char*( const char* ) >;
using WriteFn       = std::function< void( WrenVM*, const char* ) >;
using AllocateFn    = std::function<void*(std::size_t)>;
using FreeFn        = std::function<void(void*)>;

namespace detail {
    void registerFunction(
        const std::string& mod, const std::string& clss, bool isStatic,
        std::string sig, FunctionPtr function
    );
    void registerClass(
        const std::string& mod, std::string clss, WrenForeignClassMethods methods
    );
}

class VM;

/**
 * Note that this class stores a reference to the owning VM instance!
 * Make sure you don't move the VM instance which created this object, before
 * this object goes out of scope!
 */
class Method {
    public:
        Method( VM*, WrenValue* variable, WrenValue* method );
        Method() = delete;
        Method( const Method& );
        Method( Method&& );
        Method& operator=( const Method& );
        Method& operator=( Method&& );
        ~Method();

        // this is const because we want to be able to pass this around like
        // immutable data
        template<typename... Args>
        void operator()( Args... args ) const;

    private:
        void retain_();
        void release_();

        mutable VM*             vm_;    // this pointer is not managed here
        mutable WrenValue*      method_;
        mutable WrenValue*      variable_;
        unsigned*               refCount_;
};

class ModuleContext;

class ClassContext {
    public:
        ClassContext() = delete;
        ClassContext( std::string c, ModuleContext* mod );
        virtual ~ClassContext() = default;

        template< typename F, F f >
        ClassContext& bindFunction( bool isStatic, std::string signature );
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
        :   ClassContext( c, mod ) {}

        virtual ~RegisteredClassContext() = default;

        template< typename F, F f >
        RegisteredClassContext& bindMethod( bool isStatic, std::string signature );
        template< typename U, U T::*Field >
        RegisteredClassContext& bindGetter( std::string signature );
        template< typename U, U T::*Field >
        RegisteredClassContext& bindSetter( std::string signature );
        RegisteredClassContext& bindCFunction( bool isStatic, std::string signature, FunctionPtr function );
};

class ModuleContext {
    public:
        ModuleContext() = delete;
        ModuleContext( std::string mod );

        ClassContext beginClass( std::string className );

        template< typename T, typename... Args >
        RegisteredClassContext<T> bindClass( std::string className );

        void endModule();

    private:
        friend class ClassContext;
        template< typename T > friend class RegisteredClassContext;

        std::string name_;
};

ModuleContext beginModule( std::string mod );

enum class Result {
    Success,
    CompileError,
    RuntimeError
};

class VM {

    public:
        VM();
        VM( const VM& )             = delete;
        VM(VM&& );
        VM& operator=( const VM& )  = delete;
        VM& operator=(VM&& );
        ~VM();

        WrenVM* vm();

        Result executeModule( const std::string& );
        Result executeString( const std::string& );

        void collectGarbage();

        /**
         * The signature consists of the name of the method, followed by a
         * parenthesis enclosed list of of underscores representing each argument.
         */
        Method method(
            const std::string& module,
            const std::string& variable,
            const std::string& signature
        );

        static LoadModuleFn loadModuleFn;
        static WriteFn      writeFn;
        static AllocateFn   allocateFn;
        static FreeFn       freeFn;
        static std::size_t  initialHeapSize;
        static std::size_t  minHeapSize;
        static int          heapGrowthPercent;
        static std::size_t  chunkSize;

    private:
        friend class ModuleContext;
        friend class ClassContext;
        friend class Method;
        template< typename T > friend class RegisteredClassContext;

        void setState_();

        WrenVM*	               vm_;
        detail::ChunkAllocator allocator_;
};

template< typename... Args >
void Method::operator()( Args... args ) const {
    vm_->setState_();
    constexpr const std::size_t Arity = sizeof...( Args );
    wrenEnsureSlots( vm_->vm(), Arity + 1u );
    wrenSetSlotValue(vm_->vm(), 0, variable_);

    std::tuple<Args...> tuple = std::make_tuple( args... );
    detail::passArgumentsToWren( vm_->vm(), tuple, std::make_index_sequence<Arity> {} );

    wrenCall( vm_->vm(), method_ );
}

template< typename T, typename... Args >
RegisteredClassContext<T> ModuleContext::bindClass( std::string className ) {
    WrenForeignClassMethods wrapper{ &detail::allocate< T, Args... >, &detail::finalize< T > };
    detail::registerClass( name_, className, wrapper );
    // store the name and module
    assert(detail::classNameStorage().size() == detail::getTypeId<T>());
    assert(detail::moduleNameStorage().size() == detail::getTypeId<T>());
    detail::bindTypeToModuleName<T>(name_);
    detail::bindTypeToClassName<T>(className);
    return RegisteredClassContext<T>( className, this );
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
RegisteredClassContext<T>& RegisteredClassContext<T>::bindGetter( std::string s ) {
    detail::registerFunction(
        module_->name_,
        class_, false,
        s,
        detail::propertyGetter< T, U, Field >
    );
    return *this;
}

template< typename T >
template< typename U, U T::*Field >
RegisteredClassContext<T>& RegisteredClassContext<T>::bindSetter( std::string s ) {
    detail::registerFunction(
        module_->name_,
        class_, false,
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

template<typename T, int Slot>
T* getSlotForeign(WrenVM* vm) {
    detail::ForeignObject* obj = static_cast<detail::ForeignObject*>(wrenGetSlotForeign(vm, Slot));
    return static_cast<T*>(obj->objectPtr());
}

template<typename T>
void setSlotForeignValue(WrenVM* vm, const T& obj) {
    detail::ForeignObjectValue<T>::setInSlot<0>(vm, obj);
}

template<typename T>
void setSlotForeignPtr(WrenVM* vm, T* obj) {
    detail::ForeignObjectPtr<T>::setInSlot<0>(vm, obj);
}

}

#endif  // WRENPP_H_INCLUDED
