#include <catch2/catch_test_macros.hpp>
#include <gpu_qual/io_json.hpp>
#include <gpu_qual/observed.hpp>

#include <string>

using namespace gpu_qual;

TEST_CASE("NVML statuses have stable serialized names") {
  CHECK(std::string{to_string(NvmlStatus::NOT_PROBED)} == "not_probed");
  CHECK(std::string{to_string(NvmlStatus::LIBRARY_NOT_FOUND)} == "library_not_found");
  CHECK(std::string{to_string(NvmlStatus::INITIALIZATION_FAILED)} == "initialization_failed");
  CHECK(std::string{to_string(NvmlStatus::NO_PERMISSION)} == "no_permission");
  CHECK(std::string{to_string(NvmlStatus::READY)} == "ready");
}

TEST_CASE("to_json serializes the identity-only observed state") {
  const ObservedState observed{
      .nvml = {.status = NvmlStatus::READY, .driver_version = "550.144.03"},
      .gpus = {{
          .index = 0,
          .name = "NVIDIA H100 80GB HBM3",
          .uuid = "GPU-...",
          .pci_bdf = "0000:1b:00.0",
          .memory_mib = 81559,
      }},
  };

  const nlohmann::json expected = {
      {"nvml", {{"status", "ready"}, {"driver_version", "550.144.03"}}},
      {"gpu_count", 1},
      {"gpus",
       {{
           {"index", 0},
           {"name", "NVIDIA H100 80GB HBM3"},
           {"uuid", "GPU-..."},
           {"pci_bdf", "0000:1b:00.0"},
           {"memory_mib", 81559},
       }}},
  };

  CHECK(to_json(observed) == expected);
}

TEST_CASE("a default observed state is explicit and safe") {
  const nlohmann::json expected = {
      {"nvml", {{"status", "not_probed"}, {"driver_version", nullptr}}},
      {"gpu_count", 0},
      {"gpus", nlohmann::json::array()},
  };

  CHECK(to_json(ObservedState{}) == expected);
}

TEST_CASE("observed appears on a result only when present") {
  auto result = compute_result(Mode::INVENTORY, {});
  CHECK_FALSE(to_json(result).contains("observed"));

  result.observed = ObservedState{};
  const auto serialized = to_json(result);
  REQUIRE(serialized.contains("observed"));
  CHECK(serialized["observed"]["nvml"]["status"] == "not_probed");
  CHECK(serialized["observed"]["gpu_count"] == 0);
}
