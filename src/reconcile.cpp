#include <gpu_qual/reconcile.hpp>
#include <gpu_qual/version.hpp>

#include <algorithm>
#include <compare>
#include <vector>

namespace gpu_qual {
namespace {
void check_gpu_count(std::vector<Reason>& reasons, const ExpectedSpec& spec,
                     const ObservedState& observed) {
  if (!spec.gpu_count.has_value() || *spec.gpu_count == observed.gpus.size()) {
    return;
  }

  reasons.push_back(make_reason(ReasonCode::GPU_COUNT_MISMATCH, "gpu_count", *spec.gpu_count,
                                observed.gpus.size()));
}

void check_gpus(std::vector<Reason>& reasons, const ExpectedSpec& spec,
                const ObservedState& observed) {
  for (const GpuInfo& gpu : observed.gpus) {
    if (!spec.allowed_names.empty() &&
        std::find(spec.allowed_names.begin(), spec.allowed_names.end(), gpu.name) ==
            spec.allowed_names.end()) {
      reasons.push_back(
          make_reason(ReasonCode::GPU_NAME_MISMATCH, "gpu_name", spec.allowed_names, gpu.name));
    }

    if (spec.min_memory_mib.has_value() && gpu.memory_mib < *spec.min_memory_mib) {
      reasons.push_back(make_reason(ReasonCode::GPU_MEMORY_BELOW_MIN, "gpu_memory",
                                    *spec.min_memory_mib, gpu.memory_mib));
    }
  }
}

void check_driver_version(std::vector<Reason>& reasons, const ExpectedSpec& spec,
                          const ObservedState& observed) {
  if (!spec.min_driver_version.has_value()) {
    return;
  }

  if (!is_valid_driver_version(*spec.min_driver_version)) {
    reasons.push_back(make_reason(ReasonCode::EXPECTED_SPEC_INVALID, "expected.min_driver_version",
                                  std::nullopt, json(*spec.min_driver_version)));
    return;
  }

  if (!observed.nvml.driver_version.has_value() ||
      !is_valid_driver_version(*observed.nvml.driver_version)) {
    const json observed_driver = observed.nvml.driver_version.has_value()
                                     ? json(*observed.nvml.driver_version)
                                     : json(nullptr);
    reasons.push_back(make_reason(ReasonCode::PROBE_OUTPUT_INVALID, "nvml.driver_version",
                                  json("dot-separated decimal components"), observed_driver));
    return;
  }

  const auto comparison =
      compare_driver_versions(*observed.nvml.driver_version, *spec.min_driver_version);
  if (comparison.has_value() && *comparison == std::strong_ordering::less) {
    reasons.push_back(make_reason(ReasonCode::DRIVER_VERSION_BELOW_MIN, "driver_version",
                                  *spec.min_driver_version, *observed.nvml.driver_version));
  }
}
} // namespace

std::vector<Reason> reconcile(const ExpectedSpec& spec, const ObservedState& observed) {
  std::vector<Reason> reasons;
  if (observed.nvml.status != NvmlStatus::READY) {
    return reasons;
  }

  check_gpu_count(reasons, spec, observed);
  check_gpus(reasons, spec, observed);
  check_driver_version(reasons, spec, observed);
  return reasons;
}

} // namespace gpu_qual
