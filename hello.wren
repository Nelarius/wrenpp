class Foo {
  foreign say( message )
}

var h = Foo.new()
h.say( "Hello from Wren via foreign method!" )
