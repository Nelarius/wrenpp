
import "assert" for Assert

class Test {
    construct new( name, body ) {
        _name = name
        _fiber = Fiber.new {
            body.call()
        }
    }

    try() {
        _fiber.try()
    }
    abort(msg) {
        _fiber.abort(msg)
    }
    error { _fiber.error }
    name { _name }
}

class TestRunner {

    construct new() {
        _tests = []
    }

    add( test, body ) {
        _tests.add( Fiber.new {
            body.call()
        })
    }

    test(name, body) {
        var t = Test.new(name, body)
        t.try()
        if (t.error) {
            System.print("Test \"%(name)\" failed with %(fiber.error)")
        } else {
            System.print("Test \"%(name)\" OK")
        }
    }

    run() {
        var failedCount = 0
        for ( t in _tests) {
            t.try()
            if (t.error) {
                failedCount = failedCount + 1
                System.print("Test\"%(t.name)\" failed with %(t.error)")
            }
        }
        if (failedCount > 0) {
            System.print("%(failedCount) tests out of %(_tests.count) failed.")
        } else {
            System.print("%(_tests.count) tests passed.")
        }
    }
}

var testRunner = TestRunner.new()

/***
 *      _______        _       
 *     |__   __|      | |      
 *        | | ___  ___| |_ ___ 
 *        | |/ _ \/ __| __/ __|
 *        | |  __/\__ \ |_\__ \
 *        |_|\___||___/\__|___/
 *                             
 *                             
 */

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
