#include "Wrenly.h"
#include <iostream>

#include <type_traits>
#include <functional>

void say( std::string msg ) {
    std::cout << msg << std::endl;
}

int main( int argc, char** argv ) {
    wrenly::Wren wren{};
        
    wren.registerMethod( "main", "Foo", false, "say(_)", wrenly::detail::ForeignMethodWrapper<decltype(say), say>::call );
    wren.executeModule( "hello" );
    
    return 0;
}
