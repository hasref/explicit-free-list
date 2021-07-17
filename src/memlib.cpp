#include "memlib.h"

#include <fmt/core.h>

#include <cerrno>
#include <cstddef>
#include <cstdlib>

/*
 * This file defines the memory model to be used by our explicit
 * free list implementation. The author's of Computer Systems: A Programmer's
 * Perspective semeed to have chosen this in order to allow interleaving calls
 * to malloc with our own allocator.
 *
 * The memory model is essentially this: we request a large chunk of memory
 * from the system at init time and then for increasing the size of the heap,
 * keep reusing this memory. If the heap is full ( heap_max_addr == heap_start +
 * MAX_HEAP_SIZE ) then we return a nullptr from the allocator and set errno
 * to ENOMEM;
 *
 * This allocator follows a strategy similar to that followed by Apple's old
 * (~1989) sbrk function that also operated off a maximum heap size.
 */

constexpr std::size_t MAX_HEAP_SIZE = 20 * (1 << 20); /* 20 MB */

static std::byte* heap_start;    /* beginning of the heap (mem_heap)*/
static std::byte* heap_brk;      /* one past the end of the heap (mem_brk) */
static std::byte* heap_max_addr; /* The heap has a maximum size and this points
                                    one byte beyond it*/

/*
 * Initialize the memory model
 */
void mem_init() {
  heap_start = static_cast<std::byte*>(std::malloc(MAX_HEAP_SIZE));
  heap_brk = static_cast<std::byte*>(
      heap_start); /* No allocations yet so start = end. */
  heap_max_addr = static_cast<std::byte*>(heap_start + MAX_HEAP_SIZE);
}

/*
 * Increase the size of the heap and returns a pointer to the old head of the
 * heap.
 *
 * @param increment Make the heap bigger by increment bytes.
 * @return Old heap of the heap (the new heap of the heap is at old_head +
 * increment).
 */
std::byte* mem_sbrk(std::size_t increment) {
  std::byte* old_brk = heap_brk;

  if (increment < 0 || (heap_brk + increment) > heap_max_addr) {
    errno = ENOMEM;
    fmt::print(stderr, "ERROR: mem_sbrk failed. Ran out of memory...\n");
    return nullptr;
  }
  heap_brk += increment;
  return old_brk;
}

void mem_teardown() { std::free(heap_start); }
