#ifndef MEM_LIB_H_
#define MEM_LIB_H_

#include <cstddef>

void mem_init();

/*
 * Increment the amount of virtual memory allocated by
 * the allocator. Note that this allocator works by
 * malloc-ing a fixed size of memory at once and then using that
 * for responding to malloc requests.
 *
 * @param increment Number of bytes to grow the heap by.
 *
 * @return pointer to block of memory at least 'increment'
 * bytes. On out of memory, the allocator returns a nullptr.
 */
std::byte* mem_sbrk(std::size_t increment);

/*
 * Free allocated heap.
 */
void mem_teardown();
#endif
