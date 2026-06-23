#include <catch2/catch_test_macros.hpp>
#include <gpu_qual/io_json.hpp>

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

TEST_CASE("to_json gives a reason value keys only when they are set") {
  SECTION("a bare reason carries only code and class") {
    const auto j =
        to_json(compute_result(Mode::INVENTORY, {make_reason(ReasonCode::NVML_LIBRARY_NOT_FOUND)}));
    CHECK(j["verdict"] == "fail");
    CHECK(j["exit_code"] == 50);

    REQUIRE(j["reasons"].size() == 1);
    CHECK(j["reasons"][0] == nlohmann::json{
                                 {"code", "NVML_LIBRARY_NOT_FOUND"},
                                 {"class", "hard"},
                             });
  }

  SECTION("typed expected/observed values pass through unchanged") {
    const auto j = to_json(compute_result(
        Mode::CHECK, {make_reason(ReasonCode::GPU_COUNT_MISMATCH, "expected.gpu_count", 8, 7)}));
    REQUIRE(j["reasons"].size() == 1);

    const auto& jr = j["reasons"][0];
    CHECK(jr == nlohmann::json{
                    {"code", "GPU_COUNT_MISMATCH"},
                    {"class", "hard"},
                    {"field", "expected.gpu_count"},
                    {"expected", 8},
                    {"observed", 7},
                });
    CHECK(jr["expected"].is_number_integer());
    CHECK(jr["observed"].is_number_integer());
  }

  SECTION("a retry-class reason has no value keys") {
    const auto j =
        to_json(compute_result(Mode::INVENTORY, {make_reason(ReasonCode::PROBE_TIMEOUT)}));
    REQUIRE(j["reasons"].size() == 1);

    const auto& jr = j["reasons"][0];
    CHECK(jr["class"] == "retry");
    CHECK_FALSE(jr.contains("field"));
    CHECK_FALSE(jr.contains("expected"));
    CHECK_FALSE(jr.contains("observed"));
  }
}
