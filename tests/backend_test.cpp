#include <gpu_qual/backend.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>

namespace gpu_qual {
namespace {

TEST_CASE("MockBackend returns its injected probe outcome") {
  ObservedState observed{};
  observed.nvml.available = true;
  observed.nvml.init_ok = true;
  observed.nvml.driver_version = "test-driver";

  ProbeOutcome expected{
      .observed = std::move(observed),
      .reasons = {},
  };

  const MockBackend backend{expected};
  const ProbeOutcome actual = backend.probe();

  STATIC_REQUIRE(Backend<MockBackend>);
  CHECK(std::string{backend.name()} == "mock");
  REQUIRE(actual.observed.nvml.driver_version.has_value());
  CHECK(*actual.observed.nvml.driver_version == "test-driver");
  CHECK(actual.reasons.empty());
}

} // namespace
} // namespace gpu_qual
