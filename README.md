
# wren++

A C++ wrapper for the [Wren programming language](http://munificent.github.io/wren/). As the language itself and this library are both heavily WIP, expect everything in here to change.

Wrenly currently provides:
- a RAII wrapper for the Wren virtual machine
- convenient access to calling Wren class methods from C++
- automatic wrapping code generation for any C++ functions and classes
- template-based -- no macros!

Current defincies:
- not that type-safe. It's pretty undefined what happens when you try to bind code that returns a type which hasn't itself been bound
- Wrenly has no concept of const-ness. When you return a const pointer to one of your instances from a bound function, the resulting foreign object in Wren will happily call non-const methods -- yikes!
- Wren access from C++ is rather minimal

Currently developing against `wren:master@139b447`. This project is being developed at the whim of my [game engine](https://github.com/nelarius/playground) project.
## Build

Clone the repository using `git clone https://github.com/nelarius/wrenly.git`. The easiest way to build the project is to include the contents of the `src/` folder in your project, since there's so little code. Just remember to compile with C++14 features turned on!

Alternatively, you can build the static library with premake:

```sh
premake5 vs2015 --include=<path to wren.h>
```

If you want to build the tests as well, then you need to include the location of wren's `lib/` folder.

```sh
premake5 vs2015 --include=<path to wren.h> --link=<path to wren/lib>
``` 

## At a glance

Let's fire up an instance of the Wren VM and execute some code:

```cpp
#include "Wrenly.h"

int main() {
  wrenly::Wren vm{};
  vm.executeString( "System.print(\"Hello, world!\")" );

  return 0;
}
```

The virtual machine is held internally in a pointer. `Wren` uniquely owns the virtual machine, which means that the `Wren` instance can't be copied, but can be moved when needed - just like a unique pointer.

Module names work the same way by default as the module names of the Wren command line module. You specify the location of the module without the `.wren` postfix.

```cpp
vm.executeModule( "script" );   // refers to script.wren
```

Both `executeString` and `executeModule` return a code indicating if the interpretation encountered any errors.

```cpp
wrenly::Result res = wrenly.executeString(
  "// calling nonexistent variable"
  "foobar.call()"
);
if ( res == wrenly::Result::CompileError ) {
  std::cout << "foobar doesn't exist and a compilation error occurs.\n";
}
```

There are two other codes: `Result::RuntimeError` and `Result::Success`.

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

**TODO:** improve

Wrenly allows you to bind C++ functions and methods to Wren classes. This is done by storing the names or signatures along with the function pointers, so that Wren can find them. That is, you bind all your code once, after which all your Wren instances can use them.

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
  wrenly::beginModule( "math" )
    .beginClass( "Math" )
      .bindFunction< decltype(&cos), &cos >( true, "cos(_)" )
      .bindFunction< decltype(&sin), &sin >( true, "sin(_)" )
      .bindFunction< decltype(&tan), &tan >( true, "tan(_)" )
      .bindFunction< decltype(&exp), &exp >( true, "exp(_)" )
    .endClass()
  .endModule();
        
  wren.executeString( "import \"math\" for Math\nSystem.print( Math.cos(0.12345) )" );
    
  return 0;
}
```

Both the type of the function (in the case of `cos` the type is `double(double)`, for instance, and could be used instead of `decltype(&cos)`) and the reference to the function have to be provided to `bindFunction` as template arguments. As arguments, `bindFunction` needs to be provided with a boolean which is true, when the foreign method is static, false otherwise. Finally, the method signature is passed.

> The free function needs to call functions like `wrenGetArgumentDouble`, `wrenGetArgumentString` to access the arguments passed to the method. When you register the free function, Wrenly wraps the free function and generates the appropriate `wrenGetArgument*` function calls during compile time. Similarly, if a function returns a value, the call to the appropriate `wrenReturn*` function is inserted at compile time.

### Foreign classes

Free functions don't get us very far if we want there to be some state on a per-object basis. Foreign classes can be registered by using `bindClass` on a module context. Let's look at an example. Say we have the following Wren class representing a 3-vector:

```dart
foreign class Vec3 {
  construct new( x, y, z ) {}

  foreign norm()
  foreign dot( rhs )
  foreign cross( rhs )    // returns the result as a new vector

  // accessors
  foreign x
  foreign x=( rhs )
  foreign y
  foreign y=( rhs )
  foreign z
  foreign z=( rhs )
}

/*
 * This is a creator object for the C++ API to use in order to create foreign objects
 * and return them back to Wren with the required state
 */

var createVector3 = Fn.new { | x, y, z |
  return Vec3.new( x, y, z )
}
```

We would like to implement it using the following C++ struct.

```cpp
struct Vec3 {
  union {
    float v[3];
    struct { float x, y, z; };
  };
  
  Vec3( float x, float y, float z )
  : v{ x, y, z }
    {}

  float norm() const {
    return sqrt( x*x + y*y + z*z );
  }

  float dot( const Vec3& rhs ) const {
    return x*rhs.x + y*rhs.y + z*rhs.z;
  }
  
  Vec3 cross( const Vec3& rhs ) const {
    return Vec3 {
      y*rhs.z - z*rhs.y,
      z*rhs.x - x*rhs.z,
      x*rhs.y - y*rhs.x
    };
  }
};
```

Let's start by binding the class to Wren and adding properties.

A class is bound by writing `bindClass` instead of `beginClass`. This binds the specified constructor and destructor to Wren, and allocates a new instance of your class within Wren.

```cpp
#include "Wrenly.h"

int main() {
  wrenly::Wren wren{};
  wrenly::beginModule( "main" )
    .bindClass< Vec3, float, float, float >( "Vec3" );
    // you can now construct Vec3 in Wren

  return 0;
}
```

Pass the class type, and constructor argument types to `bindClass`. Even though a C++ class may have many constructors, only one constructor can be registered with Wren.

#### Properties

If your class or struct has public fields you wish to expose, you can do so by using `bindGetter` and `bindSetter`. This will automatically generate a function which returns the value of the field to Wren.

```cpp
wrenly::beginModule( "main" )
  .bindClass< Vec3, float, float, float >( "Vec3" )
    .bindGetter< decltype(&Vec3::x), &Vec3::x >( false, "x" )
    .bindSetter< decltype(&Vec3::x), &Vec3::x >( false, "x=(_)" )
    .bindGetter< decltype(&Vec3::y), &Vec3::y >( false, "y" )
    .bindSetter< decltype(&Vec3::y), &Vec3::y >( false, "y=(_)" )
    .bindGetter< decltype(&Vec3::z), &Vec3::z >( false, "z" )
    .bindSetter< decltype(&Vec3::z), &Vec3::z >( false, "z=(_)" );
```

#### Methods

Using `registerMethod` allows you to bind a class method to a Wren foreign method. Just do:

```cpp
#include "Wrenly.h"

int main() {
  wrenly::Wren wren{};
  wrenly::beginModule( "main" )
    .bindClass< Vec3, float, float, float >( "Vec3" )
      // properties
      .bindGetter< decltype(&Vec3::x), &Vec3::x >( false, "x" )
      .bindSetter< decltype(&Vec3::x), &Vec3::x >( false, "x=(_)" )
      .bindGetter< decltype(&Vec3::y), &Vec3::y >( false, "y" )
      .bindSetter< decltype(&Vec3::y), &Vec3::y >( false, "y=(_)" )
      .bindGetter< decltype(&Vec3::z), &Vec3::z >( false, "z" )
      .bindSetter< decltype(&Vec3::z), &Vec3::z >( false, "z=(_)" )
      // methods
      .bindMethod< decltype(Vec3::norm), &Vec3::norm >( false, "norm()" )
      .bindMethod< decltype(Vec3::dot), &Vec3::dot >( false, "dot(_)" )
    .endClass()
  .endModule();

  return 0;
}
```

The arguments are the same as what you pass `bindFunction`, but as the template parameters pass the method type and pointer instead of a function.

We've now implemented two of `Vec3`'s three foreign functions -- what about the last foreign method, `cross(_)` ?

### CFunctions

Wrenly let's you bind functions of the type `WrenForeignMethodFn`, typedefed in `wren.h`, directly. They're called CFunctions for brevity (and because of Lua). Sometimes it's convenient to wrap a collection of C++ code manually. This happens when the C++ library interface doesn't match Wren classes that well. Let's take a look at binding the excellent [dear imgui](https://github.com/ocornut/imgui) library to Wren.

Many functions to ImGui are very overloaded and have lengthy signatures. We would have to fully qualify the function pointers, which would make the automatic bindings a mess. Additionally, many ImGui functions (such as SliderFloat) take in pointers to primitive types, like float, bool. Wren doesn't have any concept of `out` parameters, so we will make our Wren ImGui API take in a number by value, and return the new value.

```dart
class Imgui {

  // windows
  foreign static begin( name )    // begin a window scope
  foreign static end()            // end a window scope

  foreign static sliderFloat( label, value, min, max )  // RETURNS the new value
}
```

First, let's implement wrappers for `ImGui::Begin` and `ImGui::SliderFloat` with reasonable default values.

```cpp
void begin(WrenVM* vm) {
  ImGui::Begin((const char*)wrenGetSlotString(vm, 1), NULL, 0);
}

void sliderFloat(WrenVM* vm) {
  const char* label = wrenGetSlotString(vm, 1);
  float value = float(wrenGetSlotDouble(vm, 2));
  float min =   float(wrenGetSlotDouble(vm, 3));
  float max =   float(wrenGetSlotDouble(vm, 4));
  ImGui::SliderFloat(label, &value, min, max);
  wrenSetSlotDouble(vm, 0, value);
}
```

Here's what the binding code looks like. Note that ImGui::End is trivial to bind as it takes no arguments.

```cpp
wrenly::beginModule( "builtin/imgui" )
  .beginClass( "Imgui" )
    // windows & their formatting
    .bindCFunction( true, "begin(_)", wren::begin )
    .bindFunction< decltype(&ImGui::End), &ImGui::End>( true, "end()" )
    .bindCFunction( true, "sliderFloat(_,_,_,_)", wren::sliderFloat)
  .endClass();
```

If you need to access and return foreign object instances within your CFunction, you can use the following two helper functions.

Use `wrenly::getForeignSlotPtr<T, int>(WrenVM*)` to get a bound type from the slot API:

```cpp
void setWindowSize(WrenVM* vm) {
  ImGui::SetNextWindowSize(*(const ImVec2*)wrenly::getForeignSlotPtr<Vec2i, 1>(vm));
}
```

Use `wrenly::setForeignSlotValue<T>(WrenVM*, const T&)` and `wrenly::setForeignSlotPtr<T>(WrenVM*, T* obj)` to place an object with foreign bytes in slot 0, by value and by reference, respectively. `wrenly::setForeignSlotValue<T>` uses the type's copy constructor to copy the object into the new value.

### C++ and Wren lifetimes

If the return type of a bound method or function is a reference or pointer to an object, then the returned wren object will have C++ lifetime, and Wren will not garbage collect the object pointed to. If an object is returned by value, then a new instance of the object is also constructed withing the returned Wren object. In this situation, the returned Wren object has Wren lifetime and is garbage collected.

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

### Customize heap allocation & garbage collection

You can use your own allocation/free functions assigning your them to the `Wren::allocateFn` and `Wren::freeFn` callback functions. By default, they are just wrappers around the system malloc/free:

```cpp
Wren::allocateFn = []( std::size_t bytes ) -> void* {
  return malloc( bytes );
};

Wren::freeFn = []( void* memory ) -> void {
  free(memory);
};
```

These functions are used by the Wren VM itself to allocate a block of memory to use. These bindings allocate an additional (small) block of memory in order to store foreign objects. Here are the other variables which control how the heap is used.

The initial heap size is the number of bytes Wren will allocate before triggering the first garbage collection. By default, it's 10 MiB.

`wrenly::Wren::initialHeapSize = 0xA00000u;`

The heap size in bytes, below which collections will stop. The idea of the minimum heap size is to avoid miniscule heap growth (calculated based on the percentage of heap growth, explained next) and thus frequent collections. By default, the minimum heap size is 1 MiB.

`wrenly::Wren::minHeapSize = 0x100000u;`

After a collection occurs, Wren will allow the heap to grow to (100 + heapGrowthPercent) % of the current heap size before the next collection occurs. By default, the heap growth percentage is 50 %.

`wrenly::Wren::heapGrowthPercent = 50;`

## TODO:

* Assert when an object pointer is misaligned
* Return registered foreign classes by value, and by reference
* Add function to get the value returned by the wren method.
  * A Value class is needed. Has template methods to get the actual value on the Wren stack.
* A compile-time method must be devised to assert that a type is registered with Wren. Use static assert, so incorrect code isn't even compiled!
  * For instance, two separate `Type`s. One is used for registration, which iterates `Type` as well. This doesn't work in the case that the user registers different types for multiple `Wren` instances.
* I need to be able to receive the return value of a foreign method, and return that from `operator()( Args... args ).`
  * Ideally there would be a wrapper for WrenValue, which might be null.
* There needs to be better error handling for not finding a method.
  * Is Wren actually responsible for crashing the program when a method is not found?
* Does Wren actually crash the program when an invalid operator is used on a class instance?
* Make Wren::collectGarbage less verbodes
