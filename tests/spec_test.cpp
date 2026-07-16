#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <limits>
#include <string>

#include <gpu_qual/spec.hpp>
#include <gpu_qual/version.hpp>

using namespace gpu_qual;

namespace {

void expect_invalid(const std::string& input, const std::string& field) {
  const SpecParseResult result = parse_spec(input);
  INFO("input: " << input);
  CHECK_FALSE(result.ok());
  CHECK_FALSE(result.spec.has_value());
  REQUIRE(result.error.has_value());
  CHECK(result.error->code == ReasonCode::EXPECTED_SPEC_INVALID);
  CHECK(result.error->field == field);
  CHECK(result.error->observed.has_value());
  CHECK(result.error->observed->is_string());
}

} // namespace

TEST_CASE("parse_spec accepts the minimal identity contract") {
  SECTION("expected may be absent") {
    const SpecParseResult result = parse_spec(R"({"schema_version":"0.1"})");

    REQUIRE(result.ok());
    CHECK_FALSE(result.error.has_value());
    CHECK_FALSE(result.spec->gpu_count.has_value());
    CHECK(result.spec->allowed_names.empty());
    CHECK_FALSE(result.spec->min_memory_mib.has_value());
    CHECK_FALSE(result.spec->min_driver_version.has_value());
  }

  SECTION("expected may be empty") {
    const SpecParseResult result = parse_spec(R"({"schema_version":"0.1","expected":{}})");

    REQUIRE(result.ok());
    CHECK_FALSE(result.error.has_value());
  }
}

TEST_CASE("parse_spec accepts a full identity contract") {
  const SpecParseResult result = parse_spec(R"({
    "schema_version": "0.1",
    "expected": {
      "gpu_count": 8,
      "allowed_names": ["NVIDIA H100 80GB HBM3", "NVIDIA H200"],
      "min_memory_mib": 80000,
      "min_driver_version": "550.90.07"
    }
  })");

  REQUIRE(result.ok());
  CHECK_FALSE(result.error.has_value());
  CHECK(result.spec->gpu_count == 8U);
  CHECK(result.spec->allowed_names ==
        std::vector<std::string>{"NVIDIA H100 80GB HBM3", "NVIDIA H200"});
  CHECK(result.spec->min_memory_mib == std::uint64_t{80000});
  CHECK(result.spec->min_driver_version == "550.90.07");
}

TEST_CASE("parse_spec enforces the schema envelope") {
  expect_invalid("{not JSON", "");
  expect_invalid("[]", "");
  expect_invalid(R"({"expected":{}})", "schema_version");
  expect_invalid(R"({"schema_version":null})", "schema_version");
  expect_invalid(R"({"schema_version":0.1})", "schema_version");
  expect_invalid(R"({"schema_version":"1.0"})", "schema_version");
  expect_invalid(R"({"schema_version":"0.1","metadata":{}})", "metadata");
  expect_invalid(R"({"schema_version":"0.1","expected":{"mig_mode":"any"}})", "expected.mig_mode");
  expect_invalid(R"({"schema_version":"0.1","expected":null})", "expected");
}

TEST_CASE("parse_spec rejects null and wrong identity field types") {
  expect_invalid(R"({"schema_version":"0.1","expected":{"gpu_count":null}})", "expected.gpu_count");
  expect_invalid(R"({"schema_version":"0.1","expected":{"gpu_count":"8"}})", "expected.gpu_count");
  expect_invalid(R"({"schema_version":"0.1","expected":{"gpu_count":8.5}})", "expected.gpu_count");
  expect_invalid(R"({"schema_version":"0.1","expected":{"allowed_names":null}})",
                 "expected.allowed_names");
  expect_invalid(R"({"schema_version":"0.1","expected":{"allowed_names":"H100"}})",
                 "expected.allowed_names");
  expect_invalid(R"({"schema_version":"0.1","expected":{"allowed_names":[1]}})",
                 "expected.allowed_names");
  expect_invalid(R"({"schema_version":"0.1","expected":{"min_memory_mib":null}})",
                 "expected.min_memory_mib");
  expect_invalid(R"({"schema_version":"0.1","expected":{"min_memory_mib":"80000"}})",
                 "expected.min_memory_mib");
  expect_invalid(R"({"schema_version":"0.1","expected":{"min_driver_version":null}})",
                 "expected.min_driver_version");
  expect_invalid(R"({"schema_version":"0.1","expected":{"min_driver_version":550}})",
                 "expected.min_driver_version");
}

TEST_CASE("parse_spec requires positive in-range integer fields") {
  expect_invalid(R"({"schema_version":"0.1","expected":{"gpu_count":0}})", "expected.gpu_count");
  expect_invalid(R"({"schema_version":"0.1","expected":{"gpu_count":-1}})", "expected.gpu_count");
  expect_invalid(R"({"schema_version":"0.1","expected":{"gpu_count":4294967296}})",
                 "expected.gpu_count");
  expect_invalid(R"({"schema_version":"0.1","expected":{"min_memory_mib":0}})",
                 "expected.min_memory_mib");
  expect_invalid(R"({"schema_version":"0.1","expected":{"min_memory_mib":-1}})",
                 "expected.min_memory_mib");
  expect_invalid(R"({"schema_version":"0.1","expected":{"min_memory_mib":18446744073709551616}})",
                 "expected.min_memory_mib");

  const SpecParseResult maximum =
      parse_spec(R"({"schema_version":"0.1","expected":{"min_memory_mib":18446744073709551615}})");
  REQUIRE(maximum.ok());
  REQUIRE(maximum.spec->min_memory_mib.has_value());
  CHECK(*maximum.spec->min_memory_mib == std::numeric_limits<std::uint64_t>::max());

  const SpecParseResult integral_float =
      parse_spec(R"({"schema_version":"0.1","expected":{"gpu_count":8.0,"min_memory_mib":8e4}})");
  REQUIRE(integral_float.ok());
  CHECK(integral_float.spec->gpu_count == 8U);
  CHECK(integral_float.spec->min_memory_mib == std::uint64_t{80000});
}

TEST_CASE("parse_spec validates allowed GPU names") {
  expect_invalid(R"({"schema_version":"0.1","expected":{"allowed_names":[]}})",
                 "expected.allowed_names");
  expect_invalid(R"({"schema_version":"0.1","expected":{"allowed_names":[""]}})",
                 "expected.allowed_names");
  expect_invalid(R"({"schema_version":"0.1","expected":{"allowed_names":["H100","H100"]}})",
                 "expected.allowed_names");
}

TEST_CASE("parse_spec validates driver version components") {
  for (const std::string& version : {"", ".550", "550.", "550..90", "550.a", "-1.2", "+550.90"}) {
    expect_invalid("{\"schema_version\":\"0.1\",\"expected\":{\"min_driver_version\":\"" + version +
                       "\"}}",
                   "expected.min_driver_version");
  }

  const SpecParseResult leading_zeroes =
      parse_spec(R"({"schema_version":"0.1","expected":{"min_driver_version":"0550.090.007"}})");
  CHECK(leading_zeroes.ok());

  const SpecParseResult large_component = parse_spec(
      R"({"schema_version":"0.1","expected":{"min_driver_version":"429496729600000000000000.1"}})");
  CHECK(large_component.ok());
}
