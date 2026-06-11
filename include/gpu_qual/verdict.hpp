#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "version.hpp"

namespace gpu_qual {
  enum class Mode { INVENTORY, CHECK };
  enum class Verdict { OBSERVED, PASS, WARN, RETRY, FAIL };
  enum class ExitCode : int { OK = 0, WARN = 10, RETRY = 20, FAIL_CONTRACT = 30, FAIL_USABILITY = 40, FAIL_STACK = 50 };
  enum class ReasonClass { REPORT, WARN, HARD, RETRY };

  enum class ReasonCode {
    GPU_COUNT_MISMATCH,
    GPU_NAME_MISMATCH,
    GPU_MEMORY_BELOW_MIN,
    MIG_MODE_MISMATCH,

    CUDA_VISIBLE_COUNT_BELOW_MIN,

    NVML_LIBRARY_NOT_FOUND,
    NVML_INIT_FAILED,
    NVML_NO_PERMISSION,
    NO_NVIDIA_DEVICES,

    CUDA_LIBRARY_NOT_FOUND,
    CUDA_INIT_FAILED,
    CUDA_CONTEXT_FAILED,
    CUDA_SMOKE_FAILED,

    EXPECTED_SPEC_INVALID,

    PROBE_TIMEOUT,
    PROBE_CHILD_CRASHED,
    PROBE_OUTPUT_INVALID,

    FIELD_UNSUPPORTED,
    UNKNOWN_FIELD_IGNORED,
  };

  std::string_view to_string(Mode);
  std::string_view to_string(Verdict);
  std::string_view to_string(ReasonClass);
  std::string_view to_string(ReasonCode);
  ReasonClass default_class(ReasonCode);
  ExitCode default_exit_code(ReasonCode);

  struct Reason {
    ReasonCode code;
    ReasonClass cls;
    std::string field;
    std::string expected_val;
    std::string observed_val;
  };

  struct Result {
    std::string tool_version = kToolVersion;
    std::string schema_version = kSchemaVersion;
    Mode mode;
    Verdict verdict;
    ExitCode exit_code;
    std::vector<Reason> reasons;
  };

  Result compute_result(Mode, std::vector<Reason>);
}
