#include "detail/ChunkAllocator.h"
#include "Wren++.h"
#include <cstdint>
#include <cstring>
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

std::uint32_t nextPowerOf2(std::uint32_t n) {
    // if given 0, this returns 0
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;
    return n;
}

void* writeGuardBytes(void* memory, std::size_t blockSize, std::uint32_t guard) {
    std::uint32_t* intPtr = (std::uint32_t*)memory;
    assert(blockSize % 4 == 0u);
    intPtr[0] = guard;
    intPtr[(blockSize / 4u) - 1u] = guard;
    // return the memory pointer starting after the guard byte
    return intPtr + 1u;
}

void* writeHeader(void* memory, std::size_t blockSize) {
    std::uint32_t* header = (std::uint32_t*)memory;
    *header = std::uint32_t(blockSize);
    return static_cast<std::uint8_t*>(memory) + sizeof(std::uint32_t);
}

}

#ifdef DEBUG
#define markAllocBytes(memory, numBytes) std::memset(memory, 0xa5, numBytes)
#define markFreeBytes(memory, numBytes) std::memset(memory, 0xee, numBytes)
#else
#define markAllocBytes(memory, bytes)
#define markFreeBytes(memory, bytes)
#endif

#define BEEFCAFE 0xfecaefbe

namespace wrenpp {
namespace detail {

ChunkAllocator::~ChunkAllocator() {
    // TODO: freeFn is out of scope by the time this gets called :/
    /*for (auto& block : chunks_) {
        VM::freeFn(block.memory);
    }
    chunks_.clear();*/
}

void ChunkAllocator::getNewChunk_() {
    void* memory = VM::allocateFn(VM::chunkSize);
    assert(memory);
    chunks_.emplace_back(memory, VM::chunkSize, 0u);
}

// TODO: maybe we should only request bytes within std::uint32_t
// we don't want a single alloc to be larger than 2 GB
void* ChunkAllocator::alloc(std::size_t requestedBytes) {

    if (chunks_.empty()) {
        getNewChunk_();
    }

    std::uint32_t blockSize = requestedBytes;
    //we need space for the header, as well as the guard bytes. Inflate the block size accordingly
    blockSize += HeaderBytes_ + GuardBytes_;
    blockSize = blockSize < MinimumBlockSize_ ? MinimumBlockSize_ : blockSize;
    // we want all blocks to be powers of two in size to reduce fragmentation
    // the extra memory could be used in a realloc
    std::uint32_t actualBytes = nextPowerOf2(blockSize);
    assert(VM::chunkSize >= actualBytes);
    assert(actualBytes >= sizeof(FreeBlock));
    assert(actualBytes - HeaderBytes_ - GuardBytes_ >= requestedBytes);

    // fetch the memory from the free list
    // TODO
    // try to get the memory from a free block first
    //auto* freeBlock = freeBlocks_;
    //while (freeBlocks_ != nullptr) {
    //    // if the requested block size doesn't fit in this free block, then try the next
    //    if (freeBlock->size < actualBytes) {
    //        freeBlock = freeBlock->next;
    //        continue;
    //    }
    //    // fetch block from the free block list here
    //}

    // else, we need to fetch it from the top of our chunk
    auto& chunk = chunks_.back();
    if (chunk.size - chunk.currentOffset < actualBytes) {
        // TODO: stick the remainder of the block into the free list
        getNewChunk_();
        chunk = chunks_.back();
    }
    void* memory = (std::uint8_t*)chunk.memory + chunk.currentOffset;
    chunk.currentOffset += actualBytes;
    memory = writeGuardBytes(memory, actualBytes, BEEFCAFE);
    memory = writeHeader(memory, actualBytes);
    markAllocBytes(memory, actualBytes - GuardBytes_ - HeaderBytes_);

    return memory;
}

void* ChunkAllocator::realloc(void* memory, std::size_t bytes) {
    if (memory == nullptr && bytes == 0u) {
        return nullptr;
    }
    if (memory == nullptr) {
        return alloc(bytes);
    }
    if (memory && bytes == 0u) {
        free(memory);
        return nullptr;
    }
    std::uint32_t availableSize = *((std::uint32_t*)memory - 1u) - HeaderBytes_ - GuardBytes_;
    if (availableSize >= bytes) {
        return memory;
    }
    void* newMem = alloc(bytes);
    std::memcpy(newMem, memory, bytes);
    free(memory);
    return newMem;
}

void ChunkAllocator::free(void* memory) {
    if (!memory) {
        nullptr;
    }
    std::uint32_t* intPtr = (std::uint32_t*)memory;
    std::uint32_t blockSize = *(intPtr - 1u);
    intPtr -= 2u;
    assert(blockSize >= sizeof(FreeBlock));
    assert(*intPtr == BEEFCAFE);
    assert(*(intPtr + blockSize / 4u - 1u) == BEEFCAFE);
    markFreeBytes(intPtr, blockSize); // the block size includes the guard bytes as well!
    // memory block goes into the free list here
}

std::size_t ChunkAllocator::currentMemorySize() const {
    std::size_t res = 0u;
    for (auto block : chunks_) {
        res += block.size;
    }
    return res;
}

}
}
