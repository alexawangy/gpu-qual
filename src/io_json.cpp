#include "gpu_qual/observed.hpp"
#include "gpu_qual/verdict.hpp"
#include <gpu_qual/io_json.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace {
  template <typename T>
  json optional_or_null(const std::optional<T> &value) {
    if (value.has_value()) return *value;
    return nullptr;
  }
}

json gpu_qual::to_json(const NvmlState &nvml) {
  json jres = {
    {"init_ok", nvml.init_ok},
    {"available", nvml.available},
    {"driver_version", optional_or_null(nvml.driver_version)},
  };

  return jres;
}

json gpu_qual::to_json(const CudaState &cuda) {
  json jres = {
    {"smoke_ran", cuda.smoke_ran},
    {"available", optional_or_null(cuda.available)},
    {"visible_device_count", optional_or_null(cuda.device_count)},
    {"smoke_passed", optional_or_null(cuda.smoke_passed)},
  };

  return jres;
}

json gpu_qual::to_json(const GpuInfo &gpu) {
  json jres = {
    {"index", gpu.index},
    {"name", gpu.name},
    {"uuid", gpu.uuid},
    {"pci_bdf", gpu.pci_bdf},
    {"memory_mib", gpu.memory_mib},
    {"mig_mode", gpu_qual::to_string(gpu.mig_mode)},
  };

  return jres;
}

json gpu_qual::to_json(const FallbackSignals &fallback) {
  json jres = {
    {"nvidia_device_nodes_present", fallback.nvidia_device_nodes_present},
    {"nvidia_proc_driver_present", fallback.nvidia_proc_driver_present},
    {"nvidia_pci_devices_present", fallback.nvidia_pci_devices_present},
  };

  return jres;
}

json gpu_qual::to_json(const Result &res) {
   json jres = {
     {"tool_version", res.tool_version},
     {"schema_version", res.schema_version},
     {"mode", gpu_qual::to_string(res.mode)},
     {"exit_code", static_cast<int>(res.exit_code)},
     {"verdict", gpu_qual::to_string(res.verdict)},
   };

   if (res.observed.has_value()) jres["observed"] = to_json(*res.observed);

   jres["reasons"] = json::array();

   for (const Reason &reason : res.reasons) {
     json jr = {
       {"code", gpu_qual::to_string(reason.code)},
       {"class", gpu_qual::to_string(reason.cls)}
     };

     if (!reason.field.empty()) jr["field"] = reason.field;
     if (!reason.expected.empty()) jr["expected"] = reason.expected;
     if (!reason.observed.empty()) jr["observed"] = reason.observed;

     jres["reasons"].push_back(jr);
   }

   return jres;
}

json gpu_qual::to_json(const ObservedState &observed) {
  json jres = {
    {"nvml", to_json(observed.nvml)},
    {"cuda", to_json(observed.cuda)},
    {"fallback", to_json(observed.fallback)},
    {"gpu_count", observed.gpus.size()},
  };

  jres["gpus"] = json::array();

  for (const GpuInfo &gpu : observed.gpus) {
    jres["gpus"].push_back(to_json(gpu));
  }

  return jres;
}
