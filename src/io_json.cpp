#include <gpu_qual/io_json.hpp>

#include <optional>
#include <utility>

namespace gpu_qual {
namespace {
template <typename T> json optional_or_null(const std::optional<T>& value) {
  return value.has_value() ? json(*value) : json(nullptr);
}

json nvml_to_json(const NvmlState& nvml) {
  return {
      {"status", to_string(nvml.status)},
      {"driver_version", optional_or_null(nvml.driver_version)},
  };
}

json gpu_to_json(const GpuInfo& gpu) {
  return {
      {"index", gpu.index},
      {"name", gpu.name},
      {"uuid", gpu.uuid},
      {"pci_bdf", gpu.pci_bdf},
      {"memory_mib", gpu.memory_mib},
  };
}
} // namespace

json to_json(const Result& result) {
  json output = {
      {"tool_version", result.tool_version},
      {"schema_version", result.schema_version},
      {"mode", to_string(result.mode)},
      {"verdict", to_string(result.verdict)},
      {"exit_code", static_cast<int>(result.exit_code)},
      {"reasons", json::array()},
  };

  if (result.observed.has_value()) {
    output["observed"] = to_json(*result.observed);
  }

  for (const Reason& reason : result.reasons) {
    json serialized_reason = {
        {"code", to_string(reason.code)},
        {"class", to_string(default_class(reason.code))},
    };

    if (!reason.field.empty()) {
      serialized_reason["field"] = reason.field;
    }
    if (reason.expected.has_value()) {
      serialized_reason["expected"] = *reason.expected;
    }
    if (reason.observed.has_value()) {
      serialized_reason["observed"] = *reason.observed;
    }

    output["reasons"].push_back(std::move(serialized_reason));
  }

  return output;
}

json to_json(const ObservedState& observed) {
  json gpus = json::array();
  for (const GpuInfo& gpu : observed.gpus) {
    gpus.push_back(gpu_to_json(gpu));
  }

  return {
      {"nvml", nvml_to_json(observed.nvml)},
      {"gpu_count", observed.gpus.size()},
      {"gpus", std::move(gpus)},
  };
}
} // namespace gpu_qual
