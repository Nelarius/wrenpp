#include "Wrenly.h"

int main( int argc, char** argv ) {
    wrenly::Wren wren{};
    wren.executeModule( "hello" );
    return 0;
}