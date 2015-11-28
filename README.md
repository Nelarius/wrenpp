
# wrenly

A C++ wrapper for the [Wren programming language](http://munificent.github.io/wren/). As the language itself and this library are both heavily WIP, expect everything in here to change.

The goals of this library are
* provide a RAII wrapper for the Wren virtual machine
* to wrap the method call function
* to generate automatic wrappers for free functions and methods implementing Wren foreign methods
* to generate automatic wrappers for classes implementing Wren foreign classes
* template-based -- no macros!

Currently developing against `wren:master@2ff8acb`.

## Build

Checkout the repository using `git clone https://github.com/nelarius/wrenly.git`.

The easiest way to build the project is to include contents of the `src/` folder in your project. Just remember to compile with C++14 features turned on!

You can also use CMake to build the project. CMake tries to find Wren, and stores the result in the `WREN_INCLUDE_DIR` variable. If you don't have Wren installed in a system-wide location, you can help CMake by specifying the variable on the command line. For example, you could write:

```sh
mkdir build
cd build
cmake -DWREN_INCLUDE_DIR=<location of wren.h here> ../
make
```

A premake5 build script is also included, if you prefer using it over CMake. The resulting build files are placed in `build/<action>/`. Use the `--wren=<location of wren.h>` option generate the build files with the wren header location.

The code has been built on Linux using gcc, and Windows using MinGW-w64.

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
wren.executeString( "System.print(\"Hello from a C++ string!\")" );
```

## Accessing Wren from C++

### Methods

You can use the `Wren` instance to get a callable handle for a method implemented in Wren. Given the following class, defined in `bar.wren`, 

```dart
class Foo {
  static say( text ) {
    System.print( text )
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
            
    wren.executeString( "import \"math\" for Math\nSystem.print( Math.cos(0.12345) )" );
    
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

  inline double get() { return x; }  
  
  private:
    double x{ 0.0 };
};

int main() {
  wrenly::Wren wren{};
  wren.beginModule( "main" )
    .registerClass< Test, double >( "Test" )
      .registerMethod< decltype( &Test::get ), &Test::get >( false, "get()" );

  return 0;
}
```

Pass the class type, and constructor argument types to `registerClass`. Even though a C++ class may have many constructors, only one constructor can be registered with Wren.

Methods are registered in a similar way to free functions. You simply call `registerMethod` on the registered class context. The arguments are the same as what you pass `registerFunction`.

>When you define a foreign class in Wren, and bind it using wrenly, what actually happens? In Wren, the C representation of a foreign class object contains an array of bytes. It is up to the embedder to decide what kind of state they want to store in the foreign class' byte array. Wrenly uses the byte array to store an instance of the registered class, and nothing more.

### Foreign method arguments

Note that a class registered with Wren as a foreign class can be passed to foreign functions as an argument. It may be passed as a pointer, reference, or by value, `const`, or not.

### Foreign method return values

Note that only the following types can be returned to Wren: `int` (and anything which can be cast to `int`), `const char*`, `std::string`, `bool`, `float`, `double`. A foreign method implementor may be left `void`, of course.

## Customize VM behavior

### Customize `System.print`

The Wren scripting language does not implement `System.print( ... )` itself, but expects the embedder to provide it. The `Wren` wrapper class does exactly that with the static `Wren::writeFn` field, which is of type `std::function< void( WrenVM*, const char* ) >`. By default, it is implemented as follows.

```cpp
Wren::writeFn = []( WrenVM* vm, const char* text ) -> void { printf( text ) };
```

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

* Investigate whether it is possible to use "properties" to access C-like struct members.
* Another major shortcoming is non-ability to store references to instances.
* Add the ability for wren to call user-defined implementor function directly (no wrapper).
* Consistency: `executeModule` should use `Wren::loadModuleFn`
* The contexts need to be independent of `Wren`. Methods and classes will be registered globally. Thus there will be two trees of WrenForeignMethodFn.
* A compile-time method must be devised to assert that a type is registered with Wren. Use static assert, so incorrect code isn't even compiled!
  * For instance, two separate `Type`s. One is used for registration, which iterates `Type` as well. This doesn't work in the case that the user registers different types for multiple `Wren` instances.
* I need to be able to receive the return value of a foreign method, and return that from `operator()( Args... args ).`
  * Ideally there would be a wrapper for WrenValue, which might be null.
* Divorce registration from the Wren instances.
* There needs to be better error handling for not finding a method.
  * Is Wren actually responsible for crashing the program when a method is not found?
