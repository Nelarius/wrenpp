
#include "Wrenly.h"
#include <iostream>

void testMethodCall() {
    wrenly::Wren wren{};

    wren.executeModule("test");
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

int main() {

    testMethodCall();

    return 0;
}
