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

TEST_CASE("compute_result folds contract health and fabric reason codes") {
    const auto stack = gpu_qual::compute_result(
        gpu_qual::Mode::CHECK,
        {
            r(gpu_qual::ReasonCode::ROW_REMAP_FAILURE),
            r(gpu_qual::ReasonCode::ECC_MODE_MISMATCH),
        }
    );
    CHECK(stack.verdict   == gpu_qual::Verdict::FAIL);
    CHECK(stack.exit_code == gpu_qual::ExitCode::FAIL_STACK);

    const auto usability = gpu_qual::compute_result(
        gpu_qual::Mode::CHECK,
        {
            r(gpu_qual::ReasonCode::FABRIC_NOT_READY),
            r(gpu_qual::ReasonCode::GPU_COUNT_MISMATCH),
        }
    );
    CHECK(usability.verdict   == gpu_qual::Verdict::FAIL);
    CHECK(usability.exit_code == gpu_qual::ExitCode::FAIL_USABILITY);

    const auto not_applicable = gpu_qual::compute_result(
        gpu_qual::Mode::INVENTORY,
        {r(gpu_qual::ReasonCode::FABRIC_NOT_APPLICABLE)}
    );
    CHECK(not_applicable.verdict   == gpu_qual::Verdict::OBSERVED);
    CHECK(not_applicable.exit_code == gpu_qual::ExitCode::OK);
    REQUIRE(not_applicable.reasons.size() == 1);
    CHECK(not_applicable.reasons[0].code == gpu_qual::ReasonCode::FABRIC_NOT_APPLICABLE);

    CHECK(gpu_qual::make_reason(gpu_qual::ReasonCode::DRIVER_VERSION_BELOW_MIN).cls ==
          gpu_qual::ReasonClass::WARN);
}
