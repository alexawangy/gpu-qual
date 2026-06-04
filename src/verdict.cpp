#include <gpu_qual/verdict.hpp>
#include <string_view>

namespace gpu_qual {
  std::string_view to_string(Verdict v) {
    switch (v) {
      case Verdict::ASSIGN:
        return "assign";
      case Verdict::ASSIGN_WITH_WARNINGS:
        return "assign_with_warnings";
      case Verdict::RETRY:
        return "retry";
      case Verdict::QUARANTINE:
        return "quarantine";
    }
  }

  std::string_view to_string(ReasonCode rc) {
    switch (rc) {
      case ReasonCode::GPU_COUNT_MISMATCH:
        return "GPU_COUNT_MISMATCH";
      case ReasonCode::GPU_FAMILY_MISMATCH:
        return "GPU_FAMILY_MISMATCH";
      case ReasonCode::MEMORY_BELOW_FLOOR:
        return "MEMORY_BELOW_FLOOR";
      case ReasonCode::MIG_MODE_MISMATCH:
        return "MIG_MODE_MISMATCH";
      case ReasonCode::CUDA_CONTEXT_FAILED:
        return "CUDA_CONTEXT_FAILED";
      case ReasonCode::COMPUTE_SMOKE_FAILED:
        return "COMPUTE_SMOKE_FAILED";
      case ReasonCode::DRIVER_ABSENT:
        return "DRIVER_ABSENT";
      case ReasonCode::NVML_INIT_FAILED:
        return "NVML_INIT_FAILED";
      case ReasonCode::DRIVER_LOADING:
        return "DRIVER_LOADING";
      case ReasonCode::NVML_TIMEOUT:
        return "NVML_TIMEOUT";
      case ReasonCode::P2P_DEGRADED:
        return "P2P_DEGRADED";
      case ReasonCode::DRIVER_BRANCH_BELOW_MIN:
        return "DRIVER_BRANCH_BELOW_MIN";
    }
  }

  Result compute_result(std::vector<ReasonCode> reasons) {
    Result res = Result {
      .verdict = Verdict::ASSIGN,
      .exit_code = ExitCode::ASSIGN,
      .reasons = {},
    };

    return res;
  }
}
