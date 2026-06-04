#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "version.hpp"

namespace gpu_qual {
  enum class Verdict { ASSIGN, ASSIGN_WITH_WARNINGS, RETRY, QUARANTINE };
  enum class ExitCode : int { ASSIGN=0, ASSIGN_WITH_WARNINGS=10, RETRY=20, QUARANTINE_CONTRACT=30, QUARANTINE_USABILITY=40, STACK_ABSENT=50 };

  // Add as we go
  enum class ReasonCode {
    GPU_COUNT_MISMATCH,
    GPU_FAMILY_MISMATCH,
    MEMORY_BELOW_FLOOR,
    MIG_MODE_MISMATCH,

    CUDA_CONTEXT_FAILED,
    COMPUTE_SMOKE_FAILED,

    DRIVER_ABSENT,
    NVML_INIT_FAILED,

    DRIVER_LOADING,
    NVML_TIMEOUT,

    P2P_DEGRADED,
    DRIVER_BRANCH_BELOW_MIN,
  };

  std::string_view to_string(Verdict);
  std::string_view to_string(ReasonCode);

  struct Result {
    std::string tool_version = kToolVersion;
    std::string schema_version = kSchemaVersion;
    Verdict verdict;
    ExitCode exit_code;
    std::vector<ReasonCode> reasons;
  };

  Result compute_result(std::vector<ReasonCode>);
}
