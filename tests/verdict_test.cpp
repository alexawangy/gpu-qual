#include <catch2/catch_test_macros.hpp>
#include <gpu_qual/verdict.hpp>

#include <string>

TEST_CASE("Mode to_string returns stable names") {
    CHECK(std::string{gpu_qual::to_string(gpu_qual::Mode::INVENTORY)} == "inventory");
    CHECK(std::string{gpu_qual::to_string(gpu_qual::Mode::CHECK)}     == "check");
}

TEST_CASE("Verdict to_string returns stable names") {
    CHECK(std::string{gpu_qual::to_string(gpu_qual::Verdict::OBSERVED)} == "observed");
    CHECK(std::string{gpu_qual::to_string(gpu_qual::Verdict::PASS)}     == "pass");
    CHECK(std::string{gpu_qual::to_string(gpu_qual::Verdict::WARN)}     == "warn");
    CHECK(std::string{gpu_qual::to_string(gpu_qual::Verdict::RETRY)}    == "retry");
    CHECK(std::string{gpu_qual::to_string(gpu_qual::Verdict::FAIL)}     == "fail");
}

TEST_CASE("ExitCode integer values match the routing bands") {
    using gpu_qual::ExitCode;
    CHECK(static_cast<int>(ExitCode::OK)             ==  0);
    CHECK(static_cast<int>(ExitCode::WARN)           == 10);
    CHECK(static_cast<int>(ExitCode::RETRY)          == 20);
    CHECK(static_cast<int>(ExitCode::FAIL_CONTRACT)  == 30);
    CHECK(static_cast<int>(ExitCode::FAIL_USABILITY) == 40);
    CHECK(static_cast<int>(ExitCode::FAIL_STACK)     == 50);
}
