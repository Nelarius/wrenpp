#include "Wrenly.h"
#include <iostream>

void say( WrenVM* vm ) {
    std::cout << "Hello from C++!\n";
}

int main( int argc, char** argv ) {
    wrenly::Wren wren{};
    wren.registerMethod( "main", "Foo", "say()", say );
    wren.executeModule( "hello" );
    
    return 0;
}