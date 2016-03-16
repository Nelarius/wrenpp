#include "detail/ChunkAllocator.h"
#include "Wren++.h"
#include <cstdint>
#include <limits>

namespace {

std::uint64_t nextPowerOf2(std::uint64_t n) {
    // if given 0, this returns 0
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    n++;
    return n;
}

void* writeHeader(void* memory, std::size_t blockSize) {
    std::uint32_t* header = (std::uint32_t*)memory;
    *header = std::uint32_t(blockSize);
    return static_cast<std::uint8_t*>(memory) + sizeof(std::uint32_t);
}

}

namespace wrenpp {
namespace detail {

ChunkAllocator::~ChunkAllocator() {
    for (auto& block : chunks_) {
        VM::freeFn(block.memory);
    }
    chunks_.clear();
}

void ChunkAllocator::getNewChunk_() {
    void* memory = VM::allocateFn(VM::chunkSize);
    assert(memory);
    chunks_.emplace_back(memory, VM::chunkSize, 0u);
}

void* ChunkAllocator::alloc(std::size_t bytes) {
    assert(bytes < std::numeric_limits<std::uint32_t>::max());
    if (chunks_.empty()) {
        getNewChunk_();
    }
    // calculate the block size that we want to reserve
    // the block size is the requested bytes plus 4 bytes which are going to be used to
    // store the actual size of the block
    std::size_t nextPowerOfTwo = nextPowerOf2(std::uint64_t(bytes + 4u));
    std::size_t requestedSize = nextPowerOfTwo < MinimumBlockSize ? MinimumBlockSize : nextPowerOfTwo;

    // try to get the memory from a free block first
    auto* freeBlock = freeBlocks_;
    while (freeBlocks_ != nullptr) {
        // if the requested block size doesn't fit in this free block, then try the next
        if (freeBlock->size < requestedSize) {
            freeBlock = freeBlock->next;
            continue;
        }
        // fetch block from the free block list here
    }

    // we need to fetch the memory from the chunk
    auto& block = chunks_.back();
    if (block.blockSize - block.currentOffset < requestedSize) {
        // then we need to fetch from the next block
        // TODO: stick a new entry into the free list here for this block
        getNewChunk_();
        block = chunks_.back();
    }

    void* memory = (std::uint8_t*)memory + block.currentOffset;
    block.currentOffset += requestedSize;
    // writes the block size into the header, and returns the accordingly offsetted pointer
    return writeHeader(memory, requestedSize);
}

void* ChunkAllocator::realloc(void* memory, std::size_t bytes) {
    if (memory == nullptr) {
        return alloc(bytes);
    }
    if (memory && bytes == 0u) {
        free(memory);
    }
    // realloc logic goes here
}

void ChunkAllocator::free(void* memory) {
    std::uint32_t* header = reinterpret_cast<std::uint32_t*>(memory) - 1u;
    //
}

}
}
