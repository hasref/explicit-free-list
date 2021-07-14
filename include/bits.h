#ifndef BITS_H_
#define BITS_H_

#include <cstdlib>

constexpr int WORD_SIZE = 4;
constexpr int DOUBLE_SIZE = 8; /* double word size*/
constexpr int CHUNKSIZE = (1 << 12);

template <typename T>
bool max(T x, T y) {
  return x > y ? x : y;
}

#endif
