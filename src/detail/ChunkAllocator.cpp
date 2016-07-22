#include "detail/ChunkAllocator.h"
#include "Wren++.h"
#include <algorithm>
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

void* writeGuardBytes(void* memory, std::size_t blockSize, std::uint32_t guard) {
    std::uint32_t* intPtr = (std::uint32_t*)memory;
    assert(blockSize % 4 == 0u);
    intPtr[0u] = guard;
    intPtr[(blockSize / 4u) - 1u] = guard;
    // return the memory pointer starting after the guard byte
    return intPtr + 1u;
}

void* writeHeader(void* memory, std::size_t blockSize) {
    std::size_t* header = (std::size_t*)memory;
    *header = blockSize;
    return static_cast<std::uint8_t*>(memory) + sizeof(std::size_t);
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

ChunkAllocator::ChunkAllocator() {
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
    stats_.heapSize += VM::chunkSize;
    void* memory = VM::allocateFn(VM::chunkSize);
    assert(memory);
    chunks_.push_back(Block{ memory, VM::chunkSize, 0u });
}

void* ChunkAllocator::prepareMemory_(void* memory, std::size_t bytes) {
    memory = writeGuardBytes(memory, bytes, BEEFCAFE);
    memory = writeHeader(memory, bytes);
    markAllocBytes(memory, bytes - GuardBytes_ - HeaderBytes_);
    return memory;
}

void* ChunkAllocator::alloc(std::size_t requestedBytes) {
    std::size_t blockSize = requestedBytes;
    //we need space for the header, as well as the guard bytes. Inflate the block size accordingly
    blockSize += HeaderBytes_ + GuardBytes_;
    blockSize = std::max(MinimumBlockSize_, blockSize);
    // we want all blocks to be powers of two in size to reduce fragmentation
    // the extra memory could be used in a realloc

    std::size_t actualBytes = nextPowerOf2(std::uint64_t(blockSize));
    assert(VM::chunkSize >= actualBytes);
    assert(actualBytes >= sizeof(FreeBlock));
    assert(actualBytes - HeaderBytes_ - GuardBytes_ >= requestedBytes);

    stats_.allocCount++;
    stats_.usedMemory += actualBytes;
    if (stats_.allocCount > stats_.allocCountAtLargest) {
        stats_.allocCountAtLargest = stats_.allocCount;
    }
    if (stats_.usedMemory > stats_.usedMemoryAtLargest) {
        stats_.usedMemoryAtLargest = stats_.usedMemory;
    }

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
        std::size_t size = curBlock->size;
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
    std::size_t availableSize = *((std::size_t*)memory - 1u) - HeaderBytes_ - GuardBytes_;
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
    std::size_t blockSize = *reinterpret_cast<std::size_t*>(intPtr - sizeof(std::size_t) / 4u);
    intPtr -= 1u + sizeof(std::size_t) / 4u;
    assert(blockSize >= sizeof(FreeBlock));
    assert(*intPtr == BEEFCAFE);
    // the size of the block minus one uint32 is where the last guard byte resides
    assert(*(intPtr + blockSize / 4u - 1u) == BEEFCAFE);
    markFreeBytes(intPtr, blockSize);

    // the following algorithm adds the newly freed block into the free list
    // it tries to merge adjacent free blocks into a larger block
    std::uintptr_t blockStart = reinterpret_cast<std::uintptr_t>(intPtr);
    std::uintptr_t blockEnd = blockStart + blockSize;
    FreeBlock* curBlock = freeListHead_;
    FreeBlock* prevBlock = nullptr;
    while (curBlock != nullptr) {
        if (reinterpret_cast<std::uintptr_t>(curBlock) >= blockEnd) {
            break;
        }
        prevBlock = curBlock;
        curBlock = curBlock->next;
    }

    // freeListHead_ was null or the next element was null
    if (prevBlock == nullptr) {
        prevBlock = reinterpret_cast<FreeBlock*>(blockStart);
        prevBlock->size = blockSize;
        prevBlock->next = freeListHead_;
        freeListHead_ = prevBlock;
        stats_.freeBlocks++;
    }
    // prevBlock is adjacent to the block being freed
    else if (reinterpret_cast<std::uintptr_t>(prevBlock) + prevBlock->size == blockStart) {
        prevBlock->size += blockSize;
    }
    // insert a new block in between curBlock and prevBlock
    // set the new block as prevBlock in preparation to check if the following
    // block is adjacent to the new block
    else {
        FreeBlock* newBlock = reinterpret_cast<FreeBlock*>(blockStart);
        newBlock->next = prevBlock->next;
        newBlock->size = blockSize;
        prevBlock->next = newBlock;
        prevBlock = newBlock;
        stats_.freeBlocks++;
    }

    if (stats_.freeBlocks > stats_.freeBlocksAtLargest) {
        stats_.freeBlocksAtLargest = stats_.freeBlocks;
    }

    // check if curBlock is adjacent to the freed block
    if (curBlock != nullptr && reinterpret_cast<std::uintptr_t>(curBlock) == blockEnd) {
        // we merge curBlock into prevBlock
        prevBlock->size += curBlock->size;
        prevBlock->next = curBlock->next;
        assert(stats_.freeBlocks != 0u);
        stats_.freeBlocks--;
    }

    assert(stats_.allocCount != 0u);
    stats_.allocCount--;
    stats_.usedMemory -= blockSize;
}

}
}
