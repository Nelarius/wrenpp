import "test" for TestRunner
import "assert" for Assert
import "vector" for Vec3

class VectorReference {
    foreign static get()
}

var testRunner = TestRunner.new()

var v1 = Vec3.new(1.0, 3.0, 5.0)
var v2 = Vec3.new(2.0, 4.0, 6.0)

testRunner.test("Number should be 1.0", Fn.new {
    Assert.isEqual(v1.x, 1.0)
})

var dot = v1.dot(v2)

var epsilon = 0.000001

testRunner.test("Number should be 44.0", Fn.new{
    Assert.isNear(dot, 44.0, epsilon)
})

var v3 = v1.plus(v2)

testRunner.test("Number should be 3.0", Fn.new {
    Assert.isNear(v3.x, 3.0, epsilon)
})

testRunner.test("Number should be 3.0", Fn.new {
    Assert.isNear(v3.y, 7.0, epsilon)
})

testRunner.test("Number should be 11.0", Fn.new {
    Assert.isNear(v3.z, 11.0, epsilon)
})

var v4 = VectorReference.get()

testRunner.test("Number should be 2.0", Fn.new {
    Assert.isEqual(v4.x, 2.0)
})

VectorReference.get().x = 10.0

testRunner.test("Number should be 10.0", Fn.new {
    Assert.isEqual(v4.x, 10.0)
})
