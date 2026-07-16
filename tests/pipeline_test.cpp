#include <gpu_qual/pipeline.hpp>
#include <gpu_qual/spec.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <utility>

namespace gpu_qual {
namespace {

GpuInfo valid_gpu(std::string name = "NVIDIA H100 80GB HBM3") {
  return {
      .index = 0,
      .name = std::move(name),
      .uuid = "GPU-test",
      .pci_bdf = "0000:1b:00.0",
      .memory_mib = 81559,
  };
}

TEST_CASE("healthy inventory evaluation succeeds", "[pipeline]") {
  ObservedState observed{};
  observed.nvml = {.status = NvmlStatus::READY, .driver_version = "550.90.07"};
  observed.gpus.push_back(valid_gpu());

  const Result result = evaluate_inventory(ProbeOutcome{.observed = std::move(observed)});

  CHECK(result.mode == Mode::INVENTORY);
  CHECK(result.verdict == Verdict::OBSERVED);
  CHECK(result.exit_code == ExitCode::OK);
  CHECK(result.reasons.empty());
  REQUIRE(result.observed.has_value());
  REQUIRE(result.observed->gpus.size() == 1);
  CHECK(result.observed->gpus[0].uuid == "GPU-test");
}

TEST_CASE("inventory evaluation attaches observations and routes probe reasons", "[pipeline]") {
  ObservedState observed{};
  observed.nvml.status = NvmlStatus::LIBRARY_NOT_FOUND;

  ProbeOutcome outcome{
      .observed = std::move(observed),
      .reasons = {},
  };

  const Result result = evaluate_inventory(std::move(outcome));

  CHECK(result.mode == Mode::INVENTORY);
  CHECK(result.verdict == Verdict::FAIL);
  CHECK(result.exit_code == ExitCode::FAIL_STACK);
  REQUIRE(result.observed.has_value());
  CHECK(result.observed->gpus.empty());
  REQUIRE(result.reasons.size() == 1);
  CHECK(result.reasons[0].code == ReasonCode::NVML_LIBRARY_NOT_FOUND);
}

TEST_CASE("check evaluation applies the validated expected spec", "[pipeline]") {
  SECTION("matching identity") {
    ExpectedSpec spec{};
    spec.gpu_count = 1;
    spec.allowed_names = {"NVIDIA H100 80GB HBM3"};

    ObservedState observed{};
    observed.nvml = {.status = NvmlStatus::READY, .driver_version = "550.90.07"};
    observed.gpus.push_back(valid_gpu());
    ProbeOutcome outcome{.observed = std::move(observed), .reasons = {}};

    const Result result = evaluate_check(std::move(outcome), spec);

    CHECK(result.mode == Mode::CHECK);
    CHECK(result.verdict == Verdict::PASS);
    CHECK(result.exit_code == ExitCode::OK);
    REQUIRE(result.observed.has_value());
    CHECK(result.observed->gpus.size() == 1);
    CHECK(result.reasons.empty());
  }

  SECTION("identity mismatch") {
    ExpectedSpec spec{};
    spec.gpu_count = 2;

    ObservedState observed{};
    observed.nvml = {.status = NvmlStatus::READY, .driver_version = "550.90.07"};
    observed.gpus.push_back(valid_gpu());
    ProbeOutcome outcome{.observed = std::move(observed), .reasons = {}};

    const Result result = evaluate_check(std::move(outcome), spec);

    CHECK(result.verdict == Verdict::FAIL);
    CHECK(result.exit_code == ExitCode::FAIL_CONTRACT);
    REQUIRE(result.reasons.size() == 1);
    CHECK(result.reasons[0].code == ReasonCode::GPU_COUNT_MISMATCH);
  }
}

TEST_CASE("check evaluation does not reconcile an unusable observation", "[pipeline]") {
  ExpectedSpec spec{};
  spec.gpu_count = 2;

  ObservedState observed{};
  observed.nvml.status = NvmlStatus::NO_PERMISSION;
  ProbeOutcome outcome{
      .observed = std::move(observed),
      .reasons = {},
  };
  const Result result = evaluate_check(std::move(outcome), spec);

  CHECK(result.exit_code == ExitCode::FAIL_STACK);
  REQUIRE(result.reasons.size() == 1);
  CHECK(result.reasons[0].code == ReasonCode::NVML_NO_PERMISSION);
}

TEST_CASE("evaluation rejects internally inconsistent probe outcomes", "[pipeline]") {
  SECTION("the default not-probed outcome cannot pass") {
    const Result result = evaluate_inventory(ProbeOutcome{});

    CHECK(result.verdict == Verdict::FAIL);
    CHECK(result.exit_code == ExitCode::FAIL_STACK);
    REQUIRE(result.reasons.size() == 1);
    CHECK(result.reasons[0].code == ReasonCode::PROBE_OUTPUT_INVALID);
  }

  SECTION("ready requires a valid driver version") {
    ObservedState observed{};
    observed.nvml.status = NvmlStatus::READY;
    observed.gpus.push_back(GpuInfo{});

    const Result result = evaluate_inventory(ProbeOutcome{.observed = std::move(observed)});

    CHECK(result.exit_code == ExitCode::FAIL_STACK);
    REQUIRE(result.reasons.size() == 1);
    CHECK(result.reasons[0].code == ReasonCode::PROBE_OUTPUT_INVALID);
  }

  SECTION("ready with no physical GPUs is an explicit stack failure") {
    ObservedState observed{};
    observed.nvml = {.status = NvmlStatus::READY, .driver_version = "550.90.07"};

    const Result result = evaluate_inventory(ProbeOutcome{.observed = std::move(observed)});

    CHECK(result.exit_code == ExitCode::FAIL_STACK);
    REQUIRE(result.reasons.size() == 1);
    CHECK(result.reasons[0].code == ReasonCode::NO_NVIDIA_DEVICES);
  }

  SECTION("ready requires complete physical GPU identity") {
    ObservedState observed{};
    observed.nvml = {.status = NvmlStatus::READY, .driver_version = "550.90.07"};
    observed.gpus.push_back(GpuInfo{.name = "NVIDIA H100 80GB HBM3"});

    const Result result = evaluate_inventory(ProbeOutcome{.observed = std::move(observed)});

    CHECK(result.exit_code == ExitCode::FAIL_STACK);
    REQUIRE(result.reasons.size() == 1);
    CHECK(result.reasons[0].code == ReasonCode::PROBE_OUTPUT_INVALID);
    CHECK(result.reasons[0].field == "gpus[0].uuid");
  }
}

} // namespace
} // namespace gpu_qual
