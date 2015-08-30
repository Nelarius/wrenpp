
# wrenly

A C++ wrapper for the [Wren programming language](http://munificent.github.io/wren/). As the language itself and this library are both heavily WIP, expect everything in here to change.

The goals of this library are
* Wrap the Wren VM in a nice, easy-to-use class -- DONE
* Wrap the Wren method call in easy-to-use syntax --DONE
* Implement foreign methods using free functions -- WIP
* Implement foreign classes -- WIP

> Oh and by the way, I will place notes on the implementation in quote blocks.

## Building

**TODO**

## Getting started

The Wren virtual machine is contained in the `Wren` class. Here's how you would initialize the virtual machine and execute a module:

```cpp
#include "Wrenly.h"

int main() {
  wrenly::Wren wren{};
  wren.executeModule( "hello" );	// refers to the file hello.wren
  
  return 0;
}
```

The virtual machine is held internally by a pointer. `Wren` uniquely owns the virtual machine, which means that the `Wren` instance can't be copied, but can be moved when needed - just like a unique pointer.

Module names work the same way by default as the module names of the Wren command line module. You specify the location of the module without the `.wren` postfix.

Strings can also be executed:

```cpp
wren.executeString( "IO.print(\"Hello from a C++ string!\")" );
```

## Accessing Wren from C++

### Methods

You can use the `Wren` instance to get a callable handle for a method implemented in Wren. Given the following class, defined in `bar.wren`, 

```dart
class Foo {
  static say( text ) {
    IO.print( text )
  }
}
```

you can call the static method `say` from C++ by using `void Method::operator( Args&&... )`,

```cpp
wrenly::Wren wren{};
wren.executeModule( "bar" );
    
wrenly::Method say = wren.method( "main", "Foo", "say(_)" );
say( "Hello from C++!" );
```

`Wren::method` has the following signature:

```cpp
Method Wren::method( 
  const std::string& module, 
  const std::string& variable,
  const std::string& signature
);
```

`module` will be `"main"`, if you're not in an imported module. `variable` should contain the variable name of the object that you want to call the method on. Note that you use the class name when the method is static. The signature of the method has to be specified, because Wren supports function overloading by arity (overloading by the number of arguments).

## Accessing C++ from Wren
### Foreign methods

You can implement a Wren method as a free C/C++ function. Currently, wrenly simplifies registering with the Wren virtual machine. Here's a small example.

```dart
// inside foo.wren
class Bar {
  foreign say()
}
```

```cpp
#include "Wrenly.h"
#include <iostream>

void say( WrenVM* vm ) {
  std::cout << "Hello from C++\n";
}

int main() {
  wrenly::Wren wren{};
  wren.registerMethod( "main", "Bar", "say()", say );
  wren.executeModule( "foo" );

  return 0;
}
```

> The free function needs to call functions like `wrenGetArgumentDouble`, `wrenGetArgumentString` to access the arguments passed to the method. When you register the free function, Wrenly wraps the free function and generates the appropriate `wrenGetArgument*` function calls during compile time. Similarly, if a function returns a value, the call to the appropriate `wrenReturn*` function is inserted at compile time.

### Foreign classes

## Customize VM behavior
### Customize module loading

When the virtual machine encounters an import statement, it executes a callback function which returns the module source for a given module name. If you want to change the way modules are named, or want some kind of custom file interface, you can change the callback function. Just set give `Wren::loadModuleFn` a new value, which can be a free standing function, or callable object of type `char*( const char* )`.

By default, `Wren::loadModuleFn` has the following value.

```cpp
Wren::loadModuleFn = []( const char* mod ) -> char* {
    std::string path( mod );
    path += ".wren";
    auto source = wrenly::FileToString( path );
    char* buffer = (char*) malloc( source.size() );
    memcpy( buffer, source.c_str(), source.size() );
    return buffer;
};
```

### Customize heap allocation

**TODO**

## TODO:

* Use a context to keep track of the module, class names, much like in LuaBridge. The context owns a pointer to the host Wren object, delegates registration to it.
* Use FixedVector in `Method::operator( Args... )` to close out any possible slow allocations. Size determined during compile time using `sizeof...( Args )`.
* Consistency: `executeModule` should use `Wren::loadModuleFn`
