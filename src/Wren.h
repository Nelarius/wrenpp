
#pragma once

extern "C" {
	#include <wren.h>
}

namespace wrenly {

/**
 * @class Wren
 * @author Nelarius
 * @date 09/08/2015
 * @file Wren.h
 * @brief A refcounted class owning the Wren virtual machine.
 */
class Wren {
    
    public:
        Wren();
        Wren( const Wren& );
        Wren( Wren&& );
        Wren& operator=( const Wren& );
        Wren& operator=( Wren&& );
        ~Wren();
        
    private:
        void retain_();
        void release_();
    
        WrenVM*	    vm_;
        unsigned*   refCount_;
};


}
