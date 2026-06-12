#include <catch2/catch_test_macros.hpp>
#include <gpu_qual/io_json.hpp>

TEST_CASE("to_json emits the inventory-success golden object") {
    const auto result = gpu_qual::compute_result(gpu_qual::Mode::INVENTORY, {});

    const nlohmann::json expected = {
        {"tool_version", gpu_qual::kToolVersion},
        {"schema_version", gpu_qual::kSchemaVersion},
        {"mode", "inventory"},
        {"verdict", "observed"},
        {"exit_code", 0},
        {"reasons", nlohmann::json::array()},
    };

    CHECK(gpu_qual::to_json(result) == expected);
}

TEST_CASE("to_json serializes a bare reason minimally, omitting unset keys") {
    const auto result = gpu_qual::compute_result(
        gpu_qual::Mode::INVENTORY,
        {gpu_qual::make_reason(gpu_qual::ReasonCode::NVML_LIBRARY_NOT_FOUND)}
    );

    const auto j = gpu_qual::to_json(result);
    CHECK(j["verdict"] == "fail");
    CHECK(j["exit_code"] == 50);

    const nlohmann::json expected_reason = {
        {"code", "NVML_LIBRARY_NOT_FOUND"},
        {"class", "hard"},
    };
    REQUIRE(j["reasons"].size() == 1);
    CHECK(j["reasons"][0] == expected_reason);
}

TEST_CASE("to_json passes typed expected/observed values through as-is") {
    const auto result = gpu_qual::compute_result(
        gpu_qual::Mode::CHECK,
        {gpu_qual::make_reason(gpu_qual::ReasonCode::GPU_COUNT_MISMATCH,
                               "expected.gpu_count", 8, 7)}
    );

    const auto j = gpu_qual::to_json(result);
    REQUIRE(j["reasons"].size() == 1);

    const auto &jr = j["reasons"][0];
    const nlohmann::json expected_reason = {
        {"code", "GPU_COUNT_MISMATCH"},
        {"class", "hard"},
        {"field", "expected.gpu_count"},
        {"expected", 8},
        {"observed", 7},
    };
    CHECK(jr == expected_reason);
    CHECK(jr["expected"].is_number_integer());
    CHECK(jr["observed"].is_number_integer());
}

TEST_CASE("to_json gives a retry-class reason no value keys") {
    const auto result = gpu_qual::compute_result(
        gpu_qual::Mode::INVENTORY,
        {gpu_qual::make_reason(gpu_qual::ReasonCode::PROBE_TIMEOUT)}
    );

    const auto j = gpu_qual::to_json(result);
    REQUIRE(j["reasons"].size() == 1);

    const auto &jr = j["reasons"][0];
    CHECK(jr["class"] == "retry");
    CHECK_FALSE(jr.contains("field"));
    CHECK_FALSE(jr.contains("expected"));
    CHECK_FALSE(jr.contains("observed"));
}
