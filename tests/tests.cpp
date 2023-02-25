#define CATCH_CONFIG_MAIN
#include <fmt/format.h>

#include <catch2/catch.hpp>

#include "mm.h"

TEST_CASE("Can Allocate Memory", "[allocate]") {
  mm_init();

  std::byte* ptr = mm_malloc(8);
  std::size_t is_8aligned = reinterpret_cast<std::size_t>(ptr) % 8;
  REQUIRE(is_8aligned == 0);

  mm_free(ptr);
  mm_teardown();
}

TEST_CASE("Writing to and from memory works", "[read_write]") {
  mm_init();

  std::byte* ptr = mm_malloc(12);
  int put_val = 20;
  *reinterpret_cast<int*>(ptr) = put_val;
  int read_val = *reinterpret_cast<int*>(ptr);
  REQUIRE(read_val == put_val);

  mm_free(ptr);
  mm_teardown();
}

TEST_CASE("Realloc works", "[realloc]") {
  mm_init();

  std::byte* ptr = mm_malloc(20);
  int put_val = 20;
  *reinterpret_cast<int*>(ptr) = put_val;
  mm_realloc(ptr, 30);
  int read_val = *reinterpret_cast<int*>(ptr);

  REQUIRE(read_val == put_val);

  mm_free(ptr);
  mm_teardown();
}
