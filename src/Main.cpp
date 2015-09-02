#include "Wrenly.h"
#include <iostream>

#include <string>
#include <cmath>

#include "detail/ForeignClass.h"

void say( std::string msg ) {
    std::cout << msg << std::endl;
}

std::string message() {
    return std::string("Hello from a string created in C++!");
}

class Factor {
    Factor() = default;
    Factor( double f ) : factor_( f ) {}
    
    inline double times( double x ) { return factor_ * x; }
    
    private:
        double factor_{ 1.0 };
};

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
