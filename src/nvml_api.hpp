#pragma once

#include <gpu_qual/probe.hpp>

#include <cstddef>

namespace gpu_qual::detail {

// Minimal, header-independent subset of the NVML C ABI. Runtime nodes need
// only the driver library; nvml.h is deliberately not a build dependency.
using NvmlReturn = int;

inline constexpr NvmlReturn kNvmlSuccess = 0;
inline constexpr NvmlReturn kNvmlErrorInvalidArgument = 2;
inline constexpr NvmlReturn kNvmlErrorNoPermission = 4;
inline constexpr NvmlReturn kNvmlErrorInsufficientSize = 7;
inline constexpr NvmlReturn kNvmlErrorOperatingSystem = 17;
inline constexpr unsigned kNvmlInitFlagNoGpus = 1U;

struct NvmlDeviceHandle;
using NvmlDevice = NvmlDeviceHandle*;

struct NvmlMemory {
  unsigned long long total;
  unsigned long long free;
  unsigned long long used;
};

struct NvmlPciInfo {
  char bus_id_legacy[16];
  unsigned domain;
  unsigned bus;
  unsigned device;
  unsigned pci_device_id;
  unsigned pci_subsystem_id;
  char bus_id[32];
};

static_assert(sizeof(unsigned) == 4);
static_assert(sizeof(unsigned long long) == 8);
static_assert(sizeof(NvmlPciInfo) == 68);
static_assert(offsetof(NvmlPciInfo, bus_id) == 36);

struct NvmlApi {
  using Init = NvmlReturn (*)();
  using InitWithFlags = NvmlReturn (*)(unsigned);
  using Shutdown = NvmlReturn (*)();
  using ErrorString = const char* (*)(NvmlReturn);
  using SystemGetDriverVersion = NvmlReturn (*)(char*, unsigned);
  using DeviceGetCount = NvmlReturn (*)(unsigned*);
  using DeviceGetHandleByIndex = NvmlReturn (*)(unsigned, NvmlDevice*);
  using DeviceGetString = NvmlReturn (*)(NvmlDevice, char*, unsigned);
  using DeviceGetPciInfo = NvmlReturn (*)(NvmlDevice, NvmlPciInfo*);
  using DeviceGetMemoryInfo = NvmlReturn (*)(NvmlDevice, NvmlMemory*);

  Init init_v2 = nullptr;
  InitWithFlags init_with_flags = nullptr; // Optional; permits a clean zero-GPU result.
  Shutdown shutdown = nullptr;
  ErrorString error_string = nullptr; // Optional; numeric/name diagnostics remain available.
  SystemGetDriverVersion system_get_driver_version = nullptr;
  DeviceGetCount device_get_count_v2 = nullptr;
  DeviceGetHandleByIndex device_get_handle_by_index_v2 = nullptr;
  DeviceGetString device_get_name = nullptr;
  DeviceGetString device_get_uuid = nullptr;
  DeviceGetPciInfo device_get_pci_info = nullptr;
  DeviceGetMemoryInfo device_get_memory_info = nullptr;

  [[nodiscard]] bool complete() const noexcept {
    return init_v2 != nullptr && shutdown != nullptr && system_get_driver_version != nullptr &&
           device_get_count_v2 != nullptr && device_get_handle_by_index_v2 != nullptr &&
           device_get_name != nullptr && device_get_uuid != nullptr &&
           device_get_pci_info != nullptr && device_get_memory_info != nullptr;
  }
};

ProbeOutcome collect_nvml(const NvmlApi& api);

} // namespace gpu_qual::detail
