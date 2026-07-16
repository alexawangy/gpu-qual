#include <catch2/catch_test_macros.hpp>
#include <gpu_qual/io_json.hpp>

#include <optional>
#include <string>

using namespace gpu_qual;

TEST_CASE("version constants are stable") {
  CHECK(std::string{kToolVersion} == "gpu-qual/0.1.0");
  CHECK(std::string{kSchemaVersion} == "0.1");
}

TEST_CASE("to_json emits the inventory-success golden object") {
  const nlohmann::json expected = {
      {"tool_version", kToolVersion},
      {"schema_version", kSchemaVersion},
      {"mode", "inventory"},
      {"verdict", "observed"},
      {"exit_code", 0},
      {"reasons", nlohmann::json::array()},
  };

  CHECK(to_json(compute_result(Mode::INVENTORY, {})) == expected);
}

TEST_CASE("to_json emits only present reason details") {
  SECTION("a bare reason carries only its derived code and class") {
    const auto serialized =
        to_json(compute_result(Mode::INVENTORY, {make_reason(ReasonCode::NVML_LIBRARY_NOT_FOUND)}));

    CHECK(serialized["verdict"] == "fail");
    CHECK(serialized["exit_code"] == 50);
    REQUIRE(serialized["reasons"].size() == 1);
    CHECK(serialized["reasons"][0] == nlohmann::json{
                                          {"code", "NVML_LIBRARY_NOT_FOUND"},
                                          {"class", "hard"},
                                      });
  }

  SECTION("typed expected and observed values pass through unchanged") {
    const auto serialized = to_json(compute_result(
        Mode::CHECK, {make_reason(ReasonCode::GPU_COUNT_MISMATCH, "gpu_count", 8U, 7U)}));
    REQUIRE(serialized["reasons"].size() == 1);

    const auto& reason = serialized["reasons"][0];
    CHECK(reason == nlohmann::json{
                        {"code", "GPU_COUNT_MISMATCH"},
                        {"class", "hard"},
                        {"field", "gpu_count"},
                        {"expected", 8},
                        {"observed", 7},
                    });
    CHECK(reason["expected"].is_number_unsigned());
    CHECK(reason["observed"].is_number_unsigned());
  }

  SECTION("an explicit JSON null is distinct from an absent value") {
    const auto serialized = to_json(compute_result(
        Mode::CHECK, {make_reason(ReasonCode::EXPECTED_SPEC_INVALID, "expected.gpu_count",
                                  nlohmann::json(nullptr), std::nullopt)}));
    REQUIRE(serialized["reasons"].size() == 1);

    const auto& reason = serialized["reasons"][0];
    REQUIRE(reason.contains("expected"));
    CHECK(reason["expected"].is_null());
    CHECK_FALSE(reason.contains("observed"));
  }

  SECTION("a hard class is derived from a minimum-driver mismatch") {
    const auto serialized =
        to_json(compute_result(Mode::CHECK, {make_reason(ReasonCode::DRIVER_VERSION_BELOW_MIN)}));
    CHECK(serialized["reasons"][0]["class"] == "hard");
    CHECK(serialized["exit_code"] == 30);
  }
}
