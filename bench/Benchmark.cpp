#include "Wren++.h"
#include <chrono>
#include <cstdlib>

void benchmarkModule(const char* module) {
    const int Repeat = 100;
    float sum = 0.f;
    std::printf("Benchmarking %s...\n", module);
    for (int i = 0; i < Repeat; ++i) {
        wrenpp::VM vm;
        auto start = std::chrono::steady_clock::now();
        vm.executeModule(module);
        auto end = std::chrono::steady_clock::now();
        auto diff = end - start;
        sum += std::chrono::duration<float>(diff).count();
    }
    std::printf("Execution took %f seconds, on average.\n", sum / Repeat);

    wrenpp::VM vm;
    vm.executeModule(module);
    wrenpp::AllocStats stats = vm.allocStats();
    std::printf("alloc count: %u\n", stats.numAllocs);
    std::printf("free count: %u\n", stats.numFrees);
    std::printf("move count: %u\n", stats.numMoves);
    std::printf("fast realloc count: %u\n", stats.numAvailable);
    std::printf("max free list size: %u\n\n", stats.maxFreeListSize);
}

int main() {
    wrenpp::VM::writeFn = [](WrenVM* vm, const char* msg) -> void { /*silence Wren*/ };
    benchmarkModule("binary_trees");
    benchmarkModule("delta_blue");
    //benchmarkModule("map_numeric");

    return 0;
}
