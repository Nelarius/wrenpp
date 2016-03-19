#ifndef CHUNKALLOCATOR_H_INCLUDED
#define CHUNKALLOCATOR_H_INCLUDED

#include <vector>
#include <cstdlib>
#include <cassert>

namespace wrenpp {
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
    ChunkAllocator(const ChunkAllocator&)            = delete;
    ChunkAllocator(ChunkAllocator&&)                 = delete;
    ChunkAllocator& operator=(const ChunkAllocator&) = delete;
    ChunkAllocator& operator=(ChunkAllocator&&)      = delete;
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
        void*          memory;
        std::uintptr_t size;
        std::uintptr_t currentOffset;
    };

    void* prepareMemory_(void*, std::uint32_t);
    void getNewChunk_();
    inline void addToFreeList_(void* memory, std::uint32_t size) {
        FreeBlock* freeBlock = reinterpret_cast<FreeBlock*>(memory);
        freeBlock->size = size;
        if (freeListHead_) {
            freeBlock->next = freeListHead_;
        }
        else {
            freeBlock->next = nullptr;
        }
        freeListHead_ = freeBlock;
    }

    const std::uint32_t GuardBytes_{ 2u * sizeof(std::uint32_t) };
    const std::uint32_t HeaderBytes_{ sizeof(std::uint32_t) };
    const std::size_t   MinimumBlockSize_{ sizeof(FreeBlock) };

    std::vector<Block>  chunks_{};
    FreeBlock*          freeListHead_{ nullptr };
};

}
}

#endif // CHUNKALLOCATOR_H_INCLUDED
