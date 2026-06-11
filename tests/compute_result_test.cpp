#include <catch2/catch_test_macros.hpp>
#include <gpu_qual/verdict.hpp>

#include <vector>

namespace {
    gpu_qual::Reason r(gpu_qual::ReasonCode code) {
        return {.code = code, .cls = gpu_qual::default_class(code)};
    }
}

TEST_CASE("compute_result maps empty reasons to correct success verdict") {
    const auto inv = gpu_qual::compute_result(gpu_qual::Mode::INVENTORY, {});
    CHECK(inv.verdict   == gpu_qual::Verdict::OBSERVED);
    CHECK(inv.exit_code == gpu_qual::ExitCode::OK);

    const auto chk = gpu_qual::compute_result(gpu_qual::Mode::CHECK, {});
    CHECK(chk.verdict   == gpu_qual::Verdict::PASS);
    CHECK(chk.exit_code == gpu_qual::ExitCode::OK);
}

TEST_CASE("compute_result selects highest-impact exit band") {
    const auto retry = gpu_qual::compute_result(
        gpu_qual::Mode::INVENTORY,
        {r(gpu_qual::ReasonCode::PROBE_TIMEOUT)}
    );
    CHECK(retry.verdict   == gpu_qual::Verdict::RETRY);
    CHECK(retry.exit_code == gpu_qual::ExitCode::RETRY);

    const auto fail = gpu_qual::compute_result(
        gpu_qual::Mode::CHECK,
        {
            r(gpu_qual::ReasonCode::PROBE_TIMEOUT),
            r(gpu_qual::ReasonCode::GPU_COUNT_MISMATCH),
            r(gpu_qual::ReasonCode::NVML_LIBRARY_NOT_FOUND),
        }
    );
    CHECK(fail.verdict   == gpu_qual::Verdict::FAIL);
    CHECK(fail.exit_code == gpu_qual::ExitCode::FAIL_STACK);
}
