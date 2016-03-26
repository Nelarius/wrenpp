#ifndef CHUNKALLOCATOR_H_INCLUDED
#define CHUNKALLOCATOR_H_INCLUDED

// vector is only used for storing the pointers to the chunks
// it's assumed that there will not be many chunks, so the heap overhead
// from vector should be small
#include <vector>
#include <cstdlib>
#include <cassert>

namespace wrenpp {

struct AllocStats {
    std::size_t allocCount;
    std::size_t heapSize;
    std::size_t usedMemory;
    std::size_t freeBlocks;
};

namespace detail {

// A chunk allocator. It uses the provided allocation/free functions to allocate
// one big block of memory at a time.
//
// This allocator write certain magic numbers into memory for debugging purposes.
// Memory blocks are surrounded by the 0xBEEFCAFE magic number.
// When DEBUG is defined, allocated but uninitialized memory is set to 0xA5
// When DEBUG is defined, freed memory is set to 0xEE.
class ChunkAllocator {
public:
    ChunkAllocator();
    // TODO: this needs to be made movable, because it is owned by VM, a movable entity!
    ChunkAllocator(const ChunkAllocator&)            = delete;
    ChunkAllocator(ChunkAllocator&&);
    ChunkAllocator& operator=(const ChunkAllocator&) = delete;
    ChunkAllocator& operator=(ChunkAllocator&&);
    ~ChunkAllocator();

    void*  alloc(std::size_t bytes);
    void*  realloc(void* memory, std::size_t bytes);
    void   free(void* memory);

    std::size_t currentMemorySize() const;

private:

    struct FreeBlock {
        std::uint32_t size;
        FreeBlock*  next;
    };

    struct Block {
        void*       memory;
        std::size_t size;
        std::size_t currentOffset;
    };

    void* prepareMemory_(void*, std::uint32_t);

    void getNewChunk_();

    inline void addToFreeList_(void* memory, std::uint32_t size) {
        FreeBlock* freeBlock = reinterpret_cast<FreeBlock*>(memory);
        freeBlock->size = size;
        freeBlock->next = freeListHead_;
        freeListHead_ = freeBlock;
    }

    const std::uint32_t GuardBytes_{ 2u * sizeof(std::uint32_t) };
    const std::uint32_t HeaderBytes_{ sizeof(std::uint32_t) };
    const std::size_t   MinimumBlockSize_{ sizeof(FreeBlock) };

    std::vector<Block>      chunks_{};
    FreeBlock*              freeListHead_{ nullptr };
};

}
}

#endif // CHUNKALLOCATOR_H_INCLUDED
