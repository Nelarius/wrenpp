
class Assert {
    static abortCurrentFiber(msg) {
        Fiber.abort("assertion error: %(msg)")
    }

    static isTrue(expr) {
        if (expr != true) {
            abortCurrentFiber("isTrue")
        }
    }

    static isFalse(expr) {
        if (expr != false) {
            abortCurrentFiber("isFalse")
        }
    }

    static isEqual(a, b) {
        if (a != b) {
            abortCurrentFiber("isEqual, values are %(a) and %(b)")
        }
    }

    static notEqual(a, b ) {
        if (a == b) {
            abortCurrentFiber("notEqual, values are %(a) and %(b)")
        }
    }

    static succeed(callable) {
        var fiber = Fiber.new {
            callable.call()
        }
        fiber.try()
        if (fiber.error != null) {
            abortCurrentFiber("%(callable) aborts with error %(fiber.error)")
        }
    }

    static fail(callable) {
        var fiber = Fiber.new {
            callable.call()
        }
        fiber.try()
        if (fiber.error == null) {
            abortCurrentFiber("%(callable) doesn't abort")
        }
    }
}
