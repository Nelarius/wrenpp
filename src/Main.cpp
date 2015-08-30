#include "Wrenly.h"
#include <iostream>

#include <string>
#include <type_traits>
#include <functional>
#include <cmath>

void say( std::string msg ) {
    std::cout << msg << std::endl;
}

std::string message() {
    return std::string("Hello from a string created in C++!");
}

double MyCos( double x ) {
    std::cout << "Calling my cosine function!" << std::endl;
    return cos( x );
}

double MySin( double x ) {
    return sin( x );
}

double MyTan( double x ) {
    return tan( x );
}

double MyExp( double x ) {
    return exp( x );
}

int main( int argc, char** argv ) {
    wrenly::Wren wren{};
    
    wren.registerMethod( "main", "Math", true, "cos(_)", wrenly::detail::ForeignMethodWrapper< decltype(MyCos), MyCos >::call );
    wren.registerMethod( "main", "Math", true, "sin(_)", wrenly::detail::ForeignMethodWrapper< decltype(MySin), MySin >::call );
    wren.registerMethod( "main", "Math", true, "exp(_)", wrenly::detail::ForeignMethodWrapper< decltype(MyExp), MyExp >::call );
    
    wren.registerMethod( "main", "Foo", true, "say(_)", wrenly::detail::ForeignMethodWrapper< decltype(say), say >::call );
    wren.registerMethod( "main", "Foo", true, "messageFromCpp()", wrenly::detail::ForeignMethodWrapper< decltype(message), message >::call );
    wren.executeModule( "hello" );
    
    return 0;
}
