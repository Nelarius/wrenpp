#ifndef FOREIGNPROPERTY_H_INCLUDED
#define FOREIGNPROPERTY_H_INCLUDED

extern "C" {
    #include <wren.h>
}
#include "detail/ForeignMethod.h"

namespace wrenly {
namespace detail {

template< typename T, typename U, U T::*Field >
void propertyGetter( WrenVM* vm ) {
    const T* obj = (const T*)wrenGetArgumentForeign( vm, 0 );
    WrenReturnValue< U >::ret( vm, obj->*Field );
}

template< typename T, typename U, U T::*Field >
void propertySetter( WrenVM* vm ) {
    T* obj = (T* )wrenGetArgumentForeign( vm, 0 );
    obj->*Field = WrenArgument< U >::get( vm, 1 );
}

}
}

#endif  // FOREIGNPROPERTY_H_INCLUDED