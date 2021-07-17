#ifndef MM_H_
#define MM_H_

#include <cstddef>

/*
 * Initilialize the allocator.
 */
extern int mm_init();

/*
 * Allocates size bytes and returns a pointer to the beginning of the block.
 *
 * @param size number of bytes to allocate.
 *
 * @return pointer to the first byte of the allocated block. If the allocator
 * runs out of memory, this function will return null.
 *
 */
extern std::byte* mm_malloc(std::size_t size);

/*
 * Free allocated memory.
 *
 * @param block_ptr pointer to start of block. block_ptr must point to a
 * block allocated by this allocator.
 *
 */
extern void mm_free(std::byte* ptr);

/*
 * Reallocates a block using a naive strategy (always reallocates).
 *
 * Note that this uses std::memcpy internally and therefore, the original block
 * should only contain trivially copyable types. For this reason, you probably
 * do not want to use this. Here be dragons.
 *
 * @param block_ptr Pointer to block to rellocate. If block_ptr is nullptr then
 * this is equivalent to calling mm_malloc.
 *
 * @param size New size of the block. If size is 0 then calling this
 * function is equivalent to calling mm_free(block_ptr). If size is less than
 * the current size of the block then this may truncate the block. Be careful
 * when doing this.
 *
 * @return pointer to the reallocated block. This function may return a nullptr
 * if the allocator cannot find a large enough block and is out of memory.
 */
extern std::byte* mm_realloc(std::byte* ptr, std::size_t size);

/*
 * Checks heap for correctness.
 *
 * @param verbose Prints debug info if verbose is not equal to 0.
 */
extern void mm_checkheap(int verbose);

/*
 * Free memory allocated by the allocator. This invalidates
 * all pointers handed out by the allcoator.
 */
extern void mm_teardown();

#endif
