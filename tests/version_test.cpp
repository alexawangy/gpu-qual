#include <catch2/catch_test_macros.hpp>
#include <gpu_qual/version.hpp>

#include <string>

TEST_CASE("version constants are correct") {
    CHECK(std::string{gpu_qual::kToolVersion} == "gpu-qual/0.1.0");
    CHECK(std::string{gpu_qual::kSchemaVersion} == "0.1");
}
