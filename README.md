
# wrenly

A C++ wrapper for the [Wren programming language](http://munificent.github.io/wren/). As the language itself and this library are both heavily WIP, expect everything in here to change.

The goals of this library are
* Wrap the Wren VM in a nice, easy-to-use class -- DONE
* Register foreign methods with the VM -- WIP
* Register foreign classes with the VM -- WIP

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

You can implement a method in a stand-alone C/C++ function. Currently, wrenly simplifies registering with the Wren virtual machine. Here's a small example.

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

The implementing function must be of type `void (*)( WrenVM* )`.

**TODO:** automate passing arguments between the vm and the function.

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

## TODO:

* Add foreign method support.
  * Boost Function Types may be the only way to extract parameter information
  * http://stackoverflow.com/questions/687490/how-do-i-expand-a-tuple-into-variadic-template-functions-arguments contains information on unpacking tuple into function arguments
  * http://www.drdobbs.com/cpp/extracting-function-parameter-and-return/240000586 some information on how to obtain a type list
* Move TypeCount to the `detail` namespace
* Consistency: `executeModule` should use `Wren::loadModuleFn`
