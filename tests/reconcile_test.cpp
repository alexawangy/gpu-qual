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
    CHECK(reasons[0].field == "gpus[0].health.ecc_mode_enabled");
    CHECK(reasons[0].expected == true);
    CHECK(reasons[0].observed == false);
  }

  SECTION("health checks are skipped when include_health is false") {
    ExpectedSpec spec;
    spec.expected.health.ecc_mode_enabled = true;
    spec.options.include_health = false;

    ObservedState observed{};
    observed.gpus.push_back(GpuInfo{.health = GpuHealth{.ecc_mode_enabled = false}});

    CHECK(reconcile(spec, observed).empty());
  }

  SECTION("engaged health with no observed block is unsupported") {
    ExpectedSpec spec;
    spec.expected.health.ecc_mode_enabled = true;

    ObservedState observed{};
    observed.gpus.push_back(GpuInfo{});

    const auto reasons = reconcile(spec, observed);

    REQUIRE(reasons.size() == 1);
    CHECK(reasons[0].code == ReasonCode::FIELD_UNSUPPORTED);
    CHECK(reasons[0].field == "gpus[0].health");
  }

  SECTION("CUDA smoke fails only after it ran") {
    ExpectedSpec spec;
    spec.options.cuda_smoke = true;

    ObservedState not_run{};
    CHECK(reconcile(spec, not_run).empty());

    ObservedState failed{};
    failed.cuda.smoke_ran = true;
    failed.cuda.smoke_passed = false;

    const auto reasons = reconcile(spec, failed);

    REQUIRE(reasons.size() == 1);
    CHECK(reasons[0].code == ReasonCode::CUDA_SMOKE_FAILED);
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

TEST_CASE("policy severity remaps the check-mode exit band") {
  SECTION("warn policy downgrades a hard identity failure") {
    ExpectedSpec spec;
    spec.expected.gpu_count = 2;
    spec.policy.gpu_count = Severity::WARN;

    ObservedState observed{};
    observed.gpus.push_back(GpuInfo{});

    const auto result = compute_result(Mode::CHECK, reconcile(spec, observed));

    CHECK(result.verdict == Verdict::WARN);
    CHECK(result.exit_code == ExitCode::WARN);
  }

  SECTION("hard policy escalates a warn-default failure") {
    ExpectedSpec spec;
    spec.expected.min_driver_version = "550.90.07";
    spec.policy.driver_version = Severity::HARD;

    ObservedState observed{};
    observed.nvml.driver_version = "535.104.05";

    const auto result = compute_result(Mode::CHECK, reconcile(spec, observed));

    CHECK(result.exit_code == ExitCode::FAIL_CONTRACT);
  }
}

TEST_CASE("default policy routes reasons to their natural exit band") {
  SECTION("CUDA smoke failure is a usability failure") {
    ExpectedSpec spec;
    spec.options.cuda_smoke = true;

    ObservedState observed{};
    observed.cuda.smoke_ran = true;
    observed.cuda.smoke_passed = false;

    const auto result = compute_result(Mode::CHECK, reconcile(spec, observed));

    CHECK(result.exit_code == ExitCode::FAIL_USABILITY);
  }

  SECTION("row-remap failure is a stack failure") {
    ExpectedSpec spec;
    spec.expected.health.allow_row_remap_failure = false;

    ObservedState observed{};
    observed.gpus.push_back(GpuInfo{.health = GpuHealth{.row_remap_failure = true}});

    const auto result = compute_result(Mode::CHECK, reconcile(spec, observed));

    CHECK(result.exit_code == ExitCode::FAIL_STACK);
  }

  SECTION("an unsupported field warns rather than passing") {
    ExpectedSpec spec;
    spec.expected.health.ecc_mode_enabled = true;

    ObservedState observed{};
    observed.gpus.push_back(GpuInfo{});

    const auto result = compute_result(Mode::CHECK, reconcile(spec, observed));

    CHECK(result.exit_code == ExitCode::WARN);
  }
}

TEST_CASE("reconcile returns reasons in identity, health, fabric, cuda-smoke order") {
  ExpectedSpec spec;
  spec.expected.gpu_count = 2;
  spec.expected.health.ecc_mode_enabled = true;
  spec.expected.fabric.require_fabric_ready = true;
  spec.options.cuda_smoke = true;

  ObservedState observed{};
  observed.gpus.push_back(GpuInfo{.health = GpuHealth{.ecc_mode_enabled = false}});
  observed.fabric = FabricState{.applicable = true, .ready = false};
  observed.cuda.smoke_ran = true;
  observed.cuda.smoke_passed = false;

  const auto reasons = reconcile(spec, observed);

  REQUIRE(reasons.size() == 4);
  CHECK(reasons[0].code == ReasonCode::GPU_COUNT_MISMATCH);
  CHECK(reasons[1].code == ReasonCode::ECC_MODE_MISMATCH);
  CHECK(reasons[2].code == ReasonCode::FABRIC_NOT_READY);
  CHECK(reasons[3].code == ReasonCode::CUDA_SMOKE_FAILED);
}
