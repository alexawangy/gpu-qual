#include <catch2/catch_test_macros.hpp>

#include <array>
#include <string>
#include <string_view>

#include <gpu_qual/verdict.hpp>

TEST_CASE("to_string returns stable names for all reason codes") {
  using gpu_qual::ReasonCode;

  constexpr std::array cases{
    std::pair{ReasonCode::GPU_COUNT_MISMATCH, std::string_view{"GPU_COUNT_MISMATCH"}},
    std::pair{ReasonCode::GPU_FAMILY_MISMATCH, std::string_view{"GPU_FAMILY_MISMATCH"}},
    std::pair{ReasonCode::MEMORY_BELOW_FLOOR, std::string_view{"MEMORY_BELOW_FLOOR"}},
    std::pair{ReasonCode::MIG_MODE_MISMATCH, std::string_view{"MIG_MODE_MISMATCH"}},
    std::pair{ReasonCode::CUDA_CONTEXT_FAILED, std::string_view{"CUDA_CONTEXT_FAILED"}},
    std::pair{ReasonCode::COMPUTE_SMOKE_FAILED, std::string_view{"COMPUTE_SMOKE_FAILED"}},
    std::pair{ReasonCode::DRIVER_ABSENT, std::string_view{"DRIVER_ABSENT"}},
    std::pair{ReasonCode::NVML_INIT_FAILED, std::string_view{"NVML_INIT_FAILED"}},
    std::pair{ReasonCode::DRIVER_LOADING, std::string_view{"DRIVER_LOADING"}},
    std::pair{ReasonCode::NVML_TIMEOUT, std::string_view{"NVML_TIMEOUT"}},
    std::pair{ReasonCode::P2P_DEGRADED, std::string_view{"P2P_DEGRADED"}},
    std::pair{ReasonCode::DRIVER_BRANCH_BELOW_MIN, std::string_view{"DRIVER_BRANCH_BELOW_MIN"}},
  };

  for (const auto& [reason_code, name] : cases) {
    CAPTURE(std::string{name});
    CHECK(std::string{gpu_qual::to_string(reason_code)} == std::string{name});
  }
}
