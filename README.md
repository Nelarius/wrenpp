
# wrenly

A C++ wrapper for the [Wren programming language](http://munificent.github.io/wren/). As the language itself and this library are both heavily WIP, expect everything in here to change.

The goals of this library are
* to wrap the Wren VM in a nice, easy-to-use class
* to wrap the Wren method call 
* to implement a wrapper to bind free functions to foreign function implementations
* and by far the biggest task: to bind classes to Wren foreign classes -- unimplemented at the moment
* template-based - no macros!

## Building

**TODO**

The code has been currently tested on Linux and Windows, with MinGW-w64.

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

You can implement a Wren foreign method as a stateless free function in C++. Wrenly offers an easy to use wrapper over the functions. Note that only primitive types and `std::string` work for now. Support for registered types will be provided later once the custom type registration feature is complete.

Here's how you could implement a simple math library in Wren by binding the C++ standard math library functions.

math.wren:
```dart
class Math {
    foreign static cos( x )
    foreign static sin( x )
	foreign static tan( x )
    foreign static exp( x )
}
```
main.cpp:
```cpp
#include "Wrenly.h"
#include <cmath>

int main( int argc, char** argv ) {

    wrenly::Wren wren{};
    wren.beginModule( "math" )
        .beginClass( "Math" )
            .registerFunction< decltype(cos), cos >( true, "cos(_)" )
            .registerFunction< decltype(sin), sin >( true, "sin(_)" )
            .registerFunction< decltype(tan), tan >( true, "tan(_)" )
            .registerFunction< decltype(exp), exp >( true, "exp(_)" );
            
    wren.executeString( "import \"math\" for Math\nIO.print( Math.cos(0.12345) )" );
    
    return 0;
}
```

Both the type of the function (in the case of `cos` the type is `double(double)`, for instance, and could be used instead of `decltype(cos)`) and the reference to the function have to be provided to `registerFunction` as tempalte arguments. As arguments, `registerFunction` needs to be provided with a boolean which is true, when the foreign method is static, false otherwise. Finally, the method signature is passed.

> The free function needs to call functions like `wrenGetArgumentDouble`, `wrenGetArgumentString` to access the arguments passed to the method. When you register the free function, Wrenly wraps the free function and generates the appropriate `wrenGetArgument*` function calls during compile time. Similarly, if a function returns a value, the call to the appropriate `wrenReturn*` function is inserted at compile time.

### Foreign classes

**TODO**

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

* BREAKING WREN CHANGE: `WrenMethod` no longer exist, use `WrenValue` instead.
* Add the wren repository to this one to resolve the dependency issue.
* Makefile compiles static library
* Use FixedVector in `Method::operator( Args... )` to close out any possible slow allocations. Size determined during compile time using `sizeof...( Args )`.
* Consistency: `executeModule` should use `Wren::loadModuleFn`
