#include <catch2/catch_test_macros.hpp>
#include <compare>
#include <gpu_qual/reconcile.hpp>

using namespace gpu_qual;

TEST_CASE("driver version comparisons are correct") {
  CHECK(compare_driver_versions("101.202.1", "101.202.1") == std::strong_ordering::equal);
  CHECK(compare_driver_versions("101.202.1", "101.202.2") == std::strong_ordering::less);
  CHECK(compare_driver_versions("101.222.1", "101.202.1") == std::strong_ordering::greater);
}
