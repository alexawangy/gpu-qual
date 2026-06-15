#include <gpu_qual/verdict.hpp>

#include <utility>

#include <string_view>

namespace gpu_qual {
  namespace {
    int priority(ExitCode exit_code) {
      switch (exit_code) {
        case ExitCode::OK:
          return 0;
        case ExitCode::WARN:
          return 1;
        case ExitCode::RETRY:
          return 2;
        case ExitCode::FAIL_CONTRACT:
          return 3;
        case ExitCode::FAIL_USABILITY:
          return 4;
        case ExitCode::FAIL_STACK:
          return 5;
      }

      return 0;
    }

    Verdict verdict_for(Mode mode, ExitCode exit_code) {
      switch (exit_code) {
        case ExitCode::OK:
          return mode == Mode::INVENTORY ? Verdict::OBSERVED : Verdict::PASS;
        case ExitCode::WARN:
          return Verdict::WARN;
        case ExitCode::RETRY:
          return Verdict::RETRY;
        case ExitCode::FAIL_CONTRACT:
        case ExitCode::FAIL_USABILITY:
        case ExitCode::FAIL_STACK:
          return Verdict::FAIL;
      }

      return Verdict::FAIL;
    }
  }

  std::string_view to_string(Mode mode) {
    switch (mode) {
      case Mode::INVENTORY:
        return "inventory";
      case Mode::CHECK:
        return "check";
    }

    return "unknown";
  }

  std::string_view to_string(Verdict v) {
    switch (v) {
      case Verdict::OBSERVED:
        return "observed";
      case Verdict::PASS:
        return "pass";
      case Verdict::WARN:
        return "warn";
      case Verdict::RETRY:
        return "retry";
      case Verdict::FAIL:
        return "fail";
    }

    return "unknown";
  }

  std::string_view to_string(ReasonClass reason_class) {
    switch (reason_class) {
      case ReasonClass::REPORT:
        return "report";
      case ReasonClass::WARN:
        return "warn";
      case ReasonClass::HARD:
        return "hard";
      case ReasonClass::RETRY:
        return "retry";
    }

    return "unknown";
  }

  std::string_view to_string(ReasonCode rc) {
    switch (rc) {
      case ReasonCode::GPU_COUNT_MISMATCH:
        return "GPU_COUNT_MISMATCH";
      case ReasonCode::GPU_NAME_MISMATCH:
        return "GPU_NAME_MISMATCH";
      case ReasonCode::GPU_MEMORY_BELOW_MIN:
        return "GPU_MEMORY_BELOW_MIN";
      case ReasonCode::MIG_MODE_MISMATCH:
        return "MIG_MODE_MISMATCH";
      case ReasonCode::CUDA_VISIBLE_COUNT_BELOW_MIN:
        return "CUDA_VISIBLE_COUNT_BELOW_MIN";
      case ReasonCode::NVML_LIBRARY_NOT_FOUND:
        return "NVML_LIBRARY_NOT_FOUND";
      case ReasonCode::NVML_INIT_FAILED:
        return "NVML_INIT_FAILED";
      case ReasonCode::NVML_NO_PERMISSION:
        return "NVML_NO_PERMISSION";
      case ReasonCode::NO_NVIDIA_DEVICES:
        return "NO_NVIDIA_DEVICES";
      case ReasonCode::DRIVER_VERSION_BELOW_MIN:
        return "DRIVER_VERSION_BELOW_MIN";
      case ReasonCode::ECC_MODE_MISMATCH:
        return "ECC_MODE_MISMATCH";
      case ReasonCode::ECC_UNCORRECTABLE_DETECTED:
        return "ECC_UNCORRECTABLE_DETECTED";
      case ReasonCode::ROW_REMAP_PENDING:
        return "ROW_REMAP_PENDING";
      case ReasonCode::ROW_REMAP_FAILURE:
        return "ROW_REMAP_FAILURE";
      case ReasonCode::RETIRED_PAGES_PENDING:
        return "RETIRED_PAGES_PENDING";
      case ReasonCode::GPU_RECOVERY_ACTION_REQUIRED:
        return "GPU_RECOVERY_ACTION_REQUIRED";
      case ReasonCode::FABRIC_NOT_READY:
        return "FABRIC_NOT_READY";
      case ReasonCode::FABRIC_NOT_APPLICABLE:
        return "FABRIC_NOT_APPLICABLE";
      case ReasonCode::CUDA_LIBRARY_NOT_FOUND:
        return "CUDA_LIBRARY_NOT_FOUND";
      case ReasonCode::CUDA_INIT_FAILED:
        return "CUDA_INIT_FAILED";
      case ReasonCode::CUDA_CONTEXT_FAILED:
        return "CUDA_CONTEXT_FAILED";
      case ReasonCode::CUDA_SMOKE_FAILED:
        return "CUDA_SMOKE_FAILED";
      case ReasonCode::EXPECTED_SPEC_INVALID:
        return "EXPECTED_SPEC_INVALID";
      case ReasonCode::PROBE_TIMEOUT:
        return "PROBE_TIMEOUT";
      case ReasonCode::PROBE_CHILD_CRASHED:
        return "PROBE_CHILD_CRASHED";
      case ReasonCode::PROBE_OUTPUT_INVALID:
        return "PROBE_OUTPUT_INVALID";
      case ReasonCode::FIELD_UNSUPPORTED:
        return "FIELD_UNSUPPORTED";
      case ReasonCode::UNKNOWN_FIELD_IGNORED:
        return "UNKNOWN_FIELD_IGNORED";
    }

    return "UNKNOWN";
  }

  ReasonClass default_class(ReasonCode rc) {
    switch (rc) {
      case ReasonCode::PROBE_TIMEOUT:
        return ReasonClass::RETRY;
      case ReasonCode::CUDA_VISIBLE_COUNT_BELOW_MIN:
      case ReasonCode::FABRIC_NOT_APPLICABLE:
      case ReasonCode::FIELD_UNSUPPORTED:
      case ReasonCode::UNKNOWN_FIELD_IGNORED:
        return ReasonClass::REPORT;
      case ReasonCode::DRIVER_VERSION_BELOW_MIN:
      case ReasonCode::RETIRED_PAGES_PENDING:
        return ReasonClass::WARN;
      case ReasonCode::GPU_COUNT_MISMATCH:
      case ReasonCode::GPU_NAME_MISMATCH:
      case ReasonCode::GPU_MEMORY_BELOW_MIN:
      case ReasonCode::MIG_MODE_MISMATCH:
      case ReasonCode::NVML_LIBRARY_NOT_FOUND:
      case ReasonCode::NVML_INIT_FAILED:
      case ReasonCode::NVML_NO_PERMISSION:
      case ReasonCode::NO_NVIDIA_DEVICES:
      case ReasonCode::CUDA_LIBRARY_NOT_FOUND:
      case ReasonCode::CUDA_INIT_FAILED:
      case ReasonCode::CUDA_CONTEXT_FAILED:
      case ReasonCode::CUDA_SMOKE_FAILED:
      case ReasonCode::EXPECTED_SPEC_INVALID:
      case ReasonCode::PROBE_CHILD_CRASHED:
      case ReasonCode::PROBE_OUTPUT_INVALID:
      case ReasonCode::ECC_MODE_MISMATCH:
      case ReasonCode::ECC_UNCORRECTABLE_DETECTED:
      case ReasonCode::ROW_REMAP_PENDING:
      case ReasonCode::ROW_REMAP_FAILURE:
      case ReasonCode::GPU_RECOVERY_ACTION_REQUIRED:
      case ReasonCode::FABRIC_NOT_READY:
        return ReasonClass::HARD;
    }

    return ReasonClass::HARD;
  }

  ExitCode default_exit_code(ReasonCode rc) {
    switch (rc) {
      case ReasonCode::FABRIC_NOT_APPLICABLE:
        return ExitCode::OK;
      case ReasonCode::GPU_COUNT_MISMATCH:
      case ReasonCode::GPU_NAME_MISMATCH:
      case ReasonCode::GPU_MEMORY_BELOW_MIN:
      case ReasonCode::MIG_MODE_MISMATCH:
      case ReasonCode::ECC_MODE_MISMATCH:
      case ReasonCode::ECC_UNCORRECTABLE_DETECTED:
      case ReasonCode::ROW_REMAP_PENDING:
        return ExitCode::FAIL_CONTRACT;
      case ReasonCode::CUDA_LIBRARY_NOT_FOUND:
      case ReasonCode::CUDA_INIT_FAILED:
      case ReasonCode::CUDA_CONTEXT_FAILED:
      case ReasonCode::CUDA_SMOKE_FAILED:
      case ReasonCode::FABRIC_NOT_READY:
        return ExitCode::FAIL_USABILITY;
      case ReasonCode::PROBE_TIMEOUT:
        return ExitCode::RETRY;
      case ReasonCode::CUDA_VISIBLE_COUNT_BELOW_MIN:
      case ReasonCode::FIELD_UNSUPPORTED:
      case ReasonCode::UNKNOWN_FIELD_IGNORED:
      case ReasonCode::DRIVER_VERSION_BELOW_MIN:
      case ReasonCode::RETIRED_PAGES_PENDING:
        return ExitCode::WARN;
      case ReasonCode::NVML_LIBRARY_NOT_FOUND:
      case ReasonCode::NVML_INIT_FAILED:
      case ReasonCode::NVML_NO_PERMISSION:
      case ReasonCode::NO_NVIDIA_DEVICES:
      case ReasonCode::EXPECTED_SPEC_INVALID:
      case ReasonCode::PROBE_CHILD_CRASHED:
      case ReasonCode::PROBE_OUTPUT_INVALID:
      case ReasonCode::ROW_REMAP_FAILURE:
      case ReasonCode::GPU_RECOVERY_ACTION_REQUIRED:
        return ExitCode::FAIL_STACK;
    }

    return ExitCode::FAIL_STACK;
  }

  Result compute_result(Mode mode, std::vector<Reason> reasons) {
    auto exit_code = ExitCode::OK;
    for (const auto &reason : reasons) {
      const auto candidate = default_exit_code(reason.code);
      if (priority(candidate) > priority(exit_code)) {
        exit_code = candidate;
      }
    }

    Result res{
      .mode = mode,
      .verdict = verdict_for(mode, exit_code),
      .exit_code = exit_code,
      .reasons = std::move(reasons),
    };

    return res;
  }

  Reason make_reason(ReasonCode rc, std::string field, nlohmann::json expected, nlohmann::json observed) {
    Reason reason = {
      .code = rc,
      .cls = default_class(rc),
      .field = std::move(field),
      .expected = std::move(expected),
      .observed = std::move(observed),
    };

    return reason;
  }
}
