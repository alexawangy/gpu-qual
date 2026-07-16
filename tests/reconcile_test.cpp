#include <catch2/catch_test_macros.hpp>
#include <gpu_qual/reconcile.hpp>

using namespace gpu_qual;

TEST_CASE("reconcile skips expectations that are not set") {
  const ExpectedSpec spec{};
  const ObservedState observed{
      .nvml = {.status = NvmlStatus::READY, .driver_version = "550.90.07"},
      .gpus = {{.name = "NVIDIA H100 80GB HBM3", .memory_mib = 81559}},
  };

  CHECK(reconcile(spec, observed).empty());
}

TEST_CASE("reconcile reports each identity mismatch") {
  SECTION("GPU count") {
    ExpectedSpec spec;
    spec.gpu_count = 2;

    ObservedState observed;
    observed.nvml.status = NvmlStatus::READY;
    observed.gpus.push_back(GpuInfo{});

    const auto reasons = reconcile(spec, observed);
    REQUIRE(reasons.size() == 1);
    CHECK(reasons[0].code == ReasonCode::GPU_COUNT_MISMATCH);
    REQUIRE(reasons[0].expected.has_value());
    REQUIRE(reasons[0].observed.has_value());
    CHECK(*reasons[0].expected == 2);
    CHECK(*reasons[0].observed == 1);
  }

  SECTION("GPU name") {
    ExpectedSpec spec;
    spec.allowed_names = {"NVIDIA H100 80GB HBM3"};

    ObservedState observed;
    observed.nvml.status = NvmlStatus::READY;
    observed.gpus.push_back(GpuInfo{.name = "NVIDIA A100-SXM4-80GB"});

    const auto reasons = reconcile(spec, observed);
    REQUIRE(reasons.size() == 1);
    CHECK(reasons[0].code == ReasonCode::GPU_NAME_MISMATCH);
    REQUIRE(reasons[0].expected.has_value());
    REQUIRE(reasons[0].observed.has_value());
    CHECK(*reasons[0].expected == nlohmann::json::array({"NVIDIA H100 80GB HBM3"}));
    CHECK(*reasons[0].observed == "NVIDIA A100-SXM4-80GB");
  }

  SECTION("GPU memory") {
    ExpectedSpec spec;
    spec.min_memory_mib = 80000;

    ObservedState observed;
    observed.nvml.status = NvmlStatus::READY;
    observed.gpus.push_back(GpuInfo{.memory_mib = 40960});

    const auto reasons = reconcile(spec, observed);
    REQUIRE(reasons.size() == 1);
    CHECK(reasons[0].code == ReasonCode::GPU_MEMORY_BELOW_MIN);
    REQUIRE(reasons[0].expected.has_value());
    REQUIRE(reasons[0].observed.has_value());
    CHECK(*reasons[0].expected == 80000);
    CHECK(*reasons[0].observed == 40960);
  }

  SECTION("driver version") {
    ExpectedSpec spec;
    spec.min_driver_version = "550.90.07";

    ObservedState observed;
    observed.nvml = {.status = NvmlStatus::READY, .driver_version = "535.104.05"};

    const auto reasons = reconcile(spec, observed);
    REQUIRE(reasons.size() == 1);
    CHECK(reasons[0].code == ReasonCode::DRIVER_VERSION_BELOW_MIN);
    REQUIRE(reasons[0].expected.has_value());
    REQUIRE(reasons[0].observed.has_value());
    CHECK(*reasons[0].expected == "550.90.07");
    CHECK(*reasons[0].observed == "535.104.05");
  }
}

TEST_CASE("reconcile treats unavailable requested driver data as invalid probe output") {
  ExpectedSpec spec;
  spec.min_driver_version = "550.90.07";

  ObservedState observed{};
  observed.nvml.status = NvmlStatus::READY;

  const auto reasons = reconcile(spec, observed);
  REQUIRE(reasons.size() == 1);
  CHECK(reasons[0].code == ReasonCode::PROBE_OUTPUT_INVALID);
  REQUIRE(reasons[0].observed.has_value());
  CHECK(reasons[0].observed->is_null());
}

TEST_CASE("reconcile is non-throwing for malformed programmatic versions") {
  SECTION("invalid expected version") {
    ExpectedSpec spec;
    spec.min_driver_version = "550..90";

    const ObservedState observed{
        .nvml = {.status = NvmlStatus::READY, .driver_version = "550.90.07"},
    };
    const auto reasons = reconcile(spec, observed);
    REQUIRE(reasons.size() == 1);
    CHECK(reasons[0].code == ReasonCode::EXPECTED_SPEC_INVALID);
  }

  SECTION("invalid observed version") {
    ExpectedSpec spec;
    spec.min_driver_version = "550.90.07";

    const ObservedState observed{
        .nvml = {.status = NvmlStatus::READY, .driver_version = "not-a-version"},
    };
    const auto reasons = reconcile(spec, observed);
    REQUIRE(reasons.size() == 1);
    CHECK(reasons[0].code == ReasonCode::PROBE_OUTPUT_INVALID);
  }
}

TEST_CASE("reconcile does not compare identity for a failed observation") {
  ExpectedSpec spec;
  spec.gpu_count = 8;

  const ObservedState observed{
      .nvml = {.status = NvmlStatus::NO_PERMISSION},
  };
  CHECK(reconcile(spec, observed).empty());
}

TEST_CASE("reconcile returns deterministic identity then driver ordering") {
  ExpectedSpec spec;
  spec.gpu_count = 2;
  spec.allowed_names = {"NVIDIA H100 80GB HBM3"};
  spec.min_memory_mib = 80000;
  spec.min_driver_version = "550.90.07";

  const ObservedState observed{
      .nvml = {.status = NvmlStatus::READY, .driver_version = "535.104.05"},
      .gpus = {{.name = "NVIDIA A100-SXM4-40GB", .memory_mib = 40960}},
  };

  const auto reasons = reconcile(spec, observed);
  REQUIRE(reasons.size() == 4);
  CHECK(reasons[0].code == ReasonCode::GPU_COUNT_MISMATCH);
  CHECK(reasons[1].code == ReasonCode::GPU_NAME_MISMATCH);
  CHECK(reasons[2].code == ReasonCode::GPU_MEMORY_BELOW_MIN);
  CHECK(reasons[3].code == ReasonCode::DRIVER_VERSION_BELOW_MIN);
}
