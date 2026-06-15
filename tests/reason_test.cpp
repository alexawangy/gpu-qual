#include <catch2/catch_test_macros.hpp>
#include <gpu_qual/verdict.hpp>

#include <string>

TEST_CASE("ReasonClass to_string returns stable names") {
    CHECK(std::string{gpu_qual::to_string(gpu_qual::ReasonClass::REPORT)} == "report");
    CHECK(std::string{gpu_qual::to_string(gpu_qual::ReasonClass::WARN)}   == "warn");
    CHECK(std::string{gpu_qual::to_string(gpu_qual::ReasonClass::HARD)}   == "hard");
    CHECK(std::string{gpu_qual::to_string(gpu_qual::ReasonClass::RETRY)}  == "retry");
}

TEST_CASE("ReasonCode to_string returns stable names") {
    using gpu_qual::ReasonCode;
    using gpu_qual::to_string;

    CHECK(std::string{to_string(ReasonCode::GPU_COUNT_MISMATCH)}           == "GPU_COUNT_MISMATCH");
    CHECK(std::string{to_string(ReasonCode::GPU_NAME_MISMATCH)}            == "GPU_NAME_MISMATCH");
    CHECK(std::string{to_string(ReasonCode::GPU_MEMORY_BELOW_MIN)}         == "GPU_MEMORY_BELOW_MIN");
    CHECK(std::string{to_string(ReasonCode::MIG_MODE_MISMATCH)}            == "MIG_MODE_MISMATCH");
    CHECK(std::string{to_string(ReasonCode::CUDA_VISIBLE_COUNT_BELOW_MIN)} == "CUDA_VISIBLE_COUNT_BELOW_MIN");
    CHECK(std::string{to_string(ReasonCode::NVML_LIBRARY_NOT_FOUND)}       == "NVML_LIBRARY_NOT_FOUND");
    CHECK(std::string{to_string(ReasonCode::NVML_INIT_FAILED)}             == "NVML_INIT_FAILED");
    CHECK(std::string{to_string(ReasonCode::NVML_NO_PERMISSION)}           == "NVML_NO_PERMISSION");
    CHECK(std::string{to_string(ReasonCode::NO_NVIDIA_DEVICES)}            == "NO_NVIDIA_DEVICES");
    CHECK(std::string{to_string(ReasonCode::CUDA_LIBRARY_NOT_FOUND)}       == "CUDA_LIBRARY_NOT_FOUND");
    CHECK(std::string{to_string(ReasonCode::CUDA_INIT_FAILED)}             == "CUDA_INIT_FAILED");
    CHECK(std::string{to_string(ReasonCode::CUDA_CONTEXT_FAILED)}          == "CUDA_CONTEXT_FAILED");
    CHECK(std::string{to_string(ReasonCode::CUDA_SMOKE_FAILED)}            == "CUDA_SMOKE_FAILED");
    CHECK(std::string{to_string(ReasonCode::EXPECTED_SPEC_INVALID)}        == "EXPECTED_SPEC_INVALID");
    CHECK(std::string{to_string(ReasonCode::PROBE_TIMEOUT)}                == "PROBE_TIMEOUT");
    CHECK(std::string{to_string(ReasonCode::PROBE_CHILD_CRASHED)}          == "PROBE_CHILD_CRASHED");
    CHECK(std::string{to_string(ReasonCode::PROBE_OUTPUT_INVALID)}         == "PROBE_OUTPUT_INVALID");
    CHECK(std::string{to_string(ReasonCode::FIELD_UNSUPPORTED)}            == "FIELD_UNSUPPORTED");
    CHECK(std::string{to_string(ReasonCode::UNKNOWN_FIELD_IGNORED)}        == "UNKNOWN_FIELD_IGNORED");
}

TEST_CASE("contract health and fabric reason codes expose stable names and classes") {
    using gpu_qual::ReasonClass;
    using gpu_qual::ReasonCode;
    using gpu_qual::default_class;
    using gpu_qual::to_string;

    struct Row {
        ReasonCode code;
        const char *name;
        ReasonClass cls;
    };

    const Row rows[] = {
        {ReasonCode::DRIVER_VERSION_BELOW_MIN,      "DRIVER_VERSION_BELOW_MIN",      ReasonClass::WARN},
        {ReasonCode::ECC_MODE_MISMATCH,             "ECC_MODE_MISMATCH",             ReasonClass::HARD},
        {ReasonCode::ECC_UNCORRECTABLE_DETECTED,    "ECC_UNCORRECTABLE_DETECTED",    ReasonClass::HARD},
        {ReasonCode::ROW_REMAP_PENDING,             "ROW_REMAP_PENDING",             ReasonClass::HARD},
        {ReasonCode::ROW_REMAP_FAILURE,             "ROW_REMAP_FAILURE",             ReasonClass::HARD},
        {ReasonCode::RETIRED_PAGES_PENDING,         "RETIRED_PAGES_PENDING",         ReasonClass::WARN},
        {ReasonCode::GPU_RECOVERY_ACTION_REQUIRED,  "GPU_RECOVERY_ACTION_REQUIRED",  ReasonClass::HARD},
        {ReasonCode::FABRIC_NOT_READY,              "FABRIC_NOT_READY",              ReasonClass::HARD},
        {ReasonCode::FABRIC_NOT_APPLICABLE,         "FABRIC_NOT_APPLICABLE",         ReasonClass::REPORT},
    };

    for (const auto &row : rows) {
        INFO(row.name);
        CHECK(std::string{to_string(row.code)} == row.name);
        CHECK(default_class(row.code) == row.cls);
    }
}

TEST_CASE("default_class returns correct class for each code") {
    using gpu_qual::ReasonClass;
    using gpu_qual::ReasonCode;
    using gpu_qual::default_class;

    CHECK(default_class(ReasonCode::PROBE_TIMEOUT)                == ReasonClass::RETRY);

    CHECK(default_class(ReasonCode::CUDA_VISIBLE_COUNT_BELOW_MIN) == ReasonClass::REPORT);
    CHECK(default_class(ReasonCode::FIELD_UNSUPPORTED)            == ReasonClass::REPORT);
    CHECK(default_class(ReasonCode::UNKNOWN_FIELD_IGNORED)        == ReasonClass::REPORT);

    CHECK(default_class(ReasonCode::GPU_COUNT_MISMATCH)           == ReasonClass::HARD);
    CHECK(default_class(ReasonCode::GPU_NAME_MISMATCH)            == ReasonClass::HARD);
    CHECK(default_class(ReasonCode::GPU_MEMORY_BELOW_MIN)         == ReasonClass::HARD);
    CHECK(default_class(ReasonCode::MIG_MODE_MISMATCH)            == ReasonClass::HARD);
    CHECK(default_class(ReasonCode::NVML_LIBRARY_NOT_FOUND)       == ReasonClass::HARD);
    CHECK(default_class(ReasonCode::NVML_INIT_FAILED)             == ReasonClass::HARD);
    CHECK(default_class(ReasonCode::NVML_NO_PERMISSION)           == ReasonClass::HARD);
    CHECK(default_class(ReasonCode::NO_NVIDIA_DEVICES)            == ReasonClass::HARD);
    CHECK(default_class(ReasonCode::CUDA_LIBRARY_NOT_FOUND)       == ReasonClass::HARD);
    CHECK(default_class(ReasonCode::CUDA_INIT_FAILED)             == ReasonClass::HARD);
    CHECK(default_class(ReasonCode::CUDA_CONTEXT_FAILED)          == ReasonClass::HARD);
    CHECK(default_class(ReasonCode::CUDA_SMOKE_FAILED)            == ReasonClass::HARD);
    CHECK(default_class(ReasonCode::EXPECTED_SPEC_INVALID)        == ReasonClass::HARD);
    CHECK(default_class(ReasonCode::PROBE_CHILD_CRASHED)          == ReasonClass::HARD);
    CHECK(default_class(ReasonCode::PROBE_OUTPUT_INVALID)         == ReasonClass::HARD);
}

TEST_CASE("default_exit_code returns correct exit band for each code") {
    using gpu_qual::ExitCode;
    using gpu_qual::ReasonCode;
    using gpu_qual::default_exit_code;

    CHECK(default_exit_code(ReasonCode::GPU_COUNT_MISMATCH)    == ExitCode::FAIL_CONTRACT);
    CHECK(default_exit_code(ReasonCode::GPU_NAME_MISMATCH)     == ExitCode::FAIL_CONTRACT);
    CHECK(default_exit_code(ReasonCode::GPU_MEMORY_BELOW_MIN)  == ExitCode::FAIL_CONTRACT);
    CHECK(default_exit_code(ReasonCode::MIG_MODE_MISMATCH)     == ExitCode::FAIL_CONTRACT);

    CHECK(default_exit_code(ReasonCode::CUDA_LIBRARY_NOT_FOUND) == ExitCode::FAIL_USABILITY);
    CHECK(default_exit_code(ReasonCode::CUDA_INIT_FAILED)        == ExitCode::FAIL_USABILITY);
    CHECK(default_exit_code(ReasonCode::CUDA_CONTEXT_FAILED)     == ExitCode::FAIL_USABILITY);
    CHECK(default_exit_code(ReasonCode::CUDA_SMOKE_FAILED)       == ExitCode::FAIL_USABILITY);

    CHECK(default_exit_code(ReasonCode::PROBE_TIMEOUT)          == ExitCode::RETRY);
    CHECK(default_exit_code(ReasonCode::FIELD_UNSUPPORTED)      == ExitCode::WARN);
}

TEST_CASE("contract health and fabric reason codes have stable default exit bands") {
    using gpu_qual::ExitCode;
    using gpu_qual::ReasonCode;
    using gpu_qual::default_exit_code;

    struct Row {
        ReasonCode code;
        ExitCode exit_code;
    };

    const Row rows[] = {
        {ReasonCode::DRIVER_VERSION_BELOW_MIN,      ExitCode::WARN},
        {ReasonCode::ECC_MODE_MISMATCH,             ExitCode::FAIL_CONTRACT},
        {ReasonCode::ECC_UNCORRECTABLE_DETECTED,    ExitCode::FAIL_CONTRACT},
        {ReasonCode::ROW_REMAP_PENDING,             ExitCode::FAIL_CONTRACT},
        {ReasonCode::ROW_REMAP_FAILURE,             ExitCode::FAIL_STACK},
        {ReasonCode::RETIRED_PAGES_PENDING,         ExitCode::WARN},
        {ReasonCode::GPU_RECOVERY_ACTION_REQUIRED,  ExitCode::FAIL_STACK},
        {ReasonCode::FABRIC_NOT_READY,              ExitCode::FAIL_USABILITY},
        {ReasonCode::FABRIC_NOT_APPLICABLE,         ExitCode::OK},
    };

    for (const auto &row : rows) {
        CHECK(default_exit_code(row.code) == row.exit_code);
    }
}
