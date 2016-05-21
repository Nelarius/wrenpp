#include "detail/ChunkAllocator.h"
#include "Wren++.h"
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

ChunkAllocator::ChunkAllocator()
    : chunks_{},
    freeListHead_{ nullptr } {
    getNewChunk_();
}

ChunkAllocator::ChunkAllocator(ChunkAllocator&& other)
    : chunks_{ std::move(other.chunks_) },
    freeListHead_{ other.freeListHead_ } {
    other.freeListHead_ = nullptr;
}

ChunkAllocator& ChunkAllocator::operator=(ChunkAllocator&& rhs) {
    chunks_ = std::move(rhs.chunks_);
    freeListHead_ = rhs.freeListHead_;
    rhs.freeListHead_ = nullptr;
    return *this;
}

ChunkAllocator::~ChunkAllocator() {
    for (auto& block : chunks_) {
        VM::freeFn(block.memory);
    }
    chunks_.clear();
}

void ChunkAllocator::getNewChunk_() {
    void* memory = VM::allocateFn(VM::chunkSize);
    assert(memory);
    chunks_.push_back(Block{ memory, VM::chunkSize, 0u });
}

void* ChunkAllocator::prepareMemory_(void* memory, std::uint32_t bytes) {
    memory = writeGuardBytes(memory, bytes, BEEFCAFE);
    memory = writeHeader(memory, bytes);
    markAllocBytes(memory, bytes - GuardBytes_ - HeaderBytes_);
    return memory;
}

// TODO: maybe we should only request bytes within std::uint32_t
// we don't want a single alloc to be larger than 2 GB
void* ChunkAllocator::alloc(std::size_t requestedBytes) {
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
    FreeBlock* curBlock = freeListHead_;
    FreeBlock* prevBlock = nullptr;
    while (curBlock) {
        if (curBlock->size - GuardBytes_ - HeaderBytes_ < requestedBytes) {
            prevBlock = curBlock;
            curBlock = curBlock->next;
            continue;
        }
        if (prevBlock) {
            prevBlock->next = curBlock->next;
        }
        else {
            freeListHead_ = freeListHead_->next;
        }
        void* memory = curBlock;
        std::uint32_t size = curBlock->size;
        return prepareMemory_(memory, size);
    }

    std::uintptr_t remainder = chunks_.back().size - chunks_.back().currentOffset;
    if (remainder < actualBytes) {
        if (remainder >= MinimumBlockSize_) {
            auto& chunk = chunks_.back();
            addToFreeList_((std::uint8_t*)chunk.memory + chunk.currentOffset, remainder);
        }
        getNewChunk_();
    }
    auto& chunk = chunks_.back();
    void* memory = (std::uint8_t*)chunk.memory + chunk.currentOffset;
    chunk.currentOffset += actualBytes;
    return prepareMemory_(memory, actualBytes);
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
        return;
    }
    std::uint32_t* intPtr = (std::uint32_t*)memory;
    std::uint32_t blockSize = *(intPtr - 1u);
    intPtr -= 2u;
    assert(blockSize >= sizeof(FreeBlock));
    assert(*intPtr == BEEFCAFE);
    assert(*(intPtr + blockSize / 4u - 1u) == BEEFCAFE);
    markFreeBytes(intPtr, blockSize); // the block size includes the guard bytes as well!
    addToFreeList_(intPtr, blockSize);
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
