#include "nvml_api.hpp"

#include <gpu_qual/probe.hpp>

#include <array>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#if defined(__APPLE__) || defined(__linux__) || defined(__unix__)
#include <dlfcn.h>
#define GPUQUAL_HAS_DLOPEN 1
#else
#define GPUQUAL_HAS_DLOPEN 0
#endif

namespace gpu_qual {
namespace {

constexpr std::size_t kDriverVersionBufferSize = 80;
constexpr std::size_t kDeviceNameBufferSize = 96;
constexpr std::size_t kDeviceUuidBufferSize = 96;
constexpr unsigned long long kBytesPerMib = 1024ULL * 1024ULL;

std::string_view nvml_error_name(detail::NvmlReturn error) noexcept {
  switch (error) {
  case 0: return "NVML_SUCCESS";
  case 1: return "NVML_ERROR_UNINITIALIZED";
  case 2: return "NVML_ERROR_INVALID_ARGUMENT";
  case 3: return "NVML_ERROR_NOT_SUPPORTED";
  case 4: return "NVML_ERROR_NO_PERMISSION";
  case 5: return "NVML_ERROR_ALREADY_INITIALIZED";
  case 6: return "NVML_ERROR_NOT_FOUND";
  case 7: return "NVML_ERROR_INSUFFICIENT_SIZE";
  case 8: return "NVML_ERROR_INSUFFICIENT_POWER";
  case 9: return "NVML_ERROR_DRIVER_NOT_LOADED";
  case 10: return "NVML_ERROR_TIMEOUT";
  case 11: return "NVML_ERROR_IRQ_ISSUE";
  case 12: return "NVML_ERROR_LIBRARY_NOT_FOUND";
  case 13: return "NVML_ERROR_FUNCTION_NOT_FOUND";
  case 14: return "NVML_ERROR_CORRUPTED_INFOROM";
  case 15: return "NVML_ERROR_GPU_IS_LOST";
  case 16: return "NVML_ERROR_RESET_REQUIRED";
  case 17: return "NVML_ERROR_OPERATING_SYSTEM";
  case 18: return "NVML_ERROR_LIB_RM_VERSION_MISMATCH";
  case 19: return "NVML_ERROR_IN_USE";
  case 20: return "NVML_ERROR_MEMORY";
  case 21: return "NVML_ERROR_NO_DATA";
  case 22: return "NVML_ERROR_VGPU_ECC_NOT_SUPPORTED";
  case 23: return "NVML_ERROR_INSUFFICIENT_RESOURCES";
  case 24: return "NVML_ERROR_FREQ_NOT_SUPPORTED";
  case 25: return "NVML_ERROR_ARGUMENT_VERSION_MISMATCH";
  case 26: return "NVML_ERROR_DEPRECATED";
  case 27: return "NVML_ERROR_NOT_READY";
  case 28: return "NVML_ERROR_GPU_NOT_FOUND";
  case 29: return "NVML_ERROR_INVALID_STATE";
  case 30: return "NVML_ERROR_RESET_TYPE_NOT_SUPPORTED";
  case 999: return "NVML_ERROR_UNKNOWN";
  default: return "NVML_ERROR_UNRECOGNIZED";
  }
}

json nvml_error_json(const detail::NvmlApi& api, detail::NvmlReturn error) {
  json value = {
      {"code", error},
      {"name", nvml_error_name(error)},
  };
  if (api.error_string != nullptr) {
    if (const char* message = api.error_string(error); message != nullptr && message[0] != '\0') {
      value["message"] = message;
    }
  }
  return value;
}

std::optional<std::string> read_nonempty_string(const char* buffer, std::size_t size) {
  const void* terminator = std::memchr(buffer, '\0', size);
  if (terminator == nullptr || terminator == buffer) {
    return std::nullopt;
  }

  const auto* end = static_cast<const char*>(terminator);
  return std::string(buffer, static_cast<std::size_t>(end - buffer));
}

bool access_is_denied(detail::NvmlReturn error) noexcept {
  return error == detail::kNvmlErrorNoPermission || error == detail::kNvmlErrorOperatingSystem;
}

ProbeOutcome query_failure(ProbeOutcome outcome, const detail::NvmlApi& api, std::string field,
                           detail::NvmlReturn error) {
  if (access_is_denied(error)) {
    outcome.observed.nvml.status = NvmlStatus::NO_PERMISSION;
    outcome.reasons.push_back(make_reason(ReasonCode::NVML_NO_PERMISSION, std::move(field),
                                          json("NVML_SUCCESS"), nvml_error_json(api, error)));
    return outcome;
  }

  outcome.reasons.push_back(make_reason(ReasonCode::PROBE_OUTPUT_INVALID, std::move(field),
                                        json("NVML_SUCCESS"), nvml_error_json(api, error)));
  return outcome;
}

ProbeOutcome invalid_query_output(ProbeOutcome outcome, std::string field, std::string observed) {
  outcome.reasons.push_back(make_reason(ReasonCode::PROBE_OUTPUT_INVALID, std::move(field),
                                        json("non-empty NUL-terminated value"),
                                        json(std::move(observed))));
  return outcome;
}

struct ShutdownGuard {
  const detail::NvmlApi& api;

  ~ShutdownGuard() {
    static_cast<void>(api.shutdown());
  }
};

#if GPUQUAL_HAS_DLOPEN
class SharedLibrary {
public:
  explicit SharedLibrary(const char* name) : handle_(dlopen(name, RTLD_NOW | RTLD_LOCAL)) {
    if (handle_ == nullptr) {
      if (const char* error = dlerror(); error != nullptr) {
        error_ = error;
      }
    }
  }

  SharedLibrary(const SharedLibrary&) = delete;
  SharedLibrary& operator=(const SharedLibrary&) = delete;

  ~SharedLibrary() {
    if (handle_ != nullptr) {
      static_cast<void>(dlclose(handle_));
    }
  }

  [[nodiscard]] bool loaded() const noexcept {
    return handle_ != nullptr;
  }

  [[nodiscard]] const std::string& error() const noexcept {
    return error_;
  }

  template <typename Function> [[nodiscard]] Function symbol(const char* name) const noexcept {
    static_assert(std::is_pointer_v<Function>);
    void* address = dlsym(handle_, name);
    Function function = nullptr;
    static_assert(sizeof(function) == sizeof(address));
    std::memcpy(&function, &address, sizeof(function));
    return function;
  }

private:
  void* handle_ = nullptr;
  std::string error_;
};

std::optional<std::string> load_function_table(const SharedLibrary& library, detail::NvmlApi& api) {
  std::optional<std::string> missing;
  const auto load_required = [&](auto& target, const char* name) {
    using Function = std::remove_reference_t<decltype(target)>;
    target = library.symbol<Function>(name);
    if (target == nullptr && !missing.has_value()) {
      missing = name;
    }
  };

  load_required(api.init_v2, "nvmlInit_v2");
  api.init_with_flags = library.symbol<detail::NvmlApi::InitWithFlags>("nvmlInitWithFlags");
  load_required(api.shutdown, "nvmlShutdown");
  api.error_string = library.symbol<detail::NvmlApi::ErrorString>("nvmlErrorString");
  load_required(api.system_get_driver_version, "nvmlSystemGetDriverVersion");
  load_required(api.device_get_count_v2, "nvmlDeviceGetCount_v2");
  load_required(api.device_get_handle_by_index_v2, "nvmlDeviceGetHandleByIndex_v2");
  load_required(api.device_get_name, "nvmlDeviceGetName");
  load_required(api.device_get_uuid, "nvmlDeviceGetUUID");

  api.device_get_pci_info =
      library.symbol<detail::NvmlApi::DeviceGetPciInfo>("nvmlDeviceGetPciInfo_v3");
  if (api.device_get_pci_info == nullptr) {
    api.device_get_pci_info =
        library.symbol<detail::NvmlApi::DeviceGetPciInfo>("nvmlDeviceGetPciInfo_v2");
  }
  if (api.device_get_pci_info == nullptr && !missing.has_value()) {
    missing = "nvmlDeviceGetPciInfo_v3 or nvmlDeviceGetPciInfo_v2";
  }

  load_required(api.device_get_memory_info, "nvmlDeviceGetMemoryInfo");
  return missing;
}
#endif

} // namespace

namespace detail {

ProbeOutcome collect_nvml(const NvmlApi& api) {
  ProbeOutcome outcome{};
  if (!api.complete()) {
    outcome.observed.nvml.status = NvmlStatus::INITIALIZATION_FAILED;
    outcome.reasons.push_back(make_reason(ReasonCode::NVML_INIT_FAILED, "nvml.function_table",
                                          json("all required functions"), json("incomplete")));
    return outcome;
  }

  const NvmlReturn init_result =
      api.init_with_flags != nullptr ? api.init_with_flags(kNvmlInitFlagNoGpus) : api.init_v2();
  if (init_result != kNvmlSuccess) {
    outcome.observed.nvml.status = access_is_denied(init_result)
                                       ? NvmlStatus::NO_PERMISSION
                                       : NvmlStatus::INITIALIZATION_FAILED;
    const ReasonCode code = access_is_denied(init_result) ? ReasonCode::NVML_NO_PERMISSION
                                                          : ReasonCode::NVML_INIT_FAILED;
    outcome.reasons.push_back(
        make_reason(code, "nvml.init", json("NVML_SUCCESS"), nvml_error_json(api, init_result)));
    return outcome;
  }

  const ShutdownGuard shutdown_guard{api};
  outcome.observed.nvml.status = NvmlStatus::READY;

  std::array<char, kDriverVersionBufferSize> driver_version{};
  NvmlReturn query_result = api.system_get_driver_version(
      driver_version.data(), static_cast<unsigned>(driver_version.size()));
  if (query_result != kNvmlSuccess) {
    return query_failure(std::move(outcome), api, "nvml.driver_version", query_result);
  }
  auto parsed_driver_version = read_nonempty_string(driver_version.data(), driver_version.size());
  if (!parsed_driver_version.has_value()) {
    return invalid_query_output(std::move(outcome), "nvml.driver_version",
                                "empty or unterminated string");
  }
  outcome.observed.nvml.driver_version = std::move(*parsed_driver_version);

  unsigned device_count = 0;
  query_result = api.device_get_count_v2(&device_count);
  if (query_result != kNvmlSuccess) {
    return query_failure(std::move(outcome), api, "nvml.device_count", query_result);
  }

  outcome.observed.gpus.reserve(device_count);
  for (unsigned index = 0; index < device_count; ++index) {
    const std::string prefix = "gpus[" + std::to_string(index) + "].";
    NvmlDevice device = nullptr;
    query_result = api.device_get_handle_by_index_v2(index, &device);
    if (query_result != kNvmlSuccess) {
      return query_failure(std::move(outcome), api, prefix + "handle", query_result);
    }

    std::array<char, kDeviceNameBufferSize> name{};
    query_result = api.device_get_name(device, name.data(), static_cast<unsigned>(name.size()));
    if (query_result != kNvmlSuccess) {
      return query_failure(std::move(outcome), api, prefix + "name", query_result);
    }
    auto parsed_name = read_nonempty_string(name.data(), name.size());
    if (!parsed_name.has_value()) {
      return invalid_query_output(std::move(outcome), prefix + "name",
                                  "empty or unterminated string");
    }

    std::array<char, kDeviceUuidBufferSize> uuid{};
    query_result = api.device_get_uuid(device, uuid.data(), static_cast<unsigned>(uuid.size()));
    if (query_result != kNvmlSuccess) {
      return query_failure(std::move(outcome), api, prefix + "uuid", query_result);
    }
    auto parsed_uuid = read_nonempty_string(uuid.data(), uuid.size());
    if (!parsed_uuid.has_value()) {
      return invalid_query_output(std::move(outcome), prefix + "uuid",
                                  "empty or unterminated string");
    }

    NvmlPciInfo pci{};
    query_result = api.device_get_pci_info(device, &pci);
    if (query_result != kNvmlSuccess) {
      return query_failure(std::move(outcome), api, prefix + "pci_bdf", query_result);
    }
    auto parsed_pci_bdf = read_nonempty_string(pci.bus_id, sizeof(pci.bus_id));
    if (!parsed_pci_bdf.has_value()) {
      return invalid_query_output(std::move(outcome), prefix + "pci_bdf",
                                  "empty or unterminated string");
    }

    NvmlMemory memory{};
    query_result = api.device_get_memory_info(device, &memory);
    if (query_result != kNvmlSuccess) {
      return query_failure(std::move(outcome), api, prefix + "memory_mib", query_result);
    }
    const unsigned long long memory_mib = memory.total / kBytesPerMib;
    if (memory_mib == 0) {
      outcome.reasons.push_back(make_reason(ReasonCode::PROBE_OUTPUT_INVALID, prefix + "memory_mib",
                                            json("positive integer"), json(memory_mib)));
      return outcome;
    }

    outcome.observed.gpus.push_back({
        .index = index,
        .name = std::move(*parsed_name),
        .uuid = std::move(*parsed_uuid),
        .pci_bdf = std::move(*parsed_pci_bdf),
        .memory_mib = memory_mib,
    });
  }

  return outcome;
}

} // namespace detail

ProbeOutcome probe_nvml() {
#if GPUQUAL_HAS_DLOPEN
  constexpr const char* kNvmlLibrary = "libnvidia-ml.so.1";
  const SharedLibrary library(kNvmlLibrary);
  if (!library.loaded()) {
    ProbeOutcome outcome{};
    outcome.observed.nvml.status = NvmlStatus::LIBRARY_NOT_FOUND;
    outcome.reasons.push_back(make_reason(ReasonCode::NVML_LIBRARY_NOT_FOUND, "nvml.library",
                                          json(kNvmlLibrary), json(library.error())));
    return outcome;
  }

  detail::NvmlApi api{};
  if (const auto missing = load_function_table(library, api); missing.has_value()) {
    ProbeOutcome outcome{};
    outcome.observed.nvml.status = NvmlStatus::INITIALIZATION_FAILED;
    outcome.reasons.push_back(make_reason(ReasonCode::NVML_INIT_FAILED, "nvml.symbol",
                                          json("required NVML symbol"), json(*missing)));
    return outcome;
  }

  return detail::collect_nvml(api);
#else
  ProbeOutcome outcome{};
  outcome.observed.nvml.status = NvmlStatus::LIBRARY_NOT_FOUND;
  outcome.reasons.push_back(make_reason(ReasonCode::NVML_LIBRARY_NOT_FOUND, "nvml.library",
                                        json("libnvidia-ml.so.1"),
                                        json("dynamic loading is unsupported on this platform")));
  return outcome;
#endif
}

ProbeOutcome probe_simulated_nvml() {
  ProbeOutcome outcome{};
  outcome.observed.nvml = {
      .status = NvmlStatus::READY,
      .driver_version = "550.90.07",
  };
  outcome.observed.gpus.push_back({
      .index = 0,
      .name = "SIMULATED NVIDIA H100 80GB HBM3",
      .uuid = "GPU-SIMULATED-00000000-0000-0000-0000-000000000001",
      .pci_bdf = "00000000:01:00.0",
      .memory_mib = 81920,
  });
  return outcome;
}

} // namespace gpu_qual

#undef GPUQUAL_HAS_DLOPEN
