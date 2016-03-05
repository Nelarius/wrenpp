
#include "Wrenly.h"
#include <cmath>
#include <cstdlib>
#include <cstring>

// a small class to test class & method binding with
struct Vec3 {
    float x, y, z;

    Vec3(float x, float y, float z)
        : x{ x }, y{ y }, z{ z } {}

    Vec3(const Vec3&) = default;

    float norm() const {
        return std::sqrt(x*x + y*y + z*z);
    }

    float dot(const Vec3& rhs) const {
        return x*rhs.x + y*rhs.y + z*rhs.z;
    }

    Vec3 cross(const Vec3& rhs) const {
        return Vec3{
            y*rhs.z - z*rhs.y,
            z*rhs.x - x*rhs.z,
            x*rhs.y - y*rhs.x
        };
    }

    Vec3 plus(const Vec3& rhs) const {
        return Vec3{ x + rhs.x, y + rhs.y, z + rhs.z };
    }
};

void plus(WrenVM* vm) {
    Vec3* lhs = wrenly::getForeignSlotPtr<Vec3, 0>(vm);
    Vec3* rhs = wrenly::getForeignSlotPtr<Vec3, 1>(vm);
    Vec3 res = lhs->plus(*rhs);
    wrenly::setForeignSlotValue(vm, res);   // must be copyable
}

void returnVectorReference(WrenVM* vm) {
    static Vec3 v{ 2.0, 1.0, 1.0 };
    wrenly::setForeignSlotPtr(vm, &v);
}

void testMethodCall() {
    wrenly::Wren wren{};

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

void testClassMethods() {

    wrenly::beginModule("vector")
        .bindClass<Vec3, float, float, float>("Vec3")
            .bindGetter< decltype(Vec3::x), &Vec3::x >(false, "x")
            .bindSetter< decltype(Vec3::x), &Vec3::x >(false, "x=(_)")
            .bindGetter< decltype(Vec3::y), &Vec3::y >(false, "y")
            .bindSetter< decltype(Vec3::y), &Vec3::y >(false, "y=(_)")
            .bindGetter< decltype(Vec3::z), &Vec3::z >(false, "z")
            .bindSetter< decltype(Vec3::z), &Vec3::z >(false, "z=(_)")
            .bindMethod< decltype(&Vec3::norm), &Vec3::norm >(false, "norm()")
            .bindMethod< decltype(&Vec3::dot), &Vec3::dot >(false, "dot(_)")
            .bindCFunction(false, "plus(_)", plus)
        .endClass()
    .endModule();
    wrenly::beginModule("main")
        .beginClass("VectorReference")
            .bindCFunction(true, "get()", returnVectorReference)
        .endClass()
    .endModule();

    wrenly::Wren wren{};

    wren.executeModule("test_vector");
}

int main() {

    printf("\nCalling Wren code from C++...\n\n");

    testMethodCall();

    printf("\nTesting method calls...\n\n");

    testClassMethods();

    return 0;
}
