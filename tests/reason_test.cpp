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
