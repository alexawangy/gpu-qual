#include <catch2/catch_test_macros.hpp>
#include <gpu_qual/io_json.hpp>
#include <gpu_qual/observed.hpp>

#include <optional>

TEST_CASE("to_json serializes a populated observed state with the fixed contract shape") {
    gpu_qual::ObservedState observed{
        .nvml = {
            .init_ok = true,
            .available = true,
            .driver_version = "550.144.03",
        },
        .cuda = {
            .available = std::nullopt,
            .device_count = std::nullopt,
            .smoke_ran = false,
            .smoke_passed = std::nullopt,
        },
        .gpus = {
            {
                .index = 0,
                .name = "NVIDIA H100 80GB HBM3",
                .uuid = "GPU-...",
                .pci_bdf = "0000:1b:00.0",
                .memory_mib = 81559,
                .mig_mode = gpu_qual::MigMode::DISABLED,
            },
        },
        .fallback = {
            .nvidia_device_nodes_present = true,
            .nvidia_proc_driver_present = true,
            .nvidia_pci_devices_present = true,
        },
    };

    const nlohmann::json expected = {
        {"nvml", {
            {"available", true},
            {"init_ok", true},
            {"driver_version", "550.144.03"},
        }},
        {"cuda", {
            {"available", nullptr},
            {"visible_device_count", nullptr},
            {"smoke_ran", false},
            {"smoke_passed", nullptr},
        }},
        {"gpu_count", 1},
        {"gpus", {{
            {"index", 0},
            {"uuid", "GPU-..."},
            {"pci_bdf", "0000:1b:00.0"},
            {"name", "NVIDIA H100 80GB HBM3"},
            {"memory_mib", 81559},
            {"mig_mode", "disabled"},
        }}},
        {"fallback", {
            {"nvidia_device_nodes_present", true},
            {"proc_driver_nvidia_present", true},
            {"nvidia_pci_devices_present", true},
        }},
    };

    CHECK(gpu_qual::to_json(observed) == expected);
}

TEST_CASE("to_json serializes default observed state without omitting nullable fields") {
    const gpu_qual::ObservedState observed{};

    const nlohmann::json expected = {
        {"nvml", {
            {"available", false},
            {"init_ok", false},
            {"driver_version", nullptr},
        }},
        {"cuda", {
            {"available", nullptr},
            {"visible_device_count", nullptr},
            {"smoke_ran", false},
            {"smoke_passed", nullptr},
        }},
        {"gpu_count", 0},
        {"gpus", nlohmann::json::array()},
        {"fallback", {
            {"nvidia_device_nodes_present", false},
            {"proc_driver_nvidia_present", false},
            {"nvidia_pci_devices_present", false},
        }},
    };

    CHECK(gpu_qual::to_json(observed) == expected);
}

TEST_CASE("to_json emits observed on a result only when the optional value is set") {
    auto result = gpu_qual::compute_result(gpu_qual::Mode::INVENTORY, {});
    result.observed = gpu_qual::ObservedState{};

    const auto j = gpu_qual::to_json(result);

    REQUIRE(j.contains("observed"));
    CHECK(j["observed"]["gpu_count"] == 0);
    CHECK(j["observed"]["gpus"] == nlohmann::json::array());
}
