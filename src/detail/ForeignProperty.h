#ifndef FOREIGNPROPERTY_H_INCLUDED
#define FOREIGNPROPERTY_H_INCLUDED

extern "C" {
    #include <wren.h>
}
#include "detail/ForeignMethod.h"
#include "detail/ForeignObject.h"

namespace wrenpp {
namespace detail {

template< typename T, typename U, U T::*Field >
void propertyGetter( WrenVM* vm ) {
    ForeignObject* objWrapper = static_cast<ForeignObject*>(wrenGetSlotForeign(vm, 0));
    const T* obj = static_cast<const T*>(objWrapper->objectPtr());
    WrenSlotAPI< U >::set( vm, 0, obj->*Field );
}

template< typename T, typename U, U T::*Field >
void propertySetter( WrenVM* vm ) {
    ForeignObject* objWrapper = static_cast<ForeignObject*>(wrenGetSlotForeign(vm, 0));
    T* obj = static_cast<T*>(objWrapper->objectPtr());
    obj->*Field = WrenSlotAPI< U >::get( vm, 1 );
}

}
}

#endif  // FOREIGNPROPERTY_H_INCLUDED
