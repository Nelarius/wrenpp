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

struct AllocStats {
    std::size_t largestByte{ std::numeric_limits<std::size_t>::min() };
    std::size_t smallestByte{std::numeric_limits<std::size_t>::max() };
    std::size_t count{ 0u };
    std::size_t accumulation{ 0u };
};

extern AllocStats stats;

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
        Value& operator=(const Value&);
        Value& operator=(Value&&);
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

class VM;

/**
 * @class Method
 * @brief Use this to call a wren method from C++.
 * Make sure that the Wren instance which dispensed this object does not go out of scope
 * before this instance does. A segfault will occur otherwise.
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

        /**
        * @brief Call a wren method and pass it any arguments
        * This method is const, because it is intentended to be pass around
        * like immutable data. In reality, the underlying virtual machine might 
        * change its state, depending on the Wren method being called.
        */
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

/**
 * @class ClassContext
 * @brief Stores the class and module name. Gives easier access to function registration.
 */
class ClassContext {
    public:
        ClassContext() = delete;
        ClassContext( std::string c, ModuleContext* mod );
        virtual ~ClassContext() = default;

        /**
         * @brief Register a free function.
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

enum class Result {
    Success,
    CompileError,
    RuntimeError
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
