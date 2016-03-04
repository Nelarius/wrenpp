import "test" for TestRunner
import "assert" for Assert

var testRunner = TestRunner.new()

var passNumber = Fn.new { |x|
    testRunner.test("Number should be five", Fn.new {
        Assert.isEqual(x, 5.0)
    })
}

var passBool = Fn.new { |x|
    testRunner.test("Bool should be true", Fn.new {
        Assert.isTrue(x)
    })
}

var passString = Fn.new { |str|
    testRunner.test("String should be \"hello\"", Fn.new {
        Assert.isEqual(str, "hello")
    })
}
