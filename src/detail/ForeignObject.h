#ifndef FOREIGNOBJECT_H_INCLUDED
#define FOREIGNOBJECT_H_INCLUDED

extern "C" {
#include <wren.h>
}
#include <type_traits>
#include <cstdint>
#include <cstdlib>
#include <cassert>

namespace wrenly {
namespace detail {

class IForeignObject {
public:
    virtual void* objectPtr() = 0;
};

/*
 * This wraps a class object by value. The lifetimes of these objects are managed in Wren.
 */
template<typename T>
class ForeignObjectValue : public IForeignObject {
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
    static void setInSlot(WrenVM* vm, const T& obj) {
        wrenEnsureSlots(vm, Slot + 1);
        // get the foreign class value here somehow, and stick it in slot Slot
        ForeignObjectValue<T>* val = new (wrenSetSlotNewForeign(vm, Slot, Slot, sizeof(ForeignObjectValue<T>))) ForeignObjectValue<T>();
        new (val->objectPtr()) T{ std::forward<Args>()... };
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
class ForeignObjectPtr : public IForeignObject {
public:
    explicit ForeignObjectPtr(void* object)
        : object_{ object } {}
    virtual ~ForeignObjectPtr() = default;

    void* objectPtr() override {
        return object_;
    }

    // TODO: ensure that the pointer is not const
    template<int Slot, typename T>
    static void setInSlot(WrenVM* vm, T* obj) {
        wrenEnsureSlots(vm, Slot + 1);
        void* bytes = wrenSetSlotNewForeign(vm, Slot, Slot, sizeof(ForeignObjectPtr));
        new (bytes) ForeignObjectPtr{ obj };
    }

private:
    void* object_;
};

}
}

#endif  // FOREIGNOBJECT_H_INCLUDED
