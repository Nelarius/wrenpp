import "test" for TestRunner
import "assert" for Assert
import "transform" for Transform
import "vector" for Vec3

var testRunner = TestRunner.new()

testRunner.test("Setting transform position.x should be reflected in getter", Fn.new {
    var t = Transform.new(Vec3.new(1.0, 1.0, 1.0))

    Assert.isEqual(t.position.x, 1.0)
    t.position.x = 2.0
    Assert.isEqual(t.position.x, 2.0)
})
