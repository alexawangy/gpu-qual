#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "gpu_qual/observed.hpp"
#include "version.hpp"

#include <nlohmann/json.hpp>

namespace gpu_qual {
// Namespace-scoped so code in gpu_qual can say `json` without leaking the
// alias into every includer's global namespace.
using json = nlohmann::json;

enum class Mode { INVENTORY, CHECK };
enum class Verdict { OBSERVED, PASS, WARN, RETRY, FAIL };
enum class ExitCode : int {
  OK = 0,
  WARN = 10,
  RETRY = 20,
  FAIL_CONTRACT = 30,
  FAIL_USABILITY = 40,
  FAIL_STACK = 50
};
enum class ReasonClass { REPORT, WARN, HARD, RETRY };

enum class ReasonCode {
  GPU_COUNT_MISMATCH,
  GPU_NAME_MISMATCH,
  GPU_MEMORY_BELOW_MIN,
  NVML_LIBRARY_NOT_FOUND,
  NVML_INIT_FAILED,
  NVML_NO_PERMISSION,
  NO_NVIDIA_DEVICES,
  DRIVER_VERSION_BELOW_MIN,
  EXPECTED_SPEC_INVALID,
  PROBE_OUTPUT_INVALID,
};

std::string_view to_string(Mode);
std::string_view to_string(Verdict);
std::string_view to_string(ReasonClass);
std::string_view to_string(ReasonCode);
ReasonClass default_class(ReasonCode);
ExitCode default_exit_code(ReasonCode);

struct Reason {
  ReasonCode code;
  std::string field;
  std::optional<json> expected;
  std::optional<json> observed;
};

struct Result {
  std::string_view tool_version = kToolVersion;
  std::string_view schema_version = kSchemaVersion;
  Mode mode;
  Verdict verdict;
  ExitCode exit_code;
  std::vector<Reason> reasons;
  std::optional<ObservedState> observed;
};

Result compute_result(Mode, std::vector<Reason>);

Reason make_reason(ReasonCode code, std::string field = {},
                   std::optional<json> expected = std::nullopt,
                   std::optional<json> observed = std::nullopt);
} // namespace gpu_qual
