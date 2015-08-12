
#pragma once

extern "C" {
    #include <wren.h>
}
#include <string>
#include <functional>

namespace wrenly {

using LoadModuleFn = std::function<char*(WrenVM*, const char*)>;

/**
 * @class Wren
 * @author Nelarius
 * @date 09/08/2015
 * @file Wren.h
 * @brief Holds an instance of the Wren virtual machine.
 * This class is refcounted, so it can be passed around safely.
 * This class is NOT thread-safe, however.
 */
class Wren {

    public:
        Wren();
        Wren( const Wren& );
        Wren( Wren&& );
        Wren& operator=( const Wren& );
        Wren& operator=( Wren&& );
        ~Wren();
        
        void executeModule( const std::string& );
        
        static LoadModuleFn loadModuleFn;
        
    private:
        void retain_();
        void release_();
    
        WrenVM*	    vm_;
        unsigned*   refCount_;
};


}
