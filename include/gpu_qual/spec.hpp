#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <gpu_qual/observed.hpp>
#include <gpu_qual/verdict.hpp>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace gpu_qual {
enum class Severity { HARD, WARN, REPORT };
std::string_view to_string(Severity);
std::optional<Severity> severity_from_string(std::string_view);

enum class MigExpectation { ENABLED, DISABLED, ANY };
std::string_view to_string(MigExpectation);
std::optional<MigExpectation> mig_expectation_from_string(std::string_view);
std::optional<RecoveryAction> recovery_action_from_string(std::string_view);

struct ExpectedHealth {
  std::optional<bool> ecc_mode_enabled;
  std::optional<long long> max_volatile_uncorrectable_ecc;
  std::optional<long long> max_aggregate_uncorrectable_ecc;
  std::optional<bool> allow_row_remap_pending;
  std::optional<bool> allow_row_remap_failure;
  std::optional<bool> allow_pending_retired_pages;
  std::vector<RecoveryAction> disallowed_recovery_actions;
};

struct ExpectedFabric {
  std::optional<bool> require_fabric_ready;
};

struct Expected {
  std::optional<int> gpu_count;
  std::vector<std::string> allowed_names;
  std::optional<long long> min_memory_mib;
  std::optional<MigExpectation> mig_mode;
  std::optional<int> min_cuda_visible_devices;
  std::optional<std::string> min_driver_version;
  ExpectedHealth health;
  ExpectedFabric fabric;
};

struct Policy {
  Severity gpu_count = Severity::HARD;
  Severity gpu_name = Severity::HARD;
  Severity memory = Severity::HARD;
  Severity mig_mode = Severity::HARD;
  Severity cuda_visible_devices = Severity::REPORT;
  Severity driver_version = Severity::WARN;
  Severity ecc = Severity::HARD;
  Severity row_remap = Severity::HARD;
  Severity retired_pages = Severity::WARN;
  Severity recovery_action = Severity::HARD;
  Severity fabric = Severity::HARD;
  Severity cuda_smoke = Severity::HARD;
};

struct Options {
  bool cuda_smoke = false;
  bool include_health = true;
  int timeout_ms = 10000;
  int cuda_smoke_timeout_ms = 5000;
};

struct ExpectedSpec {
  std::string schema_version;
  json metadata; // opaque, verbatim caller passthrough; null when absent
  Expected expected;
  Policy policy;
  Options options;
};

struct SpecParseResult {
  std::optional<ExpectedSpec> spec;
  std::vector<Reason> reasons;

  bool ok() const {
    return spec.has_value();
  }
};

// Upper bound on the serialized size of caller `metadata`. The CLI/result code
// should reference this same constant so the limit stays in one place.
constexpr std::size_t kMaxMetadataBytes = 16 * 1024;

SpecParseResult parse_spec(std::string_view json_text);
} // namespace gpu_qual
