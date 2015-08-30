import "math" for Math

class Foo {
    foreign static say( message )
    foreign static messageFromCpp()
}

var y = Math.cos( 0.32 )

Foo.say( "Hello from Wren via foreign method!" )
IO.print( Foo.messageFromCpp() )
