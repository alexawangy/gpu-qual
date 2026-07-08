#include <gpu_qual/pipeline.hpp>
#include <gpu_qual/spec.hpp>

#include <catch2/catch_test_macros.hpp>

#include <utility>

namespace gpu_qual {
namespace {

TEST_CASE("inventory evaluation attaches observations and routes probe reasons", "[pipeline]") {
  ObservedState observed{};
  observed.nvml.available = false;
  observed.nvml.init_ok = false;
  observed.fallback.nvidia_pci_devices_present = true;

  ProbeOutcome outcome{
      .observed = std::move(observed),
      .reasons = {make_reason(ReasonCode::NVML_LIBRARY_NOT_FOUND)},
  };

  const Result result = evaluate_inventory(std::move(outcome));

  CHECK(result.mode == Mode::INVENTORY);
  CHECK(result.verdict == Verdict::FAIL);
  CHECK(result.exit_code == ExitCode::FAIL_STACK);
  REQUIRE(result.observed.has_value());
  CHECK(result.observed->fallback.nvidia_pci_devices_present);
  REQUIRE(result.reasons.size() == 1);
  CHECK(result.reasons[0].code == ReasonCode::NVML_LIBRARY_NOT_FOUND);
}

TEST_CASE("check evaluation applies the validated expected spec", "[pipeline]") {
  SECTION("matching identity") {
    ExpectedSpec spec{};
    spec.expected.gpu_count = 1;
    spec.expected.allowed_names = {"NVIDIA H100 80GB HBM3"};

    ObservedState observed{};
    observed.gpus.push_back(GpuInfo{.name = "NVIDIA H100 80GB HBM3"});
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
    spec.expected.gpu_count = 2;

    ObservedState observed{};
    observed.gpus.push_back(GpuInfo{});
    ProbeOutcome outcome{.observed = std::move(observed), .reasons = {}};

    const Result result = evaluate_check(std::move(outcome), spec);

    CHECK(result.verdict == Verdict::FAIL);
    CHECK(result.exit_code == ExitCode::FAIL_CONTRACT);
    REQUIRE(result.reasons.size() == 1);
    CHECK(result.reasons[0].code == ReasonCode::GPU_COUNT_MISMATCH);
  }

  SECTION("supported health mismatch") {
    ExpectedSpec spec{};
    spec.expected.health.ecc_mode_enabled = true;

    ObservedState observed{};
    observed.gpus.push_back(GpuInfo{.index = 0, .health = GpuHealth{.ecc_mode_enabled = false}});
    ProbeOutcome outcome{.observed = std::move(observed), .reasons = {}};

    const Result result = evaluate_check(std::move(outcome), spec);

    CHECK(result.verdict == Verdict::FAIL);
    CHECK(result.exit_code == ExitCode::FAIL_CONTRACT);
    REQUIRE(result.reasons.size() == 1);
    CHECK(result.reasons[0].code == ReasonCode::ECC_MODE_MISMATCH);
  }
}

TEST_CASE("check evaluation orders probe, spec, then reconciliation reasons", "[pipeline]") {
  ExpectedSpec spec{};
  spec.expected.gpu_count = 2;

  ObservedState observed{};
  observed.gpus.push_back(GpuInfo{});
  ProbeOutcome outcome{
      .observed = std::move(observed),
      .reasons = {make_reason(ReasonCode::NVML_NO_PERMISSION)},
  };
  std::vector<Reason> spec_reasons{
      make_reason(ReasonCode::UNKNOWN_FIELD_IGNORED, "surprise"),
  };

  const Result result = evaluate_check(std::move(outcome), spec, std::move(spec_reasons));

  CHECK(result.exit_code == ExitCode::FAIL_STACK);
  REQUIRE(result.reasons.size() == 3);
  CHECK(result.reasons[0].code == ReasonCode::NVML_NO_PERMISSION);
  CHECK(result.reasons[1].code == ReasonCode::UNKNOWN_FIELD_IGNORED);
  CHECK(result.reasons[2].code == ReasonCode::GPU_COUNT_MISMATCH);
}

} // namespace
} // namespace gpu_qual
