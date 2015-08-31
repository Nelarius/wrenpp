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

double MyCos( double x ) {
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
    
    wren.beginModule( "math" )
        .beginClass( "Math" )
            .registerFunction< decltype(MyCos), MyCos >( true, "cos(_)" )
            .registerFunction< decltype(MySin), MySin >( true, "sin(_)" )
            .registerFunction< decltype(MyTan), MyTan >( true, "tan(_)" )
            .registerFunction< decltype(MyExp), MyExp >( true, "exp(_)" );
            
    wren.executeString("import \"math\" for Math\nIO.print( Math.cos(0.12345) )");
    
    return 0;
}
