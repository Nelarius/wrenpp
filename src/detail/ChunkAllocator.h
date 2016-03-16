#ifndef CHUNKALLOCATOR_H_INCLUDED
#define CHUNKALLOCATOR_H_INCLUDED

#include <vector>
#include <cstdlib>

namespace wrenpp {
namespace detail {

// A page allocator. It uses the provided allocation/free functions to allocate
// one big block of memory at a time.
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

private:

    struct FreeBlock {
        std::uint32_t size;
        FreeBlock*  next;
    };

    struct Block {

        Block(void* m, std::size_t b, std::uintptr_t c)
            : memory{ m }, blockSize{ b }, currentOffset{ c } {}

        void*          memory;
        std::uintptr_t blockSize;
        std::uintptr_t currentOffset;
    };

    void getNewChunk_();

    const std::size_t   MinimumBlockSize{ sizeof(FreeBlock) };

    std::vector<Block>  chunks_{};
    FreeBlock*          freeBlocks_{ nullptr };
};

}
}

#endif // CHUNKALLOCATOR_H_INCLUDED
