#include <catch2/catch_test_macros.hpp>
#include <gpu_qual/version.hpp>

#include <compare>

using namespace gpu_qual;

TEST_CASE("driver version validation matches the contract grammar") {
  CHECK(is_valid_driver_version("0"));
  CHECK(is_valid_driver_version("550.090.007"));
  CHECK(is_valid_driver_version("429496729600000000000000.1"));

  for (const char* version :
       {"", ".", ".10", "10.", "10..11", "something.10.11", "10. 11", "-1.2", "+550.90"}) {
    INFO(version);
    CHECK_FALSE(is_valid_driver_version(version));
  }
}

TEST_CASE("driver version comparison is numeric and non-throwing") {
  CHECK(compare_driver_versions("101.202.1", "101.202.1") == std::strong_ordering::equal);
  CHECK(compare_driver_versions("101.202.1", "101.202.2") == std::strong_ordering::less);
  CHECK(compare_driver_versions("101.222.1", "101.202.1") == std::strong_ordering::greater);
  CHECK(compare_driver_versions("550.90", "550.90.0") == std::strong_ordering::equal);
  CHECK(compare_driver_versions("000550.000090.7", "550.90.007") == std::strong_ordering::equal);
  CHECK(compare_driver_versions("429496729600000000000001.1", "429496729600000000000000.999") ==
        std::strong_ordering::greater);

  CHECK_FALSE(compare_driver_versions("invalid", "550.90").has_value());
  CHECK_FALSE(compare_driver_versions("550.90", "invalid").has_value());
}
