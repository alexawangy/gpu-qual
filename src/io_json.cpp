#include "gpu_qual/observed.hpp"
#include "gpu_qual/verdict.hpp"
#include <gpu_qual/io_json.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

json gpu_qual::to_json(const NvmlState &nvml) {
  json jres = {
    {"init_ok", nvml.init_ok},
    {"available", nvml.available},
  };

  if (nvml.driver_version.has_value()) jres["driver_version"] = *nvml.driver_version;

  return jres;
}

json gpu_qual::to_json(const CudaState &cuda) {
  json jres = {
    {"smoke_ran", cuda.smoke_ran},
  };

  if (cuda.available.has_value()) jres["available"] = *cuda.available;
  if (cuda.device_count.has_value()) jres["device_count"] = *cuda.device_count;
  if (cuda.smoke_passed.has_value()) jres["smoke_passed"] = *cuda.smoke_passed;

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
     {"verdict", gpu_qual::to_string(res.verdict)}
   };

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
