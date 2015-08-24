#pragma once 

#include <cstdint>

namespace wrenly {

class BaseType {
    protected:
        static uint32_t familyCounter_;
};

template<typename T>
class Type: BaseType {
    public:
        static uint32_t family() {
            static uint32_t f{ familyCounter_++ };
            return f;
        }
};

}