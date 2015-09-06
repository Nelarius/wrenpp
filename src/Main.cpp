#include "Wrenly.h"
#include <iostream>

#include <string>
#include <cmath>

class Factor {
    public:
        Factor() = default;
        Factor( double f ) : factor_( f ) {}
        
        inline double times( double x ) { return factor_ * x; }
    
    private:
        double factor_{ 1.0 };
};

int main( int argc, char** argv ) {
    wrenly::Wren wren{};
    
    wren.beginModule( "main" )
        .registerClass< Factor, double >( "Factor" )
            .registerMethod< double(Factor::*)( double ), &Factor::times >( false, "times(_)" );
            
    wren.executeString("foreign class Factor {\n construct new( x ) {}\n //foreign times(x)\n }\nvar f = Factor.new( 4.0 )\n");
    
    return 0;
}
