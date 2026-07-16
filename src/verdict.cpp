#include <gpu_qual/verdict.hpp>

#include <utility>

namespace gpu_qual {
namespace {
int priority(ExitCode exit_code) {
  switch (exit_code) {
  case ExitCode::OK: return 0;
  case ExitCode::WARN: return 1;
  case ExitCode::RETRY: return 2;
  case ExitCode::FAIL_CONTRACT: return 3;
  case ExitCode::FAIL_USABILITY: return 4;
  case ExitCode::FAIL_STACK: return 5;
  }

  return 0;
}

Verdict verdict_for(Mode mode, ExitCode exit_code) {
  switch (exit_code) {
  case ExitCode::OK: return mode == Mode::INVENTORY ? Verdict::OBSERVED : Verdict::PASS;
  case ExitCode::WARN: return Verdict::WARN;
  case ExitCode::RETRY: return Verdict::RETRY;
  case ExitCode::FAIL_CONTRACT:
  case ExitCode::FAIL_USABILITY:
  case ExitCode::FAIL_STACK: return Verdict::FAIL;
  }

  return Verdict::FAIL;
}
} // namespace

std::string_view to_string(Mode mode) {
  switch (mode) {
  case Mode::INVENTORY: return "inventory";
  case Mode::CHECK: return "check";
  }

  return "unknown";
}

std::string_view to_string(Verdict verdict) {
  switch (verdict) {
  case Verdict::OBSERVED: return "observed";
  case Verdict::PASS: return "pass";
  case Verdict::WARN: return "warn";
  case Verdict::RETRY: return "retry";
  case Verdict::FAIL: return "fail";
  }

  return "unknown";
}

std::string_view to_string(ReasonClass reason_class) {
  switch (reason_class) {
  case ReasonClass::REPORT: return "report";
  case ReasonClass::WARN: return "warn";
  case ReasonClass::HARD: return "hard";
  case ReasonClass::RETRY: return "retry";
  }

  return "unknown";
}

std::string_view to_string(ReasonCode reason_code) {
  switch (reason_code) {
  case ReasonCode::GPU_COUNT_MISMATCH: return "GPU_COUNT_MISMATCH";
  case ReasonCode::GPU_NAME_MISMATCH: return "GPU_NAME_MISMATCH";
  case ReasonCode::GPU_MEMORY_BELOW_MIN: return "GPU_MEMORY_BELOW_MIN";
  case ReasonCode::NVML_LIBRARY_NOT_FOUND: return "NVML_LIBRARY_NOT_FOUND";
  case ReasonCode::NVML_INIT_FAILED: return "NVML_INIT_FAILED";
  case ReasonCode::NVML_NO_PERMISSION: return "NVML_NO_PERMISSION";
  case ReasonCode::NO_NVIDIA_DEVICES: return "NO_NVIDIA_DEVICES";
  case ReasonCode::DRIVER_VERSION_BELOW_MIN: return "DRIVER_VERSION_BELOW_MIN";
  case ReasonCode::EXPECTED_SPEC_INVALID: return "EXPECTED_SPEC_INVALID";
  case ReasonCode::PROBE_OUTPUT_INVALID: return "PROBE_OUTPUT_INVALID";
  }

  return "UNKNOWN";
}

ReasonClass default_class(ReasonCode reason_code) {
  switch (reason_code) {
  case ReasonCode::GPU_COUNT_MISMATCH:
  case ReasonCode::GPU_NAME_MISMATCH:
  case ReasonCode::GPU_MEMORY_BELOW_MIN:
  case ReasonCode::DRIVER_VERSION_BELOW_MIN:
  case ReasonCode::NVML_LIBRARY_NOT_FOUND:
  case ReasonCode::NVML_INIT_FAILED:
  case ReasonCode::NVML_NO_PERMISSION:
  case ReasonCode::NO_NVIDIA_DEVICES:
  case ReasonCode::EXPECTED_SPEC_INVALID:
  case ReasonCode::PROBE_OUTPUT_INVALID: return ReasonClass::HARD;
  }

  return ReasonClass::HARD;
}

ExitCode default_exit_code(ReasonCode reason_code) {
  switch (reason_code) {
  case ReasonCode::GPU_COUNT_MISMATCH:
  case ReasonCode::GPU_NAME_MISMATCH:
  case ReasonCode::GPU_MEMORY_BELOW_MIN:
  case ReasonCode::DRIVER_VERSION_BELOW_MIN: return ExitCode::FAIL_CONTRACT;
  case ReasonCode::NVML_LIBRARY_NOT_FOUND:
  case ReasonCode::NVML_INIT_FAILED:
  case ReasonCode::NVML_NO_PERMISSION:
  case ReasonCode::NO_NVIDIA_DEVICES:
  case ReasonCode::EXPECTED_SPEC_INVALID:
  case ReasonCode::PROBE_OUTPUT_INVALID: return ExitCode::FAIL_STACK;
  }

  return ExitCode::FAIL_STACK;
}

Result compute_result(Mode mode, std::vector<Reason> reasons) {
  auto exit_code = ExitCode::OK;
  for (const auto& reason : reasons) {
    const auto candidate = default_exit_code(reason.code);
    if (priority(candidate) > priority(exit_code)) {
      exit_code = candidate;
    }
  }

  return Result{
      .mode = mode,
      .verdict = verdict_for(mode, exit_code),
      .exit_code = exit_code,
      .reasons = std::move(reasons),
  };
}

Reason make_reason(ReasonCode code, std::string field, std::optional<json> expected,
                   std::optional<json> observed) {
  return Reason{
      .code = code,
      .field = std::move(field),
      .expected = std::move(expected),
      .observed = std::move(observed),
  };
}
} // namespace gpu_qual
