#include <catch2/catch_test_macros.hpp>
#include <gpu_qual/io_json.hpp>

TEST_CASE("io_json header is self-contained") {
    [[maybe_unused]] gpu_qual::Result result{
        .mode = gpu_qual::Mode::INVENTORY,
        .verdict = gpu_qual::Verdict::OBSERVED,
        .exit_code = gpu_qual::ExitCode::OK,
    };
    [[maybe_unused]] nlohmann::json json = nlohmann::json::object();

    SUCCEED();
}
