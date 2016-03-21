#include "Wren++.h"
#include <chrono>
#include <cstdlib>

int main() {
    const int Repeat = 100;
    float sum = 0.f;
    for (int i = 0; i < Repeat; ++i) {
        auto start = std::chrono::steady_clock::now();
        wrenpp::VM vm{};
        vm.executeModule("binary_trees");
        auto end = std::chrono::steady_clock::now();
        auto diff = end - start;
        sum += std::chrono::duration<float>(diff).count();
    }
    printf("\n\nExecution took %f seconds, on average", sum / Repeat);

    return 0;
}
