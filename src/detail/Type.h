#ifndef TYPE_H_INCLUDED
#define TYPE_H_INCLUDED

#include <cstdint>

namespace wrenly {
namespace detail {

class BaseType {
    protected:
        static uint32_t familyCounter_;
};

template<typename T>
class Type: public BaseType {
    public:
        static uint32_t family() {
            static uint32_t f{ familyCounter_++ };
            return f;
        }
};


}   // detail
}   // wrenly

#endif  // TYPE_H_INCLUDED
