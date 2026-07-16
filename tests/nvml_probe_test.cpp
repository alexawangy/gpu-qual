#include "nvml_api.hpp"

#include <gpu_qual/pipeline.hpp>
#include <gpu_qual/probe.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace gpu_qual {
namespace {

struct FakeGpu {
  std::string name;
  std::string uuid;
  std::string pci_bdf;
  unsigned long long memory_bytes;
};

struct FakeState {
  detail::NvmlReturn init_result = detail::kNvmlSuccess;
  detail::NvmlReturn driver_result = detail::kNvmlSuccess;
  detail::NvmlReturn count_result = detail::kNvmlSuccess;
  detail::NvmlReturn handle_result = detail::kNvmlSuccess;
  detail::NvmlReturn name_result = detail::kNvmlSuccess;
  detail::NvmlReturn uuid_result = detail::kNvmlSuccess;
  detail::NvmlReturn pci_result = detail::kNvmlSuccess;
  detail::NvmlReturn memory_result = detail::kNvmlSuccess;
  std::string driver_version = "550.90.07";
  std::vector<FakeGpu> gpus;
  int init_calls = 0;
  int shutdown_calls = 0;
};

FakeState* active_fake = nullptr;

detail::NvmlReturn copy_fake_string(const std::string& value, char* buffer, unsigned length) {
  if (buffer == nullptr) {
    return detail::kNvmlErrorInvalidArgument;
  }
  if (value.size() + 1 > length) {
    return detail::kNvmlErrorInsufficientSize;
  }
  std::memcpy(buffer, value.data(), value.size());
  buffer[value.size()] = '\0';
  return detail::kNvmlSuccess;
}

detail::NvmlDevice fake_device(unsigned index) {
  return reinterpret_cast<detail::NvmlDevice>(static_cast<std::uintptr_t>(index) + 1U);
}

unsigned fake_index(detail::NvmlDevice device) {
  return static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(device) - 1U);
}

detail::NvmlReturn fake_init() {
  ++active_fake->init_calls;
  return active_fake->init_result;
}

detail::NvmlReturn fake_shutdown() {
  ++active_fake->shutdown_calls;
  return detail::kNvmlSuccess;
}

const char* fake_error_string(detail::NvmlReturn) {
  return "fake NVML error";
}

detail::NvmlReturn fake_driver_version(char* version, unsigned length) {
  if (active_fake->driver_result != detail::kNvmlSuccess) {
    return active_fake->driver_result;
  }
  return copy_fake_string(active_fake->driver_version, version, length);
}

detail::NvmlReturn fake_device_count(unsigned* count) {
  if (active_fake->count_result != detail::kNvmlSuccess) {
    return active_fake->count_result;
  }
  if (count == nullptr) {
    return detail::kNvmlErrorInvalidArgument;
  }
  *count = static_cast<unsigned>(active_fake->gpus.size());
  return detail::kNvmlSuccess;
}

detail::NvmlReturn fake_handle_by_index(unsigned index, detail::NvmlDevice* device) {
  if (active_fake->handle_result != detail::kNvmlSuccess) {
    return active_fake->handle_result;
  }
  if (device == nullptr || index >= active_fake->gpus.size()) {
    return detail::kNvmlErrorInvalidArgument;
  }
  *device = fake_device(index);
  return detail::kNvmlSuccess;
}

detail::NvmlReturn fake_name(detail::NvmlDevice device, char* name, unsigned length) {
  if (active_fake->name_result != detail::kNvmlSuccess) {
    return active_fake->name_result;
  }
  return copy_fake_string(active_fake->gpus.at(fake_index(device)).name, name, length);
}

detail::NvmlReturn fake_uuid(detail::NvmlDevice device, char* uuid, unsigned length) {
  if (active_fake->uuid_result != detail::kNvmlSuccess) {
    return active_fake->uuid_result;
  }
  return copy_fake_string(active_fake->gpus.at(fake_index(device)).uuid, uuid, length);
}

detail::NvmlReturn fake_pci_info(detail::NvmlDevice device, detail::NvmlPciInfo* pci) {
  if (active_fake->pci_result != detail::kNvmlSuccess) {
    return active_fake->pci_result;
  }
  if (pci == nullptr) {
    return detail::kNvmlErrorInvalidArgument;
  }
  *pci = {};
  return copy_fake_string(active_fake->gpus.at(fake_index(device)).pci_bdf, pci->bus_id,
                          sizeof(pci->bus_id));
}

detail::NvmlReturn fake_memory_info(detail::NvmlDevice device, detail::NvmlMemory* memory) {
  if (active_fake->memory_result != detail::kNvmlSuccess) {
    return active_fake->memory_result;
  }
  if (memory == nullptr) {
    return detail::kNvmlErrorInvalidArgument;
  }
  *memory = {
      .total = active_fake->gpus.at(fake_index(device)).memory_bytes,
      .free = 0,
      .used = 0,
  };
  return detail::kNvmlSuccess;
}

detail::NvmlApi fake_api(FakeState& state) {
  active_fake = &state;
  return {
      .init_v2 = fake_init,
      .init_with_flags = nullptr,
      .shutdown = fake_shutdown,
      .error_string = fake_error_string,
      .system_get_driver_version = fake_driver_version,
      .device_get_count_v2 = fake_device_count,
      .device_get_handle_by_index_v2 = fake_handle_by_index,
      .device_get_name = fake_name,
      .device_get_uuid = fake_uuid,
      .device_get_pci_info = fake_pci_info,
      .device_get_memory_info = fake_memory_info,
  };
}

FakeState healthy_fake() {
  return {
      .gpus =
          {
              {
                  .name = "NVIDIA H100 80GB HBM3",
                  .uuid = "GPU-one",
                  .pci_bdf = "00000000:1B:00.0",
                  .memory_bytes = 81559ULL * 1024ULL * 1024ULL + 123ULL,
              },
              {
                  .name = "NVIDIA H100 80GB HBM3",
                  .uuid = "GPU-two",
                  .pci_bdf = "00000000:43:00.0",
                  .memory_bytes = 81559ULL * 1024ULL * 1024ULL,
              },
          },
  };
}

TEST_CASE("NVML function table collects complete physical GPU identity", "[nvml]") {
  FakeState fake = healthy_fake();
  const detail::NvmlApi api = fake_api(fake);

  const ProbeOutcome outcome = detail::collect_nvml(api);

  CHECK(outcome.observed.nvml.status == NvmlStatus::READY);
  CHECK(outcome.observed.nvml.driver_version == "550.90.07");
  REQUIRE(outcome.observed.gpus.size() == 2);
  CHECK(outcome.observed.gpus[0].index == 0);
  CHECK(outcome.observed.gpus[0].name == "NVIDIA H100 80GB HBM3");
  CHECK(outcome.observed.gpus[0].uuid == "GPU-one");
  CHECK(outcome.observed.gpus[0].pci_bdf == "00000000:1B:00.0");
  CHECK(outcome.observed.gpus[0].memory_mib == 81559);
  CHECK(outcome.observed.gpus[1].index == 1);
  CHECK(outcome.observed.gpus[1].uuid == "GPU-two");
  CHECK(outcome.reasons.empty());
  CHECK(fake.init_calls == 1);
  CHECK(fake.shutdown_calls == 1);
}

TEST_CASE("NVML initialization failures map status and respect lifecycle", "[nvml]") {
  SECTION("permission failure") {
    FakeState fake = healthy_fake();
    fake.init_result = detail::kNvmlErrorNoPermission;

    const ProbeOutcome outcome = detail::collect_nvml(fake_api(fake));

    CHECK(outcome.observed.nvml.status == NvmlStatus::NO_PERMISSION);
    REQUIRE(outcome.reasons.size() == 1);
    CHECK(outcome.reasons[0].code == ReasonCode::NVML_NO_PERMISSION);
    CHECK(fake.shutdown_calls == 0);
  }

  SECTION("OS or cgroup access block") {
    FakeState fake = healthy_fake();
    fake.init_result = detail::kNvmlErrorOperatingSystem;

    const ProbeOutcome outcome = detail::collect_nvml(fake_api(fake));

    CHECK(outcome.observed.nvml.status == NvmlStatus::NO_PERMISSION);
    REQUIRE(outcome.reasons.size() == 1);
    CHECK(outcome.reasons[0].code == ReasonCode::NVML_NO_PERMISSION);
    REQUIRE(outcome.reasons[0].observed.has_value());
    CHECK((*outcome.reasons[0].observed)["name"] == "NVML_ERROR_OPERATING_SYSTEM");
    CHECK(fake.shutdown_calls == 0);
  }

  SECTION("driver initialization failure") {
    FakeState fake = healthy_fake();
    fake.init_result = 9;

    const ProbeOutcome outcome = detail::collect_nvml(fake_api(fake));

    CHECK(outcome.observed.nvml.status == NvmlStatus::INITIALIZATION_FAILED);
    REQUIRE(outcome.reasons.size() == 1);
    CHECK(outcome.reasons[0].code == ReasonCode::NVML_INIT_FAILED);
    REQUIRE(outcome.reasons[0].observed.has_value());
    CHECK((*outcome.reasons[0].observed)["name"] == "NVML_ERROR_DRIVER_NOT_LOADED");
    CHECK(fake.shutdown_calls == 0);
  }

  SECTION("incomplete table never calls NVML") {
    const ProbeOutcome outcome = detail::collect_nvml(detail::NvmlApi{});

    CHECK(outcome.observed.nvml.status == NvmlStatus::INITIALIZATION_FAILED);
    REQUIRE(outcome.reasons.size() == 1);
    CHECK(outcome.reasons[0].code == ReasonCode::NVML_INIT_FAILED);
  }
}

TEST_CASE("NVML required-query failures return one precise stack failure", "[nvml]") {
  SECTION("count failure does not masquerade as no devices") {
    FakeState fake = healthy_fake();
    fake.count_result = 999;

    ProbeOutcome outcome = detail::collect_nvml(fake_api(fake));
    CHECK(outcome.observed.nvml.status == NvmlStatus::READY);
    REQUIRE(outcome.reasons.size() == 1);
    CHECK(outcome.reasons[0].code == ReasonCode::PROBE_OUTPUT_INVALID);
    CHECK(outcome.reasons[0].field == "nvml.device_count");
    CHECK(fake.shutdown_calls == 1);

    const Result result = evaluate_inventory(std::move(outcome));
    REQUIRE(result.reasons.size() == 1);
    CHECK(result.reasons[0].code == ReasonCode::PROBE_OUTPUT_INVALID);
    CHECK(result.exit_code == ExitCode::FAIL_STACK);
  }

  SECTION("device query identifies its exact field") {
    FakeState fake = healthy_fake();
    fake.uuid_result = 15;

    const ProbeOutcome outcome = detail::collect_nvml(fake_api(fake));

    REQUIRE(outcome.reasons.size() == 1);
    CHECK(outcome.reasons[0].code == ReasonCode::PROBE_OUTPUT_INVALID);
    CHECK(outcome.reasons[0].field == "gpus[0].uuid");
    CHECK(fake.shutdown_calls == 1);
  }
}

TEST_CASE("NVML per-device permission failure is authoritative", "[nvml]") {
  FakeState fake = healthy_fake();
  fake.handle_result = detail::kNvmlErrorNoPermission;

  ProbeOutcome outcome = detail::collect_nvml(fake_api(fake));

  CHECK(outcome.observed.nvml.status == NvmlStatus::NO_PERMISSION);
  REQUIRE(outcome.reasons.size() == 1);
  CHECK(outcome.reasons[0].code == ReasonCode::NVML_NO_PERMISSION);
  CHECK(outcome.reasons[0].field == "gpus[0].handle");
  CHECK(fake.shutdown_calls == 1);

  const Result result = evaluate_inventory(std::move(outcome));
  REQUIRE(result.reasons.size() == 1);
  CHECK(result.reasons[0].code == ReasonCode::NVML_NO_PERMISSION);
}

TEST_CASE("NVML zero-device result reaches the no-device evaluation path", "[nvml]") {
  FakeState fake{};
  ProbeOutcome outcome = detail::collect_nvml(fake_api(fake));

  CHECK(outcome.observed.nvml.status == NvmlStatus::READY);
  CHECK(outcome.observed.nvml.driver_version == "550.90.07");
  CHECK(outcome.observed.gpus.empty());
  CHECK(outcome.reasons.empty());
  CHECK(fake.shutdown_calls == 1);

  const Result result = evaluate_inventory(std::move(outcome));
  REQUIRE(result.reasons.size() == 1);
  CHECK(result.reasons[0].code == ReasonCode::NO_NVIDIA_DEVICES);
}

TEST_CASE("NVML rejects malformed successful query output", "[nvml]") {
  SECTION("empty driver string") {
    FakeState fake = healthy_fake();
    fake.driver_version.clear();

    const ProbeOutcome outcome = detail::collect_nvml(fake_api(fake));

    REQUIRE(outcome.reasons.size() == 1);
    CHECK(outcome.reasons[0].code == ReasonCode::PROBE_OUTPUT_INVALID);
    CHECK(outcome.reasons[0].field == "nvml.driver_version");
    CHECK(fake.shutdown_calls == 1);
  }

  SECTION("sub-MiB memory cannot become zero MiB identity") {
    FakeState fake = healthy_fake();
    fake.gpus[0].memory_bytes = 1024;

    const ProbeOutcome outcome = detail::collect_nvml(fake_api(fake));

    REQUIRE(outcome.reasons.size() == 1);
    CHECK(outcome.reasons[0].code == ReasonCode::PROBE_OUTPUT_INVALID);
    CHECK(outcome.reasons[0].field == "gpus[0].memory_mib");
    CHECK(fake.shutdown_calls == 1);
  }
}

TEST_CASE("explicit NVML simulation returns conspicuously tagged identity", "[nvml]") {
  const Result result = evaluate_inventory(probe_simulated_nvml());

  CHECK(result.verdict == Verdict::OBSERVED);
  CHECK(result.exit_code == ExitCode::OK);
  CHECK(result.reasons.empty());
  REQUIRE(result.observed.has_value());
  CHECK(result.observed->nvml.status == NvmlStatus::READY);
  CHECK(result.observed->nvml.driver_version == "550.90.07");
  REQUIRE(result.observed->gpus.size() == 1);
  CHECK(result.observed->gpus[0].name == "SIMULATED NVIDIA H100 80GB HBM3");
  CHECK(result.observed->gpus[0].uuid.find("SIMULATED") != std::string::npos);
  CHECK(result.observed->gpus[0].pci_bdf == "00000000:01:00.0");
  CHECK(result.observed->gpus[0].memory_mib == 81920);
}

} // namespace
} // namespace gpu_qual
