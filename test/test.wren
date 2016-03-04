
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
