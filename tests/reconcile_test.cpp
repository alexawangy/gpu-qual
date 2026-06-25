#include <catch2/catch_test_macros.hpp>
#include <compare>
#include <gpu_qual/reconcile.hpp>

using namespace gpu_qual;

TEST_CASE("driver version comparisons are correct") {
  CHECK(compare_driver_versions("101.202.1", "101.202.1") == std::strong_ordering::equal);
  CHECK(compare_driver_versions("101.202.1", "101.202.2") == std::strong_ordering::less);
  CHECK(compare_driver_versions("101.222.1", "101.202.1") == std::strong_ordering::greater);
}

TEST_CASE("reconcile skips GPU name checks when allowed names are unset") {
  ExpectedSpec spec;

  ObservedState observed;
  observed.gpus.push_back(GpuInfo{.name = "NVIDIA H100 80GB HBM3"});

  const auto reasons = reconcile(spec, observed);

  CHECK(reasons.empty());
}

TEST_CASE("reconcile reports expected mismatch reasons") {
  SECTION("GPU count mismatch") {
    ExpectedSpec spec;
    spec.expected.gpu_count = 2;

    ObservedState observed;
    observed.gpus.push_back(GpuInfo{});

    const auto reasons = reconcile(spec, observed);

    REQUIRE(reasons.size() == 1);
    CHECK(reasons[0].code == ReasonCode::GPU_COUNT_MISMATCH);
    CHECK(reasons[0].expected == 2);
    CHECK(reasons[0].observed == 1);
  }

  SECTION("driver version below minimum") {
    ExpectedSpec spec;
    spec.expected.min_driver_version = "550.90.07";

    ObservedState observed{};
    observed.nvml.driver_version = "535.104.05";

    const auto reasons = reconcile(spec, observed);

    REQUIRE(reasons.size() == 1);
    CHECK(reasons[0].code == ReasonCode::DRIVER_VERSION_BELOW_MIN);
    CHECK(reasons[0].expected == "550.90.07");
    CHECK(reasons[0].observed == "535.104.05");
  }

  SECTION("too few CUDA-visible devices") {
    ExpectedSpec spec;
    spec.expected.min_cuda_visible_devices = 4;

    ObservedState observed{};
    observed.cuda.device_count = 2;

    const auto reasons = reconcile(spec, observed);

    REQUIRE(reasons.size() == 1);
    CHECK(reasons[0].code == ReasonCode::CUDA_VISIBLE_COUNT_BELOW_MIN);
    CHECK(reasons[0].expected == 4);
    CHECK(reasons[0].observed == 2);
  }

  SECTION("ECC mode mismatch") {
    ExpectedSpec spec;
    spec.expected.health.ecc_mode_enabled = true;

    ObservedState observed{};
    observed.gpus.push_back(GpuInfo{.health = GpuHealth{.ecc_mode_enabled = false}});

    const auto reasons = reconcile(spec, observed);

    REQUIRE(reasons.size() == 1);
    CHECK(reasons[0].code == ReasonCode::ECC_MODE_MISMATCH);
    CHECK(reasons[0].expected == true);
    CHECK(reasons[0].observed == false);
  }

  SECTION("fabric not ready") {
    ExpectedSpec spec;
    spec.expected.fabric.require_fabric_ready = true;

    ObservedState observed{};
    observed.fabric = FabricState{.applicable = true, .ready = false};

    const auto reasons = reconcile(spec, observed);

    REQUIRE(reasons.size() == 1);
    CHECK(reasons[0].code == ReasonCode::FABRIC_NOT_READY);
    CHECK(reasons[0].expected == true);
    CHECK(reasons[0].observed == false);
  }
}
