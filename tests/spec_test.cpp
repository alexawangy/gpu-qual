#include <catch2/catch_test_macros.hpp>
#include <gpu_qual/spec.hpp>

#include <string>

using namespace gpu_qual;

namespace {
// The full PLAN.md §7 example: every expected/health/fabric field, all twelve
// policy keys, and all four options.
constexpr const char* kFullSpec = R"({
  "schema_version": "1.0",
  "metadata": {
    "request_id": "lease-or-job-abc123",
    "node_id": "optional-caller-node-id"
  },
  "expected": {
    "gpu_count": 8,
    "allowed_names": ["NVIDIA H100 80GB HBM3"],
    "min_memory_mib": 80000,
    "mig_mode": "any",
    "min_cuda_visible_devices": 8,
    "min_driver_version": "550.90.07",
    "health": {
      "ecc_mode_enabled": true,
      "max_volatile_uncorrectable_ecc": 0,
      "max_aggregate_uncorrectable_ecc": null,
      "allow_row_remap_pending": false,
      "allow_row_remap_failure": false,
      "allow_pending_retired_pages": false,
      "disallowed_recovery_actions": ["none", "reset", "reset_and_drain", "reboot", "field_rma"]
    },
    "fabric": {
      "require_fabric_ready": true
    }
  },
  "policy": {
    "gpu_count": "hard",
    "gpu_name": "hard",
    "memory": "hard",
    "mig_mode": "hard",
    "cuda_visible_devices": "report",
    "driver_version": "hard",
    "ecc": "hard",
    "row_remap": "hard",
    "retired_pages": "warn",
    "recovery_action": "hard",
    "fabric": "hard",
    "cuda_smoke": "hard"
  },
  "options": {
    "cuda_smoke": true,
    "include_health": false,
    "timeout_ms": 20000,
    "cuda_smoke_timeout_ms": 7000
  }
})";
} // namespace

TEST_CASE("parse_spec accepts valid specs") {
  SECTION("the full PLAN.md example parses to a fully populated spec") {
    SpecParseResult res = parse_spec(kFullSpec);

    REQUIRE(res.ok());
    REQUIRE(res.reasons.empty());

    const ExpectedSpec& spec = *res.spec;
    CHECK(spec.schema_version == "1.0");
    CHECK(spec.metadata.is_object());
    CHECK(spec.metadata.at("request_id") == "lease-or-job-abc123");

    CHECK(spec.expected.gpu_count == 8);
    REQUIRE(spec.expected.allowed_names.size() == 1);
    CHECK(spec.expected.allowed_names[0] == "NVIDIA H100 80GB HBM3");
    CHECK(spec.expected.min_memory_mib == 80000);
    REQUIRE(spec.expected.mig_mode.has_value());
    CHECK(*spec.expected.mig_mode == MigExpectation::ANY); // 'any' is represented, not dropped
    CHECK(spec.expected.min_cuda_visible_devices == 8);
    CHECK(spec.expected.min_driver_version == "550.90.07");

    CHECK(spec.expected.health.ecc_mode_enabled == true);
    CHECK(spec.expected.health.max_volatile_uncorrectable_ecc == 0);
    CHECK_FALSE(spec.expected.health.max_aggregate_uncorrectable_ecc.has_value()); // null -> unset
    CHECK(spec.expected.health.allow_row_remap_pending == false);
    const std::vector<RecoveryAction> expected_actions = {
        RecoveryAction::NONE, RecoveryAction::RESET, RecoveryAction::RESET_AND_DRAIN,
        RecoveryAction::REBOOT, RecoveryAction::FIELD_RMA};
    CHECK(spec.expected.health.disallowed_recovery_actions == expected_actions);

    REQUIRE(spec.expected.fabric.require_fabric_ready.has_value());
    CHECK(*spec.expected.fabric.require_fabric_ready == true);

    CHECK(spec.options.cuda_smoke == true);
    CHECK(spec.options.include_health == false);
    CHECK(spec.options.timeout_ms == 20000);
    CHECK(spec.options.cuda_smoke_timeout_ms == 7000);
  }

  SECTION("a minimal spec leaves expected unset and policy/options at defaults") {
    SpecParseResult res = parse_spec(R"({"schema_version":"1.0"})");

    REQUIRE(res.ok());
    CHECK(res.reasons.empty());

    const ExpectedSpec& spec = *res.spec;
    CHECK(spec.schema_version == "1.0");
    CHECK(spec.metadata.is_null());

    CHECK_FALSE(spec.expected.gpu_count.has_value());
    CHECK(spec.expected.allowed_names.empty());
    CHECK_FALSE(spec.expected.mig_mode.has_value());
    CHECK(spec.expected.health.disallowed_recovery_actions.empty());

    // Struct defaults from the schema's `default` annotations.
    CHECK(spec.policy.gpu_count == Severity::HARD);
    CHECK(spec.policy.cuda_visible_devices == Severity::REPORT);
    CHECK(spec.policy.driver_version == Severity::WARN);
    CHECK(spec.policy.retired_pages == Severity::WARN);
    CHECK(spec.options.cuda_smoke == false);
    CHECK(spec.options.include_health == true);
    CHECK(spec.options.timeout_ms == 10000);
    CHECK(spec.options.cuda_smoke_timeout_ms == 5000);
  }

  SECTION("present policy keys override defaults; missing keys keep them") {
    SpecParseResult res = parse_spec(R"({
      "schema_version": "1.0",
      "policy": {"driver_version": "hard"}
    })");

    REQUIRE(res.ok());
    CHECK(res.reasons.empty());
    CHECK(res.spec->policy.driver_version == Severity::HARD); // overridden from warn
    CHECK(res.spec->policy.retired_pages == Severity::WARN);  // still default
  }

  SECTION("min_cuda_visible_devices: null is treated as unset") {
    SpecParseResult res = parse_spec(R"({
      "schema_version": "1.0",
      "expected": {"min_cuda_visible_devices": null}
    })");

    REQUIRE(res.ok());
    CHECK_FALSE(res.spec->expected.min_cuda_visible_devices.has_value());
  }

  SECTION("enum string helpers round-trip and reject garbage") {
    CHECK(severity_from_string(to_string(Severity::HARD)) == Severity::HARD);
    CHECK(severity_from_string(to_string(Severity::WARN)) == Severity::WARN);
    CHECK(severity_from_string(to_string(Severity::REPORT)) == Severity::REPORT);
    CHECK_FALSE(severity_from_string("reject").has_value());

    CHECK(mig_expectation_from_string("enabled") == MigExpectation::ENABLED);
    CHECK(mig_expectation_from_string("disabled") == MigExpectation::DISABLED);
    CHECK(mig_expectation_from_string("any") == MigExpectation::ANY);
    CHECK_FALSE(mig_expectation_from_string("off").has_value());

    CHECK(recovery_action_from_string(to_string(RecoveryAction::RESET_AND_DRAIN)) ==
          RecoveryAction::RESET_AND_DRAIN);
    CHECK(recovery_action_from_string(to_string(RecoveryAction::FIELD_RMA)) ==
          RecoveryAction::FIELD_RMA);
    CHECK_FALSE(recovery_action_from_string("meltdown").has_value());
  }
}

TEST_CASE("parse_spec rejects invalid specs and records leniency") {
  // Asserts a single EXPECTED_SPEC_INVALID (hard) reason at `field`, no spec.
  auto expect_invalid = [](const std::string& json_text, const std::string& field) {
    SpecParseResult res = parse_spec(json_text);
    INFO("input: " << json_text);
    CHECK_FALSE(res.ok());
    CHECK_FALSE(res.spec.has_value());
    REQUIRE(res.reasons.size() == 1);
    CHECK(res.reasons[0].code == ReasonCode::EXPECTED_SPEC_INVALID);
    CHECK(res.reasons[0].cls == ReasonClass::HARD);
    CHECK(res.reasons[0].field == field);
  };

  SECTION("top-level structural failures") {
    expect_invalid("{not json", "");
    expect_invalid("[1, 2, 3]", "");
    expect_invalid(R"({"metadata": {}})", "schema_version");
    expect_invalid(R"({"schema_version": 1.0})", "schema_version");
  }

  SECTION("integer fields reject non-integers and out-of-range values") {
    expect_invalid(R"({"schema_version":"1.0","expected":{"gpu_count":0}})", "expected.gpu_count");
    expect_invalid(R"({"schema_version":"1.0","expected":{"gpu_count":8.5}})",
                   "expected.gpu_count");
    expect_invalid(R"({"schema_version":"1.0","expected":{"gpu_count":"8"}})",
                   "expected.gpu_count");
    expect_invalid(R"({"schema_version":"1.0","expected":{"min_memory_mib":0}})",
                   "expected.min_memory_mib");
    expect_invalid(R"({"schema_version":"1.0","options":{"timeout_ms":0}})", "options.timeout_ms");
  }

  SECTION("enum, array, and recovery-action failures") {
    expect_invalid(R"({"schema_version":"1.0","expected":{"mig_mode":"off"}})",
                   "expected.mig_mode");
    expect_invalid(R"({"schema_version":"1.0","policy":{"gpu_count":"reject"}})",
                   "policy.gpu_count");
    expect_invalid(R"({"schema_version":"1.0","expected":{"allowed_names":[]}})",
                   "expected.allowed_names");
    expect_invalid(R"({"schema_version":"1.0","expected":{"allowed_names":["","x"]}})",
                   "expected.allowed_names");
    expect_invalid(
        R"({"schema_version":"1.0","expected":{"health":{"disallowed_recovery_actions":["meltdown"]}}})",
        "expected.health.disallowed_recovery_actions");
  }

  SECTION("metadata must be an object and stay under the size cap") {
    expect_invalid(R"({"schema_version":"1.0","metadata":"not-an-object"})", "metadata");

    std::string big = R"({"schema_version":"1.0","metadata":{"blob":")";
    big.append(kMaxMetadataBytes, 'x');
    big += "\"}}";
    expect_invalid(big, "metadata");
  }

  SECTION("unknown non-metadata keys are recorded as report reasons, not fatal") {
    SpecParseResult res = parse_spec(R"({
      "schema_version": "1.0",
      "surprise": 1,
      "expected": {"mystery": true},
      "policy": {"bogus": "hard"}
    })");

    REQUIRE(res.ok());
    REQUIRE(res.reasons.size() == 3);
    for (const Reason& r : res.reasons) {
      CHECK(r.code == ReasonCode::UNKNOWN_FIELD_IGNORED);
      CHECK(r.cls == ReasonClass::REPORT);
    }

    auto has_field = [&](const std::string& f) {
      for (const Reason& r : res.reasons) {
        if (r.field == f) {
          return true;
        }
      }
      return false;
    };
    CHECK(has_field("surprise"));
    CHECK(has_field("expected.mystery"));
    CHECK(has_field("policy.bogus"));
  }

  SECTION("unknown keys inside metadata are preserved verbatim with no reason") {
    SpecParseResult res = parse_spec(R"({
      "schema_version": "1.0",
      "metadata": {"anything": {"nested": [1, 2, 3]}, "weird_key": "kept"}
    })");

    REQUIRE(res.ok());
    CHECK(res.reasons.empty());
    CHECK(res.spec->metadata.at("weird_key") == "kept");
    CHECK(res.spec->metadata.at("anything").at("nested") == std::vector<int>{1, 2, 3});
  }
}
