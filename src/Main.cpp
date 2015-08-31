#include "Wrenly.h"
#include <iostream>

#include <string>
#include <cmath>

void say( std::string msg ) {
    std::cout << msg << std::endl;
}

std::string message() {
    return std::string("Hello from a string created in C++!");
}

int main( int argc, char** argv ) {
    wrenly::Wren wren{};
    
    wren.beginModule( "math" )
        .beginClass( "Math" )
            .registerFunction< decltype(cos), cos >( true, "cos(_)" )
            .registerFunction< decltype(sin), sin >( true, "sin(_)" )
            .registerFunction< decltype(tan), tan >( true, "tan(_)" )
            .registerFunction< decltype(exp), exp >( true, "exp(_)" );
            
    wren.executeString("import \"math\" for Math\nIO.print( Math.cos(0.12345) )");
    
    return 0;
}
