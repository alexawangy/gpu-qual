#include <catch2/catch_test_macros.hpp>
#include <cstdint>

uint32_t factorial( uint32_t num ) {
  return num <= 1 ? num : factorial(num-1);
}

TEST_CASE("computed factorials", "[factorial]") {
  REQUIRE(factorial(1) == 1);
}
