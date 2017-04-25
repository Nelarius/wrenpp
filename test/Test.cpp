#include "Wren++.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cassert>

// a small class to test class & method binding with
struct Vec3
{
    float x, y, z;

    Vec3(float x, float y, float z)
        : x{ x }, y{ y }, z{ z }
    {}

    Vec3(const Vec3&) = default;

    float norm() const
    {
        return std::sqrt(x*x + y*y + z*z);
    }

    float dot(const Vec3& rhs) const
    {
        return x*rhs.x + y*rhs.y + z*rhs.z;
    }

    Vec3 cross(const Vec3& rhs) const
    {
        return Vec3{
            y*rhs.z - z*rhs.y,
            z*rhs.x - x*rhs.z,
            x*rhs.y - y*rhs.x
        };
    }

    Vec3 plus(const Vec3& rhs) const
    {
        return Vec3{ x + rhs.x, y + rhs.y, z + rhs.z };
    }
};

struct Transform
{
    Transform(const Vec3& p)
        : position(p)
    { }

    Vec3 position{ 0.f, 0.f, 0.f };
};

void CFunctionVectorReference(WrenVM* vm)
{
    static Vec3 v{ 2.0, 1.0, 1.0 };
    wrenpp::setSlotForeignPtr(vm, 0, &v);
}

Vec3* returnVec3Ptr()
{
    static Vec3 v{ 1.f, 1.f, 1.f };
    return &v;
}

Vec3& returnVec3Ref()
{
    static Vec3 v{ 1.f, 1.f, 1.f };
    return v;
}

const Vec3* returnVec3ConstPtr()
{
    static Vec3 v{ 1.f, 1.f, 1.f };
    return &v;
}

const Vec3& returnVec3ConstRef()
{
    static Vec3 v{ 1.f, 1.f, 1.f };
    return v;
}

void bindVectorModule(wrenpp::VM& vm)
{
    vm.beginModule("vector")
        .bindClass<Vec3, float, float, float>("Vec3")
            .bindGetter< decltype(Vec3::x), &Vec3::x >("x")
            .bindSetter< decltype(Vec3::x), &Vec3::x >("x=(_)")
            .bindGetter< decltype(Vec3::y), &Vec3::y >("y")
            .bindSetter< decltype(Vec3::y), &Vec3::y >("y=(_)")
            .bindGetter< decltype(Vec3::z), &Vec3::z >("z")
            .bindSetter< decltype(Vec3::z), &Vec3::z >("z=(_)")
            .bindMethod< decltype(&Vec3::norm), &Vec3::norm >(false, "norm()")
            .bindMethod< decltype(&Vec3::dot), &Vec3::dot >(false, "dot(_)")
            .bindMethod< decltype(&Vec3::plus), &Vec3::plus>(false, "plus(_)")
        .endClass()
    .endModule();
    vm.beginModule("main")
        .beginClass("VectorReferences")
            .bindCFunction(true, "getCFunction()", CFunctionVectorReference)
            .bindFunction<decltype(&returnVec3Ptr), &returnVec3Ptr>(true, "getPtr()")
            .bindFunction<decltype(&returnVec3Ref), &returnVec3Ref>(true, "getRef()")
            .bindFunction<decltype(&returnVec3ConstPtr), &returnVec3ConstPtr>(true, "getConstPtr()")
            .bindFunction<decltype(&returnVec3ConstRef), returnVec3ConstRef>(true, "getConstRef()")
        .endClass()
    .endModule();
}

void bindTransformModule(wrenpp::VM& vm)
{
    vm.beginModule("transform")
        .bindClass<Transform, Vec3>("Transform")
            .bindSetter<decltype(Transform::position), &Transform::position>("position=(_)")
            .bindGetter<decltype(Transform::position), &Transform::position>("position");
}

void testMethodCall()
{
    wrenpp::VM wren{};

    wren.executeModule("test_method");
    auto passNum = wren.method("main", "passNumber", "call(_)");
    passNum(5.0);
    passNum(5.f);
    passNum(5);
    passNum(5u);

    auto passBool = wren.method("main", "passBool", "call(_)");
    passBool(true);

    auto passStr = wren.method("main", "passString", "call(_)");
    passStr("hello");
    passStr(std::string("hello"));
}

void testClassMethods()
{
    wrenpp::VM vm;
    bindVectorModule(vm);
    vm.executeModule("test_vector");
}

void testProperties()
{
    wrenpp::VM vm;
    bindVectorModule(vm);
    bindTransformModule(vm);
    vm.executeModule("test_properties");
}

void testReturnValues()
{
    wrenpp::VM vm{};

    vm.executeString(
        "var returnsThree = Fn.new {\n"
        "    return 3\n"
        "}\n"
        "var returnsTrue = Fn.new {\n"
        "    return true\n"
        "}\n"
        "var returnsGreeting = Fn.new {\n"
        "    return \"Hello, world\"\n"
        "}\n"
    );
    wrenpp::Method returnsThree = vm.method("main", "returnsThree", "call()");
    double nval = returnsThree().as<double>();
    assert(nval == 3.0);
    wrenpp::Method returnsTrue = vm.method("main", "returnsTrue", "call()");
    bool bval = returnsTrue().as<bool>();
    assert(bval == true);
    wrenpp::Method returnsGreeting = vm.method("main", "returnsGreeting", "call()");
    wrenpp::Value sval = returnsGreeting();
    assert(!strcmp("Hello, world", sval.as<const char*>()));
}

void printConstRefString(const std::string& str)
{
    std::printf("%s\n", str.c_str());
}

void printValueString(std::string str)
{
    std::printf("%s\n", str.c_str());
}

void printCharString(const char* str)
{
    std::printf("%s\n", str);
}

void testStrings()
{
    wrenpp::VM vm;

    vm.beginModule("main")
        .beginClass("StringPrinter")
            .bindFunction<decltype(&printConstRefString), printConstRefString>(true, "print1(_)")
            .bindFunction<decltype(&printValueString), printValueString>(true, "print2(_)")
            .bindFunction<decltype(&printCharString), printCharString>(true, "print3(_)")
        .endClass();

    vm.executeString(
        "class StringPrinter {\n"
        "  foreign static print1(str)\n"
        "  foreign static print2(str)\n"
        "  foreign static print3(str)\n"
        "}\n"
    );

    vm.executeString("StringPrinter.print1(\"passing by const ref works\")");
    vm.executeString("StringPrinter.print2(\"passing by value works\")");
    vm.executeString("StringPrinter.print3(\"passing as C string works\")");
}

int main()
{

    std::printf("\nCalling Wren code from C++...\n\n");

    testMethodCall();

    std::printf("\nTesting method calls...\n\n");

    testClassMethods();

    std::printf("\nTesting properties...\n\n");

    testProperties();

    std::printf("\nTesting return values...\n\n");

    testReturnValues();

    std::printf("\nTesting to see if passing string to C++ works...\n\n");

    testStrings();

    return 0;
}
