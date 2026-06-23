#pragma once

#include <optional>
#include <string>

#include <string_view>
#include <vector>
namespace gpu_qual {
enum class MigMode { ENABLED, DISABLED };
enum class RecoveryAction { NONE, RESET, RESET_AND_DRAIN, REBOOT, FIELD_RMA };
std::string_view to_string(MigMode);
std::string_view to_string(RecoveryAction);

struct GpuHealth {
  std::optional<bool> ecc_mode_enabled;
  std::optional<long long> volatile_uncorrectable_ecc;
  std::optional<long long> aggregate_uncorrectable_ecc;
  std::optional<bool> row_remap_pending;
  std::optional<bool> row_remap_failure;
  std::optional<bool> pending_retired_pages;
  std::optional<RecoveryAction> recovery_action;
};

struct FabricState {
  bool applicable = false;
  std::optional<bool> ready;
};

struct NvmlState {
  bool init_ok;
  bool available;
  std::optional<std::string> driver_version;
};

struct CudaState {
  std::optional<bool> available;
  std::optional<int> device_count;
  bool smoke_ran = false;
  std::optional<bool> smoke_passed;
};

struct GpuInfo {
  int index = 0;
  std::string name;
  std::string uuid;
  std::string pci_bdf;
  long long memory_mib = 0;
  MigMode mig_mode = MigMode::DISABLED;

  std::optional<GpuHealth> health;
};

struct FallbackSignals {
  bool nvidia_device_nodes_present = false;
  bool nvidia_proc_driver_present = false;
  bool nvidia_pci_devices_present = false;
};

struct ObservedState {
  NvmlState nvml;
  CudaState cuda;
  std::vector<GpuInfo> gpus;
  FallbackSignals fallback;
  std::optional<FabricState> fabric;
};
} // namespace gpu_qual
