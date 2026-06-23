#include <catch2/catch_test_macros.hpp>
#include <gpu_qual/verdict.hpp>

#include <optional>
#include <string>

using namespace gpu_qual;

namespace {
Reason r(ReasonCode code) {
  return {.code = code, .cls = default_class(code)};
}
} // namespace

TEST_CASE("enum to_string returns stable lowercase names") {
  CHECK(std::string{to_string(Mode::INVENTORY)} == "inventory");
  CHECK(std::string{to_string(Mode::CHECK)} == "check");

  CHECK(std::string{to_string(Verdict::OBSERVED)} == "observed");
  CHECK(std::string{to_string(Verdict::PASS)} == "pass");
  CHECK(std::string{to_string(Verdict::WARN)} == "warn");
  CHECK(std::string{to_string(Verdict::RETRY)} == "retry");
  CHECK(std::string{to_string(Verdict::FAIL)} == "fail");

  CHECK(std::string{to_string(ReasonClass::REPORT)} == "report");
  CHECK(std::string{to_string(ReasonClass::WARN)} == "warn");
  CHECK(std::string{to_string(ReasonClass::HARD)} == "hard");
  CHECK(std::string{to_string(ReasonClass::RETRY)} == "retry");
}

TEST_CASE("ExitCode integer values match the routing bands") {
  CHECK(static_cast<int>(ExitCode::OK) == 0);
  CHECK(static_cast<int>(ExitCode::WARN) == 10);
  CHECK(static_cast<int>(ExitCode::RETRY) == 20);
  CHECK(static_cast<int>(ExitCode::FAIL_CONTRACT) == 30);
  CHECK(static_cast<int>(ExitCode::FAIL_USABILITY) == 40);
  CHECK(static_cast<int>(ExitCode::FAIL_STACK) == 50);
}

// Single source of truth for every reason code: name + default class, plus the
// default exit band where the routing is fixed. `exit == nullopt` marks codes
// whose standalone band isn't pinned here (it falls out of compute_result).
TEST_CASE("every ReasonCode has a stable name, class, and exit band") {
  struct Row {
    ReasonCode code;
    const char* name;
    ReasonClass cls;
    std::optional<ExitCode> exit;
  };

  const Row rows[] = {
      {ReasonCode::GPU_COUNT_MISMATCH, "GPU_COUNT_MISMATCH", ReasonClass::HARD,
       ExitCode::FAIL_CONTRACT},
      {ReasonCode::GPU_NAME_MISMATCH, "GPU_NAME_MISMATCH", ReasonClass::HARD,
       ExitCode::FAIL_CONTRACT},
      {ReasonCode::GPU_MEMORY_BELOW_MIN, "GPU_MEMORY_BELOW_MIN", ReasonClass::HARD,
       ExitCode::FAIL_CONTRACT},
      {ReasonCode::MIG_MODE_MISMATCH, "MIG_MODE_MISMATCH", ReasonClass::HARD,
       ExitCode::FAIL_CONTRACT},
      {ReasonCode::CUDA_VISIBLE_COUNT_BELOW_MIN, "CUDA_VISIBLE_COUNT_BELOW_MIN",
       ReasonClass::REPORT, std::nullopt},
      {ReasonCode::NVML_LIBRARY_NOT_FOUND, "NVML_LIBRARY_NOT_FOUND", ReasonClass::HARD,
       std::nullopt},
      {ReasonCode::NVML_INIT_FAILED, "NVML_INIT_FAILED", ReasonClass::HARD, std::nullopt},
      {ReasonCode::NVML_NO_PERMISSION, "NVML_NO_PERMISSION", ReasonClass::HARD, std::nullopt},
      {ReasonCode::NO_NVIDIA_DEVICES, "NO_NVIDIA_DEVICES", ReasonClass::HARD, std::nullopt},
      {ReasonCode::CUDA_LIBRARY_NOT_FOUND, "CUDA_LIBRARY_NOT_FOUND", ReasonClass::HARD,
       ExitCode::FAIL_USABILITY},
      {ReasonCode::CUDA_INIT_FAILED, "CUDA_INIT_FAILED", ReasonClass::HARD,
       ExitCode::FAIL_USABILITY},
      {ReasonCode::CUDA_CONTEXT_FAILED, "CUDA_CONTEXT_FAILED", ReasonClass::HARD,
       ExitCode::FAIL_USABILITY},
      {ReasonCode::CUDA_SMOKE_FAILED, "CUDA_SMOKE_FAILED", ReasonClass::HARD,
       ExitCode::FAIL_USABILITY},
      {ReasonCode::EXPECTED_SPEC_INVALID, "EXPECTED_SPEC_INVALID", ReasonClass::HARD, std::nullopt},
      {ReasonCode::PROBE_TIMEOUT, "PROBE_TIMEOUT", ReasonClass::RETRY, ExitCode::RETRY},
      {ReasonCode::PROBE_CHILD_CRASHED, "PROBE_CHILD_CRASHED", ReasonClass::HARD, std::nullopt},
      {ReasonCode::PROBE_OUTPUT_INVALID, "PROBE_OUTPUT_INVALID", ReasonClass::HARD, std::nullopt},
      {ReasonCode::FIELD_UNSUPPORTED, "FIELD_UNSUPPORTED", ReasonClass::REPORT, ExitCode::WARN},
      {ReasonCode::UNKNOWN_FIELD_IGNORED, "UNKNOWN_FIELD_IGNORED", ReasonClass::REPORT,
       std::nullopt},
      {ReasonCode::DRIVER_VERSION_BELOW_MIN, "DRIVER_VERSION_BELOW_MIN", ReasonClass::WARN,
       ExitCode::WARN},
      {ReasonCode::ECC_MODE_MISMATCH, "ECC_MODE_MISMATCH", ReasonClass::HARD,
       ExitCode::FAIL_CONTRACT},
      {ReasonCode::ECC_UNCORRECTABLE_DETECTED, "ECC_UNCORRECTABLE_DETECTED", ReasonClass::HARD,
       ExitCode::FAIL_CONTRACT},
      {ReasonCode::ROW_REMAP_PENDING, "ROW_REMAP_PENDING", ReasonClass::HARD,
       ExitCode::FAIL_CONTRACT},
      {ReasonCode::ROW_REMAP_FAILURE, "ROW_REMAP_FAILURE", ReasonClass::HARD, ExitCode::FAIL_STACK},
      {ReasonCode::RETIRED_PAGES_PENDING, "RETIRED_PAGES_PENDING", ReasonClass::WARN,
       ExitCode::WARN},
      {ReasonCode::GPU_RECOVERY_ACTION_REQUIRED, "GPU_RECOVERY_ACTION_REQUIRED", ReasonClass::HARD,
       ExitCode::FAIL_STACK},
      {ReasonCode::FABRIC_NOT_READY, "FABRIC_NOT_READY", ReasonClass::HARD,
       ExitCode::FAIL_USABILITY},
      {ReasonCode::FABRIC_NOT_APPLICABLE, "FABRIC_NOT_APPLICABLE", ReasonClass::REPORT,
       ExitCode::OK},
  };

  for (const auto& row : rows) {
    INFO(row.name);
    CHECK(std::string{to_string(row.code)} == row.name);
    CHECK(default_class(row.code) == row.cls);
    if (row.exit) {
      CHECK(default_exit_code(row.code) == *row.exit);
    }
  }
}

TEST_CASE("compute_result maps reasons to a verdict and exit band") {
  SECTION("no reasons yields the per-mode success verdict") {
    const auto inv = compute_result(Mode::INVENTORY, {});
    CHECK(inv.verdict == Verdict::OBSERVED);
    CHECK(inv.exit_code == ExitCode::OK);

    const auto chk = compute_result(Mode::CHECK, {});
    CHECK(chk.verdict == Verdict::PASS);
    CHECK(chk.exit_code == ExitCode::OK);
  }

  SECTION("the highest-impact reason selects the exit band") {
    const auto retry = compute_result(Mode::INVENTORY, {r(ReasonCode::PROBE_TIMEOUT)});
    CHECK(retry.verdict == Verdict::RETRY);
    CHECK(retry.exit_code == ExitCode::RETRY);

    const auto fail = compute_result(Mode::CHECK, {
                                                      r(ReasonCode::PROBE_TIMEOUT),
                                                      r(ReasonCode::GPU_COUNT_MISMATCH),
                                                      r(ReasonCode::NVML_LIBRARY_NOT_FOUND),
                                                  });
    CHECK(fail.verdict == Verdict::FAIL);
    CHECK(fail.exit_code == ExitCode::FAIL_STACK);
  }

  SECTION("health and fabric codes fold into the right band") {
    const auto stack = compute_result(Mode::CHECK, {
                                                       r(ReasonCode::ROW_REMAP_FAILURE),
                                                       r(ReasonCode::ECC_MODE_MISMATCH),
                                                   });
    CHECK(stack.exit_code == ExitCode::FAIL_STACK);

    const auto usability = compute_result(Mode::CHECK, {
                                                           r(ReasonCode::FABRIC_NOT_READY),
                                                           r(ReasonCode::GPU_COUNT_MISMATCH),
                                                       });
    CHECK(usability.exit_code == ExitCode::FAIL_USABILITY);
  }

  SECTION("a report-only code is recorded without failing") {
    const auto observed = compute_result(Mode::INVENTORY, {r(ReasonCode::FABRIC_NOT_APPLICABLE)});
    CHECK(observed.verdict == Verdict::OBSERVED);
    CHECK(observed.exit_code == ExitCode::OK);
    REQUIRE(observed.reasons.size() == 1);
    CHECK(observed.reasons[0].code == ReasonCode::FABRIC_NOT_APPLICABLE);
  }
}
