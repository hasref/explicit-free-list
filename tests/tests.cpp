#define CATCH_CONFIG_MAIN
#include <fmt/format.h>

#include <catch2/catch.hpp>

#include "mm.h"

TEST_CASE("Can Allocate Memory", "[allocate]") {
  mm_init();
  std::byte* ptr = mm_malloc(8);
  mm_free(ptr);
}
