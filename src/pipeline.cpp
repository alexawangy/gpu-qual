#include <gpu_qual/pipeline.hpp>
#include <gpu_qual/reconcile.hpp>
#include <gpu_qual/version.hpp>

#include <algorithm>
#include <iterator>
#include <utility>

namespace gpu_qual {
namespace {

bool contains_reason(const std::vector<Reason>& reasons, ReasonCode code) {
  return std::any_of(reasons.begin(), reasons.end(),
                     [code](const Reason& reason) { return reason.code == code; });
}

void append_once(std::vector<Reason>& reasons, Reason reason) {
  if (!contains_reason(reasons, reason.code)) {
    reasons.push_back(std::move(reason));
  }
}

void add_status_reasons(ProbeOutcome& outcome) {
  switch (outcome.observed.nvml.status) {
  case NvmlStatus::NOT_PROBED:
    append_once(outcome.reasons, make_reason(ReasonCode::PROBE_OUTPUT_INVALID, "nvml.status",
                                             json("ready"), json("not_probed")));
    return;
  case NvmlStatus::LIBRARY_NOT_FOUND:
    append_once(outcome.reasons, make_reason(ReasonCode::NVML_LIBRARY_NOT_FOUND));
    return;
  case NvmlStatus::INITIALIZATION_FAILED:
    append_once(outcome.reasons, make_reason(ReasonCode::NVML_INIT_FAILED));
    return;
  case NvmlStatus::NO_PERMISSION:
    append_once(outcome.reasons, make_reason(ReasonCode::NVML_NO_PERMISSION));
    return;
  case NvmlStatus::READY: break;
  }

  if (!outcome.observed.nvml.driver_version.has_value() ||
      !is_valid_driver_version(*outcome.observed.nvml.driver_version)) {
    const json observed_driver = outcome.observed.nvml.driver_version.has_value()
                                     ? json(*outcome.observed.nvml.driver_version)
                                     : json(nullptr);
    append_once(outcome.reasons,
                make_reason(ReasonCode::PROBE_OUTPUT_INVALID, "nvml.driver_version",
                            json("dot-separated decimal components"), observed_driver));
  }

  if (outcome.observed.gpus.empty()) {
    append_once(outcome.reasons, make_reason(ReasonCode::NO_NVIDIA_DEVICES));
    return;
  }

  for (const GpuInfo& gpu : outcome.observed.gpus) {
    const std::string prefix = "gpus[" + std::to_string(gpu.index) + "].";
    if (gpu.name.empty()) {
      append_once(outcome.reasons, make_reason(ReasonCode::PROBE_OUTPUT_INVALID, prefix + "name",
                                               json("non-empty string"), json(gpu.name)));
      return;
    }
    if (gpu.uuid.empty()) {
      append_once(outcome.reasons, make_reason(ReasonCode::PROBE_OUTPUT_INVALID, prefix + "uuid",
                                               json("non-empty string"), json(gpu.uuid)));
      return;
    }
    if (gpu.pci_bdf.empty()) {
      append_once(outcome.reasons, make_reason(ReasonCode::PROBE_OUTPUT_INVALID, prefix + "pci_bdf",
                                               json("non-empty string"), json(gpu.pci_bdf)));
      return;
    }
    if (gpu.memory_mib == 0) {
      append_once(outcome.reasons,
                  make_reason(ReasonCode::PROBE_OUTPUT_INVALID, prefix + "memory_mib",
                              json("positive integer"), json(gpu.memory_mib)));
      return;
    }
  }
}

bool observation_is_usable(const ProbeOutcome& outcome) {
  if (outcome.observed.nvml.status != NvmlStatus::READY ||
      !outcome.observed.nvml.driver_version.has_value() || outcome.observed.gpus.empty()) {
    return false;
  }

  return std::none_of(outcome.reasons.begin(), outcome.reasons.end(), [](const Reason& reason) {
    return default_exit_code(reason.code) == ExitCode::FAIL_STACK;
  });
}

} // namespace

Result evaluate_inventory(ProbeOutcome outcome) {
  add_status_reasons(outcome);
  Result result = compute_result(Mode::INVENTORY, std::move(outcome.reasons));
  result.observed = std::move(outcome.observed);
  return result;
}

Result evaluate_check(ProbeOutcome outcome, const ExpectedSpec& spec) {
  add_status_reasons(outcome);
  if (observation_is_usable(outcome)) {
    std::vector<Reason> reconciliation_reasons = reconcile(spec, outcome.observed);
    outcome.reasons.insert(outcome.reasons.end(),
                           std::make_move_iterator(reconciliation_reasons.begin()),
                           std::make_move_iterator(reconciliation_reasons.end()));
  }

  Result result = compute_result(Mode::CHECK, std::move(outcome.reasons));
  result.observed = std::move(outcome.observed);
  return result;
}

} // namespace gpu_qual
