
# wrenly

A C++ wrapper for the [Wren programming language](http://munificent.github.io/wren/). As the language itself and this library are both heavily WIP, expect everything in here to change.

The goals of this library are
* to wrap the Wren VM in a nice, easy-to-use class
* to wrap the Wren method call 
* to implement a wrapper to bind free functions to foreign function implementations
* and by far the biggest task: to bind classes to Wren foreign classes -- unimplemented at the moment
* template-based - no macros!

## Building

Checkout the repository using `git clone --recursive https://github.com/nelarius/wrenly.git`.

**TODO** CMake

The code has been currently tested on Linux and Windows, with MinGW-w64.

## Getting started

Here's how you would initialize an instance of the virtual machine and execute a module:

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

int main() {

    wrenly::Wren wren{};
    wren.beginModule( "math" )
        .beginClass( "Math" )
            .registerFunction< decltype(&cos), &cos >( true, "cos(_)" )
            .registerFunction< decltype(&sin), &sin >( true, "sin(_)" )
            .registerFunction< decltype(&tan), &tan >( true, "tan(_)" )
            .registerFunction< decltype(&exp), &exp >( true, "exp(_)" );
            
    wren.executeString( "import \"math\" for Math\nIO.print( Math.cos(0.12345) )" );
    
    return 0;
}
```

Both the type of the function (in the case of `cos` the type is `double(double)`, for instance, and could be used instead of `decltype(cos)`) and the reference to the function have to be provided to `registerFunction` as template arguments. As arguments, `registerFunction` needs to be provided with a boolean which is true, when the foreign method is static, false otherwise. Finally, the method signature is passed.

> The free function needs to call functions like `wrenGetArgumentDouble`, `wrenGetArgumentString` to access the arguments passed to the method. When you register the free function, Wrenly wraps the free function and generates the appropriate `wrenGetArgument*` function calls during compile time. Similarly, if a function returns a value, the call to the appropriate `wrenReturn*` function is inserted at compile time.

### Foreign classes

Foreign classes can be registered by `registerClass` on a module context.

```cpp
#include "Wrenly.h"

struct Test {
  Test() = default;
  Test( double xx ) : x( xx ) {}
  
  private:
    double x{ 0.0 };
};

int main() {
  wrenly::Wren wren{};
  wren.beginModule( "main" )
    .registerClass< Test, double >( "Test" );

  return 0;
}
```

Pass the class type, and constructor argument types to `registerClass`. Even though a C++ class may have many constructors, only one constructor can be registered with Wren.

**TODO: all the rest**

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

* Perhaps a `Value` class would be useful, instead of `Method`. It would behave like a Wren value, and you could call it using similar syntax to Wren (`call` method).
* Use FixedVector in `Method::operator( Args... )` to close out any possible slow allocations. Size determined during compile time using `sizeof...( Args )`.
* Consistency: `executeModule` should use `Wren::loadModuleFn`
* Allow registration of custom types
  * store the type ids in a set within `Wren`. Remember to move the set in the move constructors & assignment operators!
  * a function of type `WrenBindForeignClassMethods`, which is `WrenForeignClassMethods(WrenVM*, const char*, const char*)` must return the pointer to the struct `WrenForeignClassMethods` containing two `WrenForeignMethodFn`s.
  * store the pointers to `WrenForeignClassMethods` in an unordered_map, where the key is again a hash.
  * are called like foreign methods? Can I call the constructor I want from there?
* The contexts need to be independent of `Wren`. Methods and classes will be registered globally. Thus there will be two trees of WrenForeignMethodFn.
* A compile-time method must be devised to assert that a type is registered with Wren. Use static assert, so incorrect code isn't even compiled!
  * For instance, two separate `Type`s. One is used for registration, which iterates `Type` as well. This doesn't work in the case that the user registeres different types for multiple `Wren` instances.