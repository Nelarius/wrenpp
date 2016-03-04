import "test" for TestRunner
import "assert" for Assert
import "vector" for Vec3

var testRunner = TestRunner.new()

var v1 = Vec3.new(1.0, 2.0, 1.0)
var v2 = Vec3.new(2.0, 1.0, 2.0)

testRunner.test("Number should be 1.0", Fn.new {
    Assert.isEqual(v1.x, 1.0)
})

var dot = v1.dot(v2)

testRunner.test("Number should be 6.0", Fn.new{
    Assert.isNear(dot, 6.0, 0.000001)
})
