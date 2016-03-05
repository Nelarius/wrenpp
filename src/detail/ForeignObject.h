#ifndef FOREIGNOBJECT_H_INCLUDED
#define FOREIGNOBJECT_H_INCLUDED

#include "detail/TypeId.h"
extern "C" {
#include <wren.h>
}
#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <vector>
#include <type_traits>

namespace wrenly {
namespace detail {

inline std::vector<std::string>& classNameStorage() {
    static std::vector<std::string> classNames{};
    return classNames;
}

inline std::vector<std::string>& moduleNameStorage() {
    static std::vector<std::string> moduleNames{};
    return moduleNames;
}

inline void storeClassName(std::uint32_t index, const std::string& className) {
    assert(classNameStorage().size() == index);
    classNameStorage().push_back(className);
}

inline void storeModuleName(std::uint32_t index, const std::string& moduleName) {
    assert(moduleNameStorage().size() == index);
    moduleNameStorage().push_back(moduleName);
}

template<typename T>
void bindTypeToClassName(const std::string& className) {
    std::uint32_t id = getTypeId<T>();
    storeClassName(id, className);
}

template<typename T>
void bindTypeToModuleName(const std::string& moduleName) {
    std::uint32_t id = getTypeId<T>();
    storeModuleName(id, moduleName);
}

template<typename T>
const char* getWrenClassString() {
    std::uint32_t id = getTypeId<T>();
    return classNameStorage()[id].c_str();
}

template<typename T>
const char* getWrenModuleString() {
    std::uint32_t id = getTypeId<T>();
    return moduleNameStorage()[id].c_str();
}

/*
 * The interface for getting the object pointer. The actual C++ object may lie within the Wren
 * object, or may live in C++.
 */
class ForeignObject {
public:
    virtual ~ForeignObject() = default;
    virtual void* objectPtr() = 0;
};

/*
 * This wraps a class object by value. The lifetimes of these objects are managed in Wren.
 */
template<typename T>
class ForeignObjectValue : public ForeignObject {
public:
    ForeignObjectValue() {
        if (Padding > 0u) {
            uintptr_t offset = reinterpret_cast<uintptr_t>(&data_[0]) % alignof(T);
            if (offset > 0u) {
                offset = alignof(T) - offset;
            }
            assert(offset < Padding);
            data_[sizeof(data_) - 1u] = uint8_t(offset);
        }
    }

    virtual ~ForeignObjectValue() {
        T* obj = static_cast<T*>(objectPtr());
        obj->~T();
    }

    void* objectPtr() override {
        if (Padding == 0u) {
            return &data_[0];
        }
        else {
            // alignment offset is stored in the last byte
            return &data_[0] + data_[sizeof(data_) - 1u];
        }
    }

    template<int Slot, typename... Args>
    static void setInSlot(WrenVM* vm, Args... arg) {
        wrenEnsureSlots(vm, Slot + 1);
        // get the foreign class value here somehow, and stick it in slot Slot
        wrenGetVariable(vm, getWrenModuleString<T>(), getWrenClassString<T>(), Slot);
        ForeignObjectValue<T>* val = new (wrenSetSlotNewForeign(vm, Slot, Slot, sizeof(ForeignObjectValue<T>))) ForeignObjectValue<T>();
        new (val->objectPtr()) T{ std::forward<Args>(arg)... };
    }

private:
    using AlignType = std::conditional_t<alignof(T) <= alignof(std::size_t), T, std::size_t>;
    static constexpr std::size_t Padding = alignof(T) <= alignof(AlignType) ? 0u : alignof(T) - alignof(AlignType) + 1u;
    alignas(AlignType) std::uint8_t data_[sizeof(T) + Padding];
};

/*
 * Wraps a pointer to a class object. The lifetimes of the pointed-to objects are managed by the 
 * host program.
 */
template<typename T>
class ForeignObjectPtr : public ForeignObject {
public:
    explicit ForeignObjectPtr(T* object)
        : object_{ object } {}
    virtual ~ForeignObjectPtr() = default;

    void* objectPtr() override {
        return object_;
    }

    template<int Slot>
    static void setInSlot(WrenVM* vm, T* obj) {
        wrenEnsureSlots(vm, Slot + 1);
        wrenGetVariable(vm, getWrenModuleString<T>(), getWrenClassString<T>(), Slot);
        void* bytes = wrenSetSlotNewForeign(vm, Slot, Slot, sizeof(ForeignObjectPtr<T>));
        new (bytes) ForeignObjectPtr<T>{ obj };
    }

private:
    T* object_;
};

}
}

#endif  // FOREIGNOBJECT_H_INCLUDED
