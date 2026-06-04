#include <catch2/catch_test_macros.hpp>

#include <array>
#include <string>
#include <string_view>
#include <vector>

#include <gpu_qual/verdict.hpp>
#include <gpu_qual/version.hpp>

TEST_CASE("version constants match the frozen v0.3 contract") {
  CHECK(std::string{gpu_qual::kToolVersion} == "gpu-qual/0.3.0");
  CHECK(std::string{gpu_qual::kSchemaVersion} == "0.3");
}

TEST_CASE("to_string returns stable names for all modes") {
  using gpu_qual::Mode;

  constexpr std::array cases{
    std::pair{Mode::INVENTORY, std::string_view{"inventory"}},
    std::pair{Mode::CHECK, std::string_view{"check"}},
  };

  for (const auto& [mode, name] : cases) {
    CAPTURE(std::string{name});
    CHECK(std::string{gpu_qual::to_string(mode)} == std::string{name});
  }
}

TEST_CASE("to_string returns stable names for all verdicts") {
  using gpu_qual::Verdict;

  constexpr std::array cases{
    std::pair{Verdict::OBSERVED, std::string_view{"observed"}},
    std::pair{Verdict::PASS, std::string_view{"pass"}},
    std::pair{Verdict::WARN, std::string_view{"warn"}},
    std::pair{Verdict::RETRY, std::string_view{"retry"}},
    std::pair{Verdict::FAIL, std::string_view{"fail"}},
  };

  for (const auto& [verdict, name] : cases) {
    CAPTURE(std::string{name});
    CHECK(std::string{gpu_qual::to_string(verdict)} == std::string{name});
  }
}

TEST_CASE("exit code values match the v0.3 routing contract") {
  using gpu_qual::ExitCode;

  CHECK(static_cast<int>(ExitCode::OK) == 0);
  CHECK(static_cast<int>(ExitCode::WARN) == 10);
  CHECK(static_cast<int>(ExitCode::RETRY) == 20);
  CHECK(static_cast<int>(ExitCode::FAIL_CONTRACT) == 30);
  CHECK(static_cast<int>(ExitCode::FAIL_USABILITY) == 40);
  CHECK(static_cast<int>(ExitCode::FAIL_STACK) == 50);
}

TEST_CASE("to_string returns stable names for all reason classes") {
  using gpu_qual::ReasonClass;

  constexpr std::array cases{
    std::pair{ReasonClass::REPORT, std::string_view{"report"}},
    std::pair{ReasonClass::WARN, std::string_view{"warn"}},
    std::pair{ReasonClass::HARD, std::string_view{"hard"}},
    std::pair{ReasonClass::RETRY, std::string_view{"retry"}},
  };

  for (const auto& [reason_class, name] : cases) {
    CAPTURE(std::string{name});
    CHECK(std::string{gpu_qual::to_string(reason_class)} == std::string{name});
  }
}

TEST_CASE("to_string returns stable names for all reason codes") {
  using gpu_qual::ReasonCode;

  constexpr std::array cases{
    std::pair{ReasonCode::GPU_COUNT_MISMATCH, std::string_view{"GPU_COUNT_MISMATCH"}},
    std::pair{ReasonCode::GPU_NAME_MISMATCH, std::string_view{"GPU_NAME_MISMATCH"}},
    std::pair{ReasonCode::GPU_MEMORY_BELOW_MIN, std::string_view{"GPU_MEMORY_BELOW_MIN"}},
    std::pair{ReasonCode::MIG_MODE_MISMATCH, std::string_view{"MIG_MODE_MISMATCH"}},
    std::pair{ReasonCode::CUDA_VISIBLE_COUNT_BELOW_MIN, std::string_view{"CUDA_VISIBLE_COUNT_BELOW_MIN"}},
    std::pair{ReasonCode::NVML_LIBRARY_NOT_FOUND, std::string_view{"NVML_LIBRARY_NOT_FOUND"}},
    std::pair{ReasonCode::NVML_INIT_FAILED, std::string_view{"NVML_INIT_FAILED"}},
    std::pair{ReasonCode::NVML_NO_PERMISSION, std::string_view{"NVML_NO_PERMISSION"}},
    std::pair{ReasonCode::NO_NVIDIA_DEVICES, std::string_view{"NO_NVIDIA_DEVICES"}},
    std::pair{ReasonCode::CUDA_LIBRARY_NOT_FOUND, std::string_view{"CUDA_LIBRARY_NOT_FOUND"}},
    std::pair{ReasonCode::CUDA_INIT_FAILED, std::string_view{"CUDA_INIT_FAILED"}},
    std::pair{ReasonCode::CUDA_CONTEXT_FAILED, std::string_view{"CUDA_CONTEXT_FAILED"}},
    std::pair{ReasonCode::CUDA_SMOKE_FAILED, std::string_view{"CUDA_SMOKE_FAILED"}},
    std::pair{ReasonCode::EXPECTED_SPEC_INVALID, std::string_view{"EXPECTED_SPEC_INVALID"}},
    std::pair{ReasonCode::PROBE_TIMEOUT, std::string_view{"PROBE_TIMEOUT"}},
    std::pair{ReasonCode::PROBE_CHILD_CRASHED, std::string_view{"PROBE_CHILD_CRASHED"}},
    std::pair{ReasonCode::PROBE_OUTPUT_INVALID, std::string_view{"PROBE_OUTPUT_INVALID"}},
    std::pair{ReasonCode::FIELD_UNSUPPORTED, std::string_view{"FIELD_UNSUPPORTED"}},
    std::pair{ReasonCode::UNKNOWN_FIELD_IGNORED, std::string_view{"UNKNOWN_FIELD_IGNORED"}},
  };

  for (const auto& [reason_code, name] : cases) {
    CAPTURE(std::string{name});
    CHECK(std::string{gpu_qual::to_string(reason_code)} == std::string{name});
  }
}

TEST_CASE("reason classes match the v0.3 defaults") {
  using gpu_qual::ReasonClass;
  using gpu_qual::ReasonCode;

  constexpr std::array hard_cases{
    ReasonCode::GPU_COUNT_MISMATCH,
    ReasonCode::GPU_NAME_MISMATCH,
    ReasonCode::GPU_MEMORY_BELOW_MIN,
    ReasonCode::MIG_MODE_MISMATCH,
    ReasonCode::NVML_LIBRARY_NOT_FOUND,
    ReasonCode::NVML_INIT_FAILED,
    ReasonCode::NVML_NO_PERMISSION,
    ReasonCode::NO_NVIDIA_DEVICES,
    ReasonCode::CUDA_LIBRARY_NOT_FOUND,
    ReasonCode::CUDA_INIT_FAILED,
    ReasonCode::CUDA_CONTEXT_FAILED,
    ReasonCode::CUDA_SMOKE_FAILED,
    ReasonCode::EXPECTED_SPEC_INVALID,
    ReasonCode::PROBE_CHILD_CRASHED,
    ReasonCode::PROBE_OUTPUT_INVALID,
  };

  for (const auto reason_code : hard_cases) {
    CHECK(gpu_qual::default_class(reason_code) == ReasonClass::HARD);
  }

  CHECK(gpu_qual::default_class(ReasonCode::PROBE_TIMEOUT) == ReasonClass::RETRY);
  CHECK(gpu_qual::default_class(ReasonCode::CUDA_VISIBLE_COUNT_BELOW_MIN) == ReasonClass::REPORT);
  CHECK(gpu_qual::default_class(ReasonCode::FIELD_UNSUPPORTED) == ReasonClass::REPORT);
  CHECK(gpu_qual::default_class(ReasonCode::UNKNOWN_FIELD_IGNORED) == ReasonClass::REPORT);
}

TEST_CASE("reason exit code defaults match the v0.3 exit bands") {
  using gpu_qual::ExitCode;
  using gpu_qual::ReasonCode;

  CHECK(gpu_qual::default_exit_code(ReasonCode::GPU_COUNT_MISMATCH) == ExitCode::FAIL_CONTRACT);
  CHECK(gpu_qual::default_exit_code(ReasonCode::GPU_NAME_MISMATCH) == ExitCode::FAIL_CONTRACT);
  CHECK(gpu_qual::default_exit_code(ReasonCode::GPU_MEMORY_BELOW_MIN) == ExitCode::FAIL_CONTRACT);
  CHECK(gpu_qual::default_exit_code(ReasonCode::MIG_MODE_MISMATCH) == ExitCode::FAIL_CONTRACT);

  CHECK(gpu_qual::default_exit_code(ReasonCode::CUDA_LIBRARY_NOT_FOUND) == ExitCode::FAIL_USABILITY);
  CHECK(gpu_qual::default_exit_code(ReasonCode::CUDA_INIT_FAILED) == ExitCode::FAIL_USABILITY);
  CHECK(gpu_qual::default_exit_code(ReasonCode::CUDA_CONTEXT_FAILED) == ExitCode::FAIL_USABILITY);
  CHECK(gpu_qual::default_exit_code(ReasonCode::CUDA_SMOKE_FAILED) == ExitCode::FAIL_USABILITY);

  CHECK(gpu_qual::default_exit_code(ReasonCode::PROBE_TIMEOUT) == ExitCode::RETRY);
  CHECK(gpu_qual::default_exit_code(ReasonCode::FIELD_UNSUPPORTED) == ExitCode::WARN);
}

TEST_CASE("compute_result maps inventory and check success to distinct v0.3 verdicts") {
  const auto inventory_result = gpu_qual::compute_result(gpu_qual::Mode::INVENTORY, {});
  CHECK(inventory_result.verdict == gpu_qual::Verdict::OBSERVED);
  CHECK(inventory_result.exit_code == gpu_qual::ExitCode::OK);

  const auto check_result = gpu_qual::compute_result(gpu_qual::Mode::CHECK, {});
  CHECK(check_result.verdict == gpu_qual::Verdict::PASS);
  CHECK(check_result.exit_code == gpu_qual::ExitCode::OK);
}

TEST_CASE("compute_result chooses the highest-impact v0.3 exit band") {
  const auto retry_result = gpu_qual::compute_result(
    gpu_qual::Mode::INVENTORY,
    std::vector{gpu_qual::ReasonCode::PROBE_TIMEOUT}
  );
  CHECK(retry_result.verdict == gpu_qual::Verdict::RETRY);
  CHECK(retry_result.exit_code == gpu_qual::ExitCode::RETRY);

  const auto fail_result = gpu_qual::compute_result(
    gpu_qual::Mode::CHECK,
    std::vector{
      gpu_qual::ReasonCode::PROBE_TIMEOUT,
      gpu_qual::ReasonCode::GPU_COUNT_MISMATCH,
      gpu_qual::ReasonCode::NVML_LIBRARY_NOT_FOUND,
    }
  );
  CHECK(fail_result.verdict == gpu_qual::Verdict::FAIL);
  CHECK(fail_result.exit_code == gpu_qual::ExitCode::FAIL_STACK);
}
