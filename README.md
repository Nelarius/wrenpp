
# wrenly

A C++ wrapper for the [Wren programming language](http://munificent.github.io/wren/). As the language itself and this library are both heavily WIP, expect everything in here to change.

The goals of this library are
* provide a RAII wrapper for the Wren virtual machine
* to wrap the method call function
* to generate automatic wrappers for free functions and methods implementing Wren foreign methods
* to generate automatic wrappers for classes implementing Wren foreign classes
* template-based -- no macros!

Currently developing against `wren:master@f9d1e99`.

## Build

Checkout the repository using `git clone https://github.com/nelarius/wrenly.git`. The easiest way to build the project is to include contents of the `src/` folder in your project. Just remember to compile with C++14 features turned on!

To build a stand-alone static library, you can use premake to generate your build scripts. Just remember to include the path to `wren.h`:

```sh
premake5 vs2015 --wren=<path-to-wren.h>
```

The build script will be located in `build/`, and the library in `lib/`. 

## Getting started

Here's how you would initialize an instance of the virtual machine and execute from a module:

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
    .bindGetter< decltype(Vec3::x), &Vec3::x >( false, "x" )
    .bindSetter< declType(Vec3::x), &Vec3::x >( false, "x=(_)" )
    .bindGetter< decltype(Vec3::y), &Vec3::y >( false, "y" )
    .bindSetter< declType(Vec3::y), &Vec3::y >( false, "y=(_)" )
    .bindGetter< decltype(Vec3::z), &Vec3::z >( false, "z" )
    .bindSetter< declType(Vec3::z), &Vec3::z >( false, "z=(_)" );
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
      .bindGetter< decltype(Vec3::x), &Vec3::x >( false, "x" )
      .bindSetter< declType(Vec3::x), &Vec3::x >( false, "x=(_)" )
      .bindGetter< decltype(Vec3::y), &Vec3::y >( false, "y" )
      .bindSetter< declType(Vec3::y), &Vec3::y >( false, "y=(_)" )
      .bindGetter< decltype(Vec3::z), &Vec3::z >( false, "z" )
      .bindSetter< declType(Vec3::z), &Vec3::z >( false, "z=(_)" )
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

You can register a free function taking nothing but a pointer to the VM (called a CFunction, after Lua) to "manually" implement a foreign method. Why would we want to do this?

The problem with `cross(_)` within `Vec3` is that it should return a new instance of `Vec3` -- and we need to do that in C++, and return it to Wren. Currently, there's no way of doing that via Wren's C API. So here's a horribly hacky way of doing that, using a CFunction.

```cpp
// somewhere, in a source file far, far away...

extern "C" {
  #include <wren.h>
}
// we need get the foreign arguments from the `cross(_)` invocation
// and do the cross product ourselves, and then return it to Wren
void cross( WrenVM* vm ) {
  const Vec3* lhs = (const Vec3*)wrenGetArgumentForeign( vm, 0 );
  const Vec3* rhs = (const Vec3*)wrenGetArgumentForeign( vm, 1 );
  Vec3 res = lhs->cross( *rhs );
  WrenValue* ret = nullptr;
  WrenValue* constructor  = wrenGetMethod( vm, "main, "createVector3", "call(_,_,_)" );
  wrenCall( vm, constructor, &ret, "ddd", res.x, res.y, res.z );
  wrenReturnValue( vm, ret );
  wrenReleaseValue( vm, ret );
}
```

Finally, here's what our full binding code for `Vec3` now looks like.

```cpp
  wrenly::beginModule( "main" )
    .bindClass< math::Vector3f, float, float, float >( "Vec3" )
      .bindMethod< decltype(Vec3::norm), &Vec3::norm >( false, "norm()" )
      .bindMethod< decltype(Vec3::dot), &Vec3::dot >( false, "dot(_)" )
      .bindCFunction( false, "cross(_)", cross )
    .endClass()
  .endModule();
```

### Foreign classes as foreign method arguments

Note that a class bound to Wren as a foreign class can be passed to foreign functions as an argument. It may be passed as a pointer, reference, or by value, `const`, or not.

### Foreign method return values

Note that only the following types can be returned to Wren: `int` (and anything which can be cast to `int`), `const char*`, `std::string`, `bool`, `float`, `double`. A foreign method implementor may be left `void`, of course.

This will hopefully change in the future!

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

* Another major shortcoming is non-ability to store references to instances.
* A compile-time method must be devised to assert that a type is registered with Wren. Use static assert, so incorrect code isn't even compiled!
  * For instance, two separate `Type`s. One is used for registration, which iterates `Type` as well. This doesn't work in the case that the user registers different types for multiple `Wren` instances.
* I need to be able to receive the return value of a foreign method, and return that from `operator()( Args... args ).`
  * Ideally there would be a wrapper for WrenValue, which might be null.
* There needs to be better error handling for not finding a method.
  * Is Wren actually responsible for crashing the program when a method is not found?
