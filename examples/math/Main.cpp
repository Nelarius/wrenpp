#include "Wrenly.h"
#include <cmath>

int main() {
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
