#ifndef CHUNKALLOCATOR_H_INCLUDED
#define CHUNKALLOCATOR_H_INCLUDED

#include <vector>
#include <cstdlib>

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
    ChunkAllocator()                                 = default;
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

        Block(void* m, std::size_t b, std::uintptr_t c)
            : memory{ m }, size{ b }, currentOffset{ c } {}

        void*          memory;
        std::uintptr_t size;
        std::uintptr_t currentOffset;
    };

    void getNewChunk_();

    const std::uint32_t GuardBytes_{ 2u * sizeof(std::uint32_t) };
    const std::uint32_t HeaderBytes_{ sizeof(std::uint32_t) };
    const std::size_t   MinimumBlockSize_{ sizeof(FreeBlock) };

    std::vector<Block>  chunks_{};
    FreeBlock*          freeBlocks_{ nullptr };
};

}
}

#endif // CHUNKALLOCATOR_H_INCLUDED
