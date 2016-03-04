
#include "Wrenly.h"
#include <cmath>
#include <cstdlib>

// a small class to test class & method binding with
struct Vec3 {
    float x, y, z;

    Vec3(float x, float y, float z)
        : x{ x }, y{ y }, z{ z } {}

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
            //.bindMethod< decltype(&Vec3::plus), &Vec3::plus>(false, "plus(_)")
        .endClass()
    .endModule();

    wrenly::Wren wren{};

    wren.executeModule("test_vector");
}

int main() {

    printf("Calling Wren code from C++...\n\n");

    testMethodCall();

    printf("Testing method calls...\n\n");

    testClassMethods();

    return 0;
}
