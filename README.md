
# wrenly

A C++ wrapper for the Wren programming language. As the language itself and this library are both heavily WIP, expect nothing to work. This will eventually be a library, which you can link to.

## Getting started

The Wren virtual machine is contained in the `Wren` class. Here's how you would initialize the virtual machine and execute a module:

```cpp
#include "Wren.h"

int main() {
  wrenly::Wren wren{};
  wren.executeModule( "hello" );	// refers to the file hello.wren
  
  return 0;
}
```

`Wren` is reference-counted, and can be passed around as usual. Module names work the same way by default as the module names of the Wren command line module. You specify the location of the module without the `.wren` postfix.

## Accessing Wren from C++

#### Methods

You can use the `Wren` instance to get a callable handle for a method implemented in Wren. Given the following class, defined in `bar.wren`, 

```dart
class Foo {
  static say( text ) {
    IO.print( text )
  }
}
```

you can execute the static method `say` by writing,

```cpp
wrenly::Wren wren{};
wren.executeModule( "bar" );
    
auto say = wren.method( "main", "Foo", "say(_)" );
say( "s", "Hello from C++!" );
```

`Wren::method` has the following signature:
```cpp
Method Wren::method( 
  const std::string& module, 
  const std::string& variable,
  const std::string& signature
);
```

We used the class name `Foo` as the argument `variable`, because we were calling a static method. Normally you would use the variable name that the instance was stored in. The signature must be specified, because Wren supports function overloading by arity.

## Accessing C++ from Wren


## Customize module loading

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

## TODO:

* add `executeString` method to `Wren`
* Add foreign method support.
  * Registered foreign methods need to be placed in some kind of tree.
  * `ForeignMethodFnWrapper` needs to use a global pointer to bound method tree
  * The Wren VM binds its own instance of the tree to the global variable on module execution
  * Functions & methods need to be wrapped in a global method, maybe with templates?
* Add `WrenMethod` handle
  * figure out how to build the arg string during compile time.
* Consistency: `executeModule` should use `Wren::loadModuleFn`
