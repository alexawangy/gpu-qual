#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace gpu_qual {
enum class NvmlStatus {
  NOT_PROBED,
  LIBRARY_NOT_FOUND,
  INITIALIZATION_FAILED,
  NO_PERMISSION,
  READY,
};

std::string_view to_string(NvmlStatus);

struct NvmlState {
  NvmlStatus status = NvmlStatus::NOT_PROBED;
  std::optional<std::string> driver_version;
};

struct GpuInfo {
  unsigned index = 0;
  std::string name;
  std::string uuid;
  std::string pci_bdf;
  std::uint64_t memory_mib = 0;
};

struct ObservedState {
  NvmlState nvml;
  std::vector<GpuInfo> gpus;
};
} // namespace gpu_qual
