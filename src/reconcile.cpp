#include <gpu_qual/reconcile.hpp>

#include <algorithm>
#include <charconv>
#include <compare>
#include <cstddef>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace gpu_qual {
namespace {

bool mig_matches(MigExpectation expected, MigMode observed) {
  switch (expected) {
  case MigExpectation::ANY: return true;
  case MigExpectation::ENABLED: return observed == MigMode::ENABLED;
  case MigExpectation::DISABLED: return observed == MigMode::DISABLED;
  }

  return false;
}

void check_identity(std::vector<Reason>& reasons, const ExpectedSpec& spec,
                    const ObservedState& observed) {
  if (spec.expected.gpu_count.has_value()) {
    int expected_gpu_count = *spec.expected.gpu_count;
    int observed_gpu_count = static_cast<int>(observed.gpus.size());

    if (expected_gpu_count != observed_gpu_count) {
      reasons.push_back(make_reason(ReasonCode::GPU_COUNT_MISMATCH, "gpu_count", expected_gpu_count,
                                    observed_gpu_count));
    }
  }

  for (const GpuInfo& gpu : observed.gpus) {
    if (!spec.expected.allowed_names.empty() &&
        std::find(spec.expected.allowed_names.begin(), spec.expected.allowed_names.end(),
                  gpu.name) == spec.expected.allowed_names.end()) {

      reasons.push_back(make_reason(ReasonCode::GPU_NAME_MISMATCH, "gpu_name",
                                    spec.expected.allowed_names, gpu.name));
    }

    if (spec.expected.min_memory_mib.has_value() &&
        gpu.memory_mib < *spec.expected.min_memory_mib) {
      reasons.push_back(make_reason(ReasonCode::GPU_MEMORY_BELOW_MIN, "gpu_memory",
                                    *spec.expected.min_memory_mib, gpu.memory_mib));
    }

    if (!spec.expected.mig_mode.has_value())
      continue;

    if (mig_matches(*spec.expected.mig_mode, gpu.mig_mode))
      continue;

    reasons.push_back(make_reason(ReasonCode::MIG_MODE_MISMATCH, "mig_mode",
                                  to_string(*spec.expected.mig_mode), to_string(gpu.mig_mode)));
  }
}

void check_driver(std::vector<Reason>& reasons, const ExpectedSpec& spec,
                  const ObservedState& observed) {
  if (!spec.expected.min_driver_version.has_value()) {
    return;
  }

  if (!observed.nvml.driver_version.has_value()) {
    return;
  }

  if (compare_driver_versions(*observed.nvml.driver_version, *spec.expected.min_driver_version) ==
      std::strong_ordering::less) {
    reasons.push_back(make_reason(ReasonCode::DRIVER_VERSION_BELOW_MIN, "driver_version",
                                  *spec.expected.min_driver_version,
                                  *observed.nvml.driver_version));
  }
}

void check_cuda(std::vector<Reason>& reasons, const ExpectedSpec& spec,
                const ObservedState& observed) {
  if (spec.expected.min_cuda_visible_devices.has_value() &&
      observed.cuda.device_count.has_value() &&
      *observed.cuda.device_count < *spec.expected.min_cuda_visible_devices) {
    reasons.push_back(
        make_reason(ReasonCode::CUDA_VISIBLE_COUNT_BELOW_MIN, "cuda.visible_device_count",
                    *spec.expected.min_cuda_visible_devices, *observed.cuda.device_count));
  }

  if (spec.options.cuda_smoke && !observed.cuda.smoke_passed.value_or(false)) {
    reasons.push_back(make_reason(ReasonCode::CUDA_SMOKE_FAILED, "cuda.smoke_passed", true,
                                  observed.cuda.smoke_passed.value_or(false)));
  }
}

void check_health(std::vector<Reason>& reasons, const ExpectedSpec& spec,
                  const ObservedState& observed) {
  for (const GpuInfo& gpu : observed.gpus) {
    if (!gpu.health.has_value()) {
      continue;
    }

    const GpuHealth& health = *gpu.health;

    if (spec.expected.health.ecc_mode_enabled.has_value() && health.ecc_mode_enabled.has_value() &&
        *health.ecc_mode_enabled != *spec.expected.health.ecc_mode_enabled) {
      reasons.push_back(make_reason(ReasonCode::ECC_MODE_MISMATCH, "health.ecc_mode_enabled",
                                    *spec.expected.health.ecc_mode_enabled,
                                    *health.ecc_mode_enabled));
    }

    if (spec.expected.health.max_volatile_uncorrectable_ecc.has_value() &&
        health.volatile_uncorrectable_ecc.has_value() &&
        *health.volatile_uncorrectable_ecc > *spec.expected.health.max_volatile_uncorrectable_ecc) {
      reasons.push_back(make_reason(ReasonCode::ECC_UNCORRECTABLE_DETECTED,
                                    "health.volatile_uncorrectable_ecc",
                                    *spec.expected.health.max_volatile_uncorrectable_ecc,
                                    *health.volatile_uncorrectable_ecc));
    }

    if (spec.expected.health.max_aggregate_uncorrectable_ecc.has_value() &&
        health.aggregate_uncorrectable_ecc.has_value() &&
        *health.aggregate_uncorrectable_ecc >
            *spec.expected.health.max_aggregate_uncorrectable_ecc) {
      reasons.push_back(make_reason(ReasonCode::ECC_UNCORRECTABLE_DETECTED,
                                    "health.aggregate_uncorrectable_ecc",
                                    *spec.expected.health.max_aggregate_uncorrectable_ecc,
                                    *health.aggregate_uncorrectable_ecc));
    }

    if (spec.expected.health.allow_row_remap_pending.has_value() &&
        !*spec.expected.health.allow_row_remap_pending &&
        health.row_remap_pending.value_or(false)) {
      reasons.push_back(
          make_reason(ReasonCode::ROW_REMAP_PENDING, "health.row_remap_pending", false, true));
    }

    if (spec.expected.health.allow_row_remap_failure.has_value() &&
        !*spec.expected.health.allow_row_remap_failure &&
        health.row_remap_failure.value_or(false)) {
      reasons.push_back(
          make_reason(ReasonCode::ROW_REMAP_FAILURE, "health.row_remap_failure", false, true));
    }

    if (spec.expected.health.allow_pending_retired_pages.has_value() &&
        !*spec.expected.health.allow_pending_retired_pages &&
        health.pending_retired_pages.value_or(false)) {
      reasons.push_back(make_reason(ReasonCode::RETIRED_PAGES_PENDING,
                                    "health.pending_retired_pages", false, true));
    }

    if (!spec.expected.health.disallowed_recovery_actions.empty() &&
        health.recovery_action.has_value() &&
        std::find(spec.expected.health.disallowed_recovery_actions.begin(),
                  spec.expected.health.disallowed_recovery_actions.end(),
                  *health.recovery_action) !=
            spec.expected.health.disallowed_recovery_actions.end()) {
      std::vector<std::string> expected;
      for (RecoveryAction action : spec.expected.health.disallowed_recovery_actions) {
        expected.push_back(std::string(to_string(action)));
      }

      reasons.push_back(make_reason(ReasonCode::GPU_RECOVERY_ACTION_REQUIRED,
                                    "health.recovery_action", expected,
                                    std::string(to_string(*health.recovery_action))));
    }
  }
}

void check_fabric(std::vector<Reason>& reasons, const ExpectedSpec& spec,
                  const ObservedState& observed) {
  if (!spec.expected.fabric.require_fabric_ready.value_or(false)) {
    return;
  }

  if (!observed.fabric.has_value()) {
    reasons.push_back(make_reason(ReasonCode::FIELD_UNSUPPORTED, "fabric.ready", true, nullptr));
    return;
  }

  const FabricState& fabric = *observed.fabric;
  if (!fabric.applicable) {
    reasons.push_back(
        make_reason(ReasonCode::FABRIC_NOT_APPLICABLE, "fabric.applicable", true, false));
    return;
  }

  if (!fabric.ready.value_or(false)) {
    reasons.push_back(make_reason(ReasonCode::FABRIC_NOT_READY, "fabric.ready", true,
                                  fabric.ready.value_or(false)));
  }
}
} // namespace

std::vector<Reason> reconcile(const ExpectedSpec& spec, const ObservedState& observed) {
  std::vector<Reason> reasons;

  check_identity(reasons, spec, observed);
  check_driver(reasons, spec, observed);
  check_cuda(reasons, spec, observed);
  check_health(reasons, spec, observed);
  check_fabric(reasons, spec, observed);

  return reasons;
}

ReasonClass to_reason_class(Severity sev) {
  switch (sev) {
  case Severity::HARD: return ReasonClass::HARD;
  case Severity::WARN: return ReasonClass::WARN;
  case Severity::REPORT: return ReasonClass::REPORT;
  }
}

std::vector<unsigned> parse_version(std::string_view version) {
  std::vector<unsigned> parts;

  for (auto part : version | std::views::split('.')) {
    unsigned value = 0;

    auto begin = part.begin();

    const char* first = &*begin;
    const char* last = first + std::ranges::distance(part);

    auto [ptr, ec] = std::from_chars(first, last, value);

    if (ec != std::errc{} || ptr != last) {
      throw std::invalid_argument("Invalid version component");
    }

    parts.push_back(value);
  }

  return parts;
}

std::strong_ordering compare_driver_versions(std::string_view observed, std::string_view min_req) {
  auto left = parse_version(observed);
  auto right = parse_version(min_req);

  const auto n = std::max(left.size(), right.size());

  for (size_t i{0}; i < n; ++i) {
    unsigned l = i < left.size() ? left[i] : 0;
    unsigned r = i < right.size() ? right[i] : 0;

    if (l < r)
      return std::strong_ordering::less;
    if (l > r)
      return std::strong_ordering::greater;
  }

  return std::strong_ordering::equal;
}
} // namespace gpu_qual
