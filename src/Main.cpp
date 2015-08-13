#include "Wrenly.h"

int main( int argc, char** argv ) {
    wrenly::Wren wren{};
    wren.executeModule( "hello" );
    
    auto say = wren.method( "main", "Foo", "say(_)" );
    say( "s", "Hello from C++!" );
    
    return 0;
}