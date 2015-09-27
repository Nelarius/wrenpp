#include "Wrenly.h"
#include "detail/ForeignMethod.h"
#include <iostream>
#include <cstdio>

#include <string>
#include <cmath>

class Factor {
    public:
        Factor() = default;
        Factor( double f ) : factor_( f ) { printf( "constructor called with %f\n", f ); }
        
        inline double times( double x ) { printf( "times getting called with %f. factor_ = %f\n", x, factor_ ); return factor_ * x; }
    
    private:
        double factor_{ 1.0 };
};

int main( int argc, char** argv ) {
    wrenly::Wren wren{};
        
    wren.beginModule( "main" )
        .registerClass< Factor, double >( "Factor" )
            .registerMethod< decltype( &Factor::times ), &Factor::times >( false, "times(_)" );
    
    wren.executeString("foreign class Factor {\n construct new( x ) {} \nforeign times(x)\n }\nvar x = Factor.new( 5.0 )\nSystem.print( x.times( 5.0 ) )");
    
    return 0;
}
