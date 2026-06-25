#include <catch2/catch_test_macros.hpp>
#include <gpu_qual/reconcile.hpp>
#include <stdexcept>
#include <vector>

using namespace gpu_qual;

TEST_CASE("string version to number version") {
  std::vector<unsigned> res = {10, 34, 101};
  CHECK(parse_version("10.34.101") == res);
}

TEST_CASE("fail on non-parsable version") {
  REQUIRE_THROWS_AS(parse_version("something.10.11"), std::invalid_argument);
}
