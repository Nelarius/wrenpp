
# wrenly

A C++ wrapper for the Wren programming language. As the language itself and this library are both heavily WIP, expect nothing to work. This will eventually be a library, which you can link to.

Requires a C++14 compliant compiler.

## Getting started

The Wren virtual machine is contained in the `Wren` class. The class is reference-counted, so class instances can be passed around safely. Modules can be executed by calling `wrenly::Wren::executeModule( const std::string& )`. Module names work the same way by default as the module names of the Wren command line module. You specify the location of the module without the `.wren` postfix.

Here's a minimal program which executes a small Wren module.


```cpp
#include "Wren.h"

int main() {
  wrenly::Wren wren{};
  wren.executeModule( "hello" );
  
  return 0;
}
```

with `hello.wren`,

```dart
IO.print( "Hello from wren!" )
```

outputs

```
> Hello from wren!
```

## Accessing Wren from C++



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

* Resolve file name collision between `Wren.h` and `wren.h` on Windows.
* add `executeString` ,ethod to `Wren`
* Add foreign method support.
  * Registered foreign methods need to be placed in some kind of tree.
  * `ForeignMethodFnWrapper` needs to use a global pointer to bound method tree
  * The Wren VM binds its own instance of the tree to the global variable on module execution
  * Functions & methods need to be wrapped in a global method, maybe with templates?
* Add `WrenMethod` handle
  * Should it be a reference counted type? 
  * Simple wrapper over the `wren.h` interface
  * Use variadic templates on operator() 
* Consistency: `executeModule` should use `Wren::loadModuleFn`

Here's a small example of how the method handle should work. Wren source in `say.wren`:

```dart
class Foo {
  static say(text) {
    IO.print(\"Foo.say() called with: \", text)
  }
}
``` 

```c
WrenInterpretResult result = wrenInterpret(vm, "say.wren",  file.c_str() );
assert(result == WREN_RESULT_SUCCESS);
WrenMethod *method = wrenGetMethod(vm, "main", "Foo", "say(_)");
wrenCall(vm, method, "s", "Howdy!"); // "Foo.say() called with: Howdy!"
wrenReleaseMethod(vm, method);
```
