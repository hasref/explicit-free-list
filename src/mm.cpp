#include "mm.h"

#include <fmt/format.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "memlib.h"

// TODO: Look into using -fvisibility here: https://gcc.gnu.org/wiki/Visibility

/*
 * Terminology:
 * Block Size is always a multiple of DOUBLE_SIZE.
 *
 * Block: <Header><Actual/User Block><Footer>
 *
 * The first and last blocks have special headers/footers:
 * First: <8/1><Empty i.e. size 0><8/1> (format of header/footer: <size of
 * block/allocated?>)
 *
 * Last: only header, no footer
 * <0/1>
 */

constexpr std::size_t WORD_SIZE = 4;
constexpr std::size_t DOUBLE_SIZE = 8;         /* double word size*/
constexpr std::size_t CHUNK_SIZE = (1 << 12);  // 4 KB
static std::byte* heap_listp = nullptr;        /* pointer to first block */

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
  return block_ptr + (get_blksize(block_ptr - WORD_SIZE));
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
// forward declarations
static std::byte* extend_heap(std::size_t words);
static void place(std::byte* bp, std::size_t asize);
static std::byte* find_fit(std::size_t asize);
static std::byte* coalesce(std::byte* bp);
static void printblock(std::byte* bp);
static void checkheap(int verbose);
static void checkblock(std::byte* bp);

/*
 * Initilialize the allocator.
 */
int mm_init() {
  mem_init();
  if ((heap_listp = mem_sbrk(4 * WORD_SIZE)) == nullptr) {
    return -1;
  }
  /* TODO: not sure what this is? */
  put_uvalue_at(heap_listp, 0);
  // create special first header block (prologue)
  put_uvalue_at(heap_listp + WORD_SIZE, pack(DOUBLE_SIZE, true));
  put_uvalue_at(heap_listp + 2 * WORD_SIZE, pack(DOUBLE_SIZE, true));
  // create special last header (epilogue)
  put_uvalue_at(heap_listp + 3 * WORD_SIZE, pack(0, true));
  heap_listp +=
      2 * WORD_SIZE;  // this points to the epilogue of the first header

  if (extend_heap(CHUNK_SIZE / WORD_SIZE) == nullptr) {
    return -1;
  }
  return 0;
}

/*
 * Allocates size bytes and returns a pointer to the beginning of the block.
 *
 * @param size number of bytes to allocate.
 *
 * @return pointer to the first byte of the allocated block. If the allocator
 * runs out of memory, this function will return null.
 *
 */
std::byte* mm_malloc(std::size_t size) {
  std::size_t asize; /* Adjusted Block Size i.e. including header and footer*/
  std::size_t extendsize; /* if no blocks fit, extend heap by this size */
  std::byte* block_ptr = nullptr;

  // needs init
  if (heap_listp == nullptr) {
    mm_init();
  }

  if (size == 0) {
    return nullptr;
  }

  if (size <= DOUBLE_SIZE) {
    asize = 2 * DOUBLE_SIZE;  // header + footer == DOUBLE_SIZE
  } else {
    // TODO: recheck this - this could be wrong

    // size of headers
    std::size_t actual_reqsize = size + 2 * WORD_SIZE;
    // round up to next multiple of DOUBLE_SIZE
    asize = actual_reqsize + (DOUBLE_SIZE - actual_reqsize % DOUBLE_SIZE);

    // this official version does not make sense:
    // size = 9
    // 8 * ((9+ 8 + 7) / 8) = 24
    // but size = 10
    // 8 * ( (10 + 8 + 7) / 8) = 25 which is not a multiple of 16
  }

  // find fit
  if ((block_ptr = find_fit(asize)) != nullptr) {
    place(block_ptr, asize);
    return block_ptr;
  }

  // if no fit found, extend heap
  extendsize = max(asize, CHUNK_SIZE);
  // if extending heap fails (extend_heaps takes the number of words to extend
  // by)
  if ((block_ptr = extend_heap(extendsize / WORD_SIZE)) == nullptr) {
    return nullptr;
  }
  // otherwise
  place(block_ptr, asize);
  return block_ptr;
}

/*
 * Free allocated memory.
 *
 * @param block_ptr pointer to start of block. block_ptr must point to a
 * block allocated by this allocator.
 *
 */
void mm_free(std::byte* block_ptr) {
  if (block_ptr == 0) {
    return;
  }

  std::size_t size = get_blksize(get_header_ptr(block_ptr));

  // needs init
  if (heap_listp == nullptr) {
    mm_init();
  }
  // free block by setting allocated bit to 0.
  put_uvalue_at(get_header_ptr(block_ptr), pack(size, false));
  put_uvalue_at(get_footer_ptr(block_ptr), pack(size, false));
  coalesce(block_ptr);
}

/*
 * Coalesce free blocks around a given block.
 *
 * @param block_ptr Pointer to block to coalesce.
 *
 * @return ptr to coalesced block which may be the same as block_ptr
 * in case both the previous and next blocks are allocated.
 *
 */
static std::byte* coalesce(std::byte* block_ptr) {
  // is previous block allocated
  bool prev_allocated =
      get_allocated(get_footer_ptr(get_prevblk_ptr(block_ptr)));
  bool next_allocated =
      get_allocated(get_header_ptr(get_nextblk_ptr(block_ptr)));

  std::size_t coalesced_blksize = get_blksize(get_header_ptr(block_ptr));

  // case 1: current and previous are allocated
  if (prev_allocated && next_allocated) {
    return block_ptr;
  } else if (prev_allocated && !next_allocated) {
    coalesced_blksize +=
        get_blksize(get_header_ptr(get_nextblk_ptr(block_ptr)));

    put_uvalue_at(get_header_ptr(block_ptr), pack(coalesced_blksize, false));
    // TODO: This also seems to be wrong in the book
    put_uvalue_at(get_footer_ptr(get_nextblk_ptr(block_ptr)),
                  pack(coalesced_blksize, false));
  } else if (!prev_allocated && next_allocated) {
    coalesced_blksize +=
        get_blksize(get_header_ptr(get_prevblk_ptr(block_ptr)));

    std::byte* prev_blkptr = get_prevblk_ptr(block_ptr);

    put_uvalue_at(get_footer_ptr(block_ptr), pack(coalesced_blksize, false));
    put_uvalue_at(get_header_ptr(get_prevblk_ptr(block_ptr)),
                  pack(coalesced_blksize, false));

    block_ptr = prev_blkptr;
  } else {
    coalesced_blksize +=
        get_blksize(get_header_ptr(get_prevblk_ptr(block_ptr))) +
        get_blksize(get_header_ptr(get_nextblk_ptr(block_ptr)));

    std::byte* prev_blkptr = get_prevblk_ptr(block_ptr);

    put_uvalue_at(get_header_ptr(get_prevblk_ptr(block_ptr)),
                  pack(coalesced_blksize, false));
    put_uvalue_at(get_footer_ptr(get_nextblk_ptr(block_ptr)),
                  pack(coalesced_blksize, false));

    block_ptr = prev_blkptr;
  }

  return block_ptr;
}

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
std::byte* mm_realloc(std::byte* block_ptr, std::size_t size) {
  if (size == 0) {
    mm_free(block_ptr);
    return nullptr;
  }

  if (block_ptr == nullptr) {
    return mm_malloc(size);
  }

  // naive reallocation - just get a new block without checking if coalescing is
  // possbile.
  // TODO: Implement coalescing.
  //
  std::byte* new_blkptr = mm_malloc(size);

  if (!new_blkptr) {
    return nullptr;
  }

  std::size_t oldsize = get_blksize(get_header_ptr(block_ptr));
  // if requested size is smaller, the current block may be potentially
  // truncated. Otherwise, memcpy current contents of block and free it.
  if (size < oldsize) {
    oldsize = size;
  }
  std::memcpy(new_blkptr, block_ptr, oldsize);
  mm_free(block_ptr);

  return new_blkptr;
}

/*
 * Checks heap for correctness.
 */
void mm_checkheap(int verbose) { checkheap(verbose); }

/*
 * Extend heap by creating a new block of size "words" * WORD_SIZE bytes.
 *
 * @param words Number of words to extend the heap by.
 * @return pointer to the new block.
 */
static std::byte* extend_heap(std::size_t words) {
  std::size_t size =
      (words % 2 == 0) ? words * WORD_SIZE : (words + 1) * WORD_SIZE;

  std::byte* block_ptr = nullptr;

  if ((block_ptr = mem_sbrk(size)) == nullptr) {
    return nullptr;
  }

  // initialize the new block

  // overwrite prev free list epilogue
  put_uvalue_at(get_header_ptr(block_ptr), pack(size, false));
  put_uvalue_at(get_footer_ptr(block_ptr), pack(size, false));
  // create new epilogue
  put_uvalue_at(get_header_ptr(get_nextblk_ptr(block_ptr)), pack(0, true));

  return coalesce(block_ptr);
}

/*
 * Place block of asize bytes at start of a free block "block_ptr"
 * and split block if remainder would be at least minimum block size.
 *
 * @param block_ptr Pointer to free block
 * @param asize block size
 */
static void place(std::byte* block_ptr, std::size_t asize) {
  std::size_t curr_size = get_blksize(get_header_ptr(block_ptr));

  if ((curr_size - asize) >= (2 * DOUBLE_SIZE)) {
    put_uvalue_at(get_header_ptr(block_ptr), pack(asize, true));
    put_uvalue_at(get_footer_ptr(block_ptr), pack(asize, true));

    block_ptr = get_nextblk_ptr(block_ptr);

    put_uvalue_at(get_header_ptr(block_ptr), pack(curr_size - asize, false));
    put_uvalue_at(get_footer_ptr(block_ptr), pack(curr_size - asize, false));
  } else {
    put_uvalue_at(get_header_ptr(block_ptr), pack(curr_size, true));
    put_uvalue_at(get_footer_ptr(block_ptr), pack(curr_size, true));
  }
}

/*
 * Finds a fitting block for asize by walking the list from the beginning.
 * Returns the first block that can contain asize bytes.
 *
 * @param asize Minimum of the block to find fit for.
 *
 * @return pointer to block. Returns nullptr is no fitting block is found.
 */
static std::byte* find_fit(std::size_t asize) {
  std::byte* block_ptr;
  for (block_ptr = heap_listp; get_blksize(get_header_ptr(block_ptr)) > 0;
       block_ptr = get_nextblk_ptr(block_ptr)) {
    if (!get_allocated(get_header_ptr(block_ptr)) &&
        (asize <= get_blksize(get_header_ptr(block_ptr)))) {
      return block_ptr;
    }
  }
  return nullptr;
}

/*
 * print contents of block
 */
static void printblock(std::byte* block_ptr) {
  std::size_t hsize, halloc, fsize, falloc;

  checkheap(0);

  hsize = get_blksize(get_header_ptr(block_ptr));
  halloc = get_allocated(get_header_ptr(block_ptr));
  fsize = get_blksize(get_footer_ptr(block_ptr));
  falloc = get_allocated(get_footer_ptr(block_ptr));

  if (hsize == 0) {
    fmt::print("{}: EOL\n", fmt::ptr(block_ptr));
    return;
  }

  fmt::print("{}: header: [{}:{}], footer: [{}:{}]", fmt::ptr(block_ptr), hsize,
             (halloc ? 'a' : 'f'), fsize, (falloc ? 'a' : 'f'));
}

static void checkblock(std::byte* block_ptr) {
  bool is_aligned =
      (reinterpret_cast<std::uintptr_t>(block_ptr) % DOUBLE_SIZE == 0);

  if (!is_aligned) {
    fmt::print("Error: {} is not doubleword algined\n", fmt::ptr(block_ptr));
  }

  if (get_uat(get_header_ptr(block_ptr)) !=
      get_uat(get_footer_ptr(block_ptr))) {
    fmt::print("Error: header does not match footer\n");
  }
}

void checkheap(int verbose) {
  std::byte* block_ptr = heap_listp;

  if (verbose) {
    fmt::print("Heap ({}):\n", fmt::ptr(heap_listp));
  }

  if ((get_blksize(get_header_ptr(block_ptr))) != DOUBLE_SIZE ||
      !get_allocated(get_header_ptr(heap_listp))) {
    fmt::print("Bad prologue header\n");
  }
  checkblock(heap_listp);

  for (block_ptr = heap_listp; get_blksize(get_header_ptr(block_ptr)) > 0;
       block_ptr = get_nextblk_ptr(block_ptr)) {
    if (verbose) {
      printblock(block_ptr);
    }

    checkblock(block_ptr);
  }

  if (verbose) {
    printblock(block_ptr);
  }
  if ((get_blksize(get_header_ptr(block_ptr))) != 0 ||
      !(get_allocated(get_header_ptr(block_ptr)))) {
    fmt::print("Bad epilogue header\n");
  }
}

void mm_teardown() { mem_teardown(); }
