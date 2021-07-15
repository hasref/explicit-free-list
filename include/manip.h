#ifndef BITS_H_
#define BITS_H_

#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include "memlib.h"

// TODO: Look into using -fvisibility here: https://gcc.gnu.org/wiki/Visibility

/*
 * Terminology:
 * Block Size is always a multiple of DOUBLE_SIZE.
 * Block: <Header><Actual/User Block><Footer>
 *
 * The first and last blocks have special headers/footers:
 * First: <8/1><Empty i.e. size 0><8/1> (format of header/footer: <size of
 * block/allocated?>)
 *
 * Last: only header, no footer
 * <0/1>
 */

constexpr int WORD_SIZE = 4;
constexpr int DOUBLE_SIZE = 8; /* double word size*/
constexpr int CHUNKSIZE = (1 << 12);
static std::byte* heap_listp = nullptr; /* pointer to first block */

template <typename T>
bool max(T x, T y) {
  return x > y ? x : y;
}

/*
 * pack size and alloc into a single value using bitwise 'or' so as to create a
 * block header.
 *
 * @param size size of the header. This must be a multiple of 8.
 * @param alloc is either 0 (false) or 1(true) indicating whether the block is
 * allocated.
 */
inline uint32_t pack(uint32_t size, uint32_t alloc) { return (size | alloc); }

/*
 * pack size and alloc into a single value using bitwise 'or' so as to create a
 * block header.
 *
 * @param size size of the header. This must be a multiple of 8.
 * @param alloc indicates whether the block is allocated.
 */
inline uint32_t pack(uint32_t size, bool alloc) {
  return (size | static_cast<uint32_t>(alloc));
}

/*
 * Read 32 bits at pointer and return as uint32_t.
 *
 * @param pointer Pointer to read uint32_t at.
 *
 * @return uint32_t value at pointer (since headers are 32 bits and pointer
 * is expected to point to a free list header).
 */
inline uint32_t get_uat(std::byte* pointer) {
  return *reinterpret_cast<uint32_t*>(pointer);
}

/*
 * Put 32 bits at pointer.
 *
 * @param pointer pointer to put value at.
 * @param value Value to put at pointer.
 *
 * Expected: pointer will usually point to a free list header.
 */
inline void put_uvalue_at(std::byte* pointer, uint32_t value) {
  *reinterpret_cast<uint32_t*>(pointer) = value;
}

/*
 * get size from header
 *
 * @param header_ptr pointer to a free list header.
 * @return size of the memory block.
 */
inline uint32_t get_blksize(std::byte* header_ptr) {
  // ~0x7: bit mask that returns the most significant 29 bits (i.e.
  // disregarding the last three bits) since header is MSB 29.
  return get_uat(header_ptr) & ~0x7;
}

/*
 * Is the current block allocated?
 *
 * @param header_ptr pointer to a free list header.
 * @return boolean indicating if the current block is allocated.
 */
inline bool get_allocated(std::byte* header_ptr) {
  // last bit of header indicates where allocated or not.
  return static_cast<bool>(get_uat(header_ptr) & 0x1);
}

/*
 * Given a block ptr (one handed out to a user), determine the address
 * of the blocks header and footer.
 *
 * @param block_ptr pointer to start of (user) block i.e. after header.
 * @return pointer of type std::byte to start of block header.
 */

inline std::byte* get_header_ptr(std::byte* block_ptr) {
  return block_ptr - WORD_SIZE;
}

/*
 * Given a block_ptr (one handed out to user), returns a pointer to
 * the footer of the block.
 *
 * @param block_ptr pointer to start of (user) block i.e. after header.
 * @return pointer of type std::byte to start of the block's footer.
 *
 */
inline std::byte* get_footer_ptr(std::byte* block_ptr) {
  std::byte* header_ptr = get_header_ptr(block_ptr);
  // -DOUBLE_SIZE since block_size = header_size(4 bytes) + actual block size +
  // footer_size(4 bytes)
  return block_ptr + get_blksize(header_ptr) - DOUBLE_SIZE;
}

/*
 * Get pointer to the start of the next (user) block given a pointer to a (user)
 * block.
 *
 * @param block_ptr pointer to a (user) block i.e. after header.
 * @return pointer to the start of next (user) block.
 *
 */
inline std::byte* get_nextblk_ptr(std::byte* block_ptr) {
  return block_ptr + (get_blksize(block_ptr) - WORD_SIZE);
}

/*
 * Get pointer to previous (user) block given a (user) block pointer.
 *
 * @param block_ptr pointer to a user block.
 * @return pointer to the start of previous (user) block.
 */
inline std::byte* get_prevblk_ptr(std::byte* block_ptr) {
  // block_ptr - DOUBLE_SIZE is previous block's footer
  return block_ptr - get_blksize(block_ptr - DOUBLE_SIZE);
}

/*
 * Initilialize the allocator.
 */
int mm_init() {
  if ((heap_listp = mem_sbrk(4 * WORD_SIZE)) == nullptr) {
    return -1;
  }
  /* alignment padding */
  put_uvalue_at(heap_listp, 0);
  // create special first header block (prologue)
  put_uvalue_at(heap_listp + WORD_SIZE, pack(DOUBLE_SIZE, true));
  put_uvalue_at(heap_listp + 2 * WORD_SIZE, pack(DOUBLE_SIZE, true));
  // create special last header (epilogue)
  put_uvalue_at(heap_listp + 3 * WORD_SIZE, pack(0, true));
  heap_listp += 2 * WORD_SIZE;
}

#endif
