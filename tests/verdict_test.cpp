#include <catch2/catch_test_macros.hpp>
#include <gpu_qual/verdict.hpp>

#include <string>

using namespace gpu_qual;

TEST_CASE("enum to_string returns stable lowercase names") {
  CHECK(std::string{to_string(Mode::INVENTORY)} == "inventory");
  CHECK(std::string{to_string(Mode::CHECK)} == "check");

  CHECK(std::string{to_string(Verdict::OBSERVED)} == "observed");
  CHECK(std::string{to_string(Verdict::PASS)} == "pass");
  CHECK(std::string{to_string(Verdict::WARN)} == "warn");
  CHECK(std::string{to_string(Verdict::RETRY)} == "retry");
  CHECK(std::string{to_string(Verdict::FAIL)} == "fail");

  CHECK(std::string{to_string(ReasonClass::REPORT)} == "report");
  CHECK(std::string{to_string(ReasonClass::WARN)} == "warn");
  CHECK(std::string{to_string(ReasonClass::HARD)} == "hard");
  CHECK(std::string{to_string(ReasonClass::RETRY)} == "retry");
}

TEST_CASE("ExitCode integer values preserve every routing band") {
  CHECK(static_cast<int>(ExitCode::OK) == 0);
  CHECK(static_cast<int>(ExitCode::WARN) == 10);
  CHECK(static_cast<int>(ExitCode::RETRY) == 20);
  CHECK(static_cast<int>(ExitCode::FAIL_CONTRACT) == 30);
  CHECK(static_cast<int>(ExitCode::FAIL_USABILITY) == 40);
  CHECK(static_cast<int>(ExitCode::FAIL_STACK) == 50);
}

TEST_CASE("every identity reason has a stable name, class, and exit band") {
  struct Row {
    ReasonCode code;
    const char* name;
    ReasonClass reason_class;
    ExitCode exit_code;
  };

  const Row rows[] = {
      {ReasonCode::GPU_COUNT_MISMATCH, "GPU_COUNT_MISMATCH", ReasonClass::HARD,
       ExitCode::FAIL_CONTRACT},
      {ReasonCode::GPU_NAME_MISMATCH, "GPU_NAME_MISMATCH", ReasonClass::HARD,
       ExitCode::FAIL_CONTRACT},
      {ReasonCode::GPU_MEMORY_BELOW_MIN, "GPU_MEMORY_BELOW_MIN", ReasonClass::HARD,
       ExitCode::FAIL_CONTRACT},
      {ReasonCode::NVML_LIBRARY_NOT_FOUND, "NVML_LIBRARY_NOT_FOUND", ReasonClass::HARD,
       ExitCode::FAIL_STACK},
      {ReasonCode::NVML_INIT_FAILED, "NVML_INIT_FAILED", ReasonClass::HARD, ExitCode::FAIL_STACK},
      {ReasonCode::NVML_NO_PERMISSION, "NVML_NO_PERMISSION", ReasonClass::HARD,
       ExitCode::FAIL_STACK},
      {ReasonCode::NO_NVIDIA_DEVICES, "NO_NVIDIA_DEVICES", ReasonClass::HARD, ExitCode::FAIL_STACK},
      {ReasonCode::DRIVER_VERSION_BELOW_MIN, "DRIVER_VERSION_BELOW_MIN", ReasonClass::HARD,
       ExitCode::FAIL_CONTRACT},
      {ReasonCode::EXPECTED_SPEC_INVALID, "EXPECTED_SPEC_INVALID", ReasonClass::HARD,
       ExitCode::FAIL_STACK},
      {ReasonCode::PROBE_OUTPUT_INVALID, "PROBE_OUTPUT_INVALID", ReasonClass::HARD,
       ExitCode::FAIL_STACK},
  };

  for (const auto& row : rows) {
    INFO(row.name);
    CHECK(std::string{to_string(row.code)} == row.name);
    CHECK(default_class(row.code) == row.reason_class);
    CHECK(default_exit_code(row.code) == row.exit_code);
  }
}

TEST_CASE("compute_result maps reasons to a verdict and highest exit band") {
  SECTION("no reasons yields the success verdict for each mode") {
    const auto inventory = compute_result(Mode::INVENTORY, {});
    CHECK(inventory.verdict == Verdict::OBSERVED);
    CHECK(inventory.exit_code == ExitCode::OK);

    const auto check = compute_result(Mode::CHECK, {});
    CHECK(check.verdict == Verdict::PASS);
    CHECK(check.exit_code == ExitCode::OK);
  }

  SECTION("a minimum-driver mismatch is a contract failure") {
    const auto result =
        compute_result(Mode::CHECK, {make_reason(ReasonCode::DRIVER_VERSION_BELOW_MIN)});
    CHECK(result.verdict == Verdict::FAIL);
    CHECK(result.exit_code == ExitCode::FAIL_CONTRACT);
  }

  SECTION("the highest-impact reason wins regardless of order") {
    const auto result =
        compute_result(Mode::CHECK, {
                                        make_reason(ReasonCode::NVML_LIBRARY_NOT_FOUND),
                                        make_reason(ReasonCode::DRIVER_VERSION_BELOW_MIN),
                                        make_reason(ReasonCode::GPU_COUNT_MISMATCH),
                                    });
    CHECK(result.verdict == Verdict::FAIL);
    CHECK(result.exit_code == ExitCode::FAIL_STACK);
  }
}
