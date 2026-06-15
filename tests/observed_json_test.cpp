#include <catch2/catch_test_macros.hpp>
#include <gpu_qual/io_json.hpp>
#include <gpu_qual/observed.hpp>

#include <optional>
#include <string>

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
            {"nvidia_proc_driver_present", true},
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
            {"nvidia_proc_driver_present", false},
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

// ---- Ticket 005: per-GPU health and observed fabric ----

TEST_CASE("to_json serializes per-GPU health with the omit-when-unset / explicit-null rules") {
    gpu_qual::GpuInfo gpu{
        .index = 0,
        .name = "NVIDIA H100 80GB HBM3",
        .uuid = "GPU-...",
        .pci_bdf = "0000:1b:00.0",
        .memory_mib = 81559,
        .mig_mode = gpu_qual::MigMode::DISABLED,
    };

    SECTION("a fully populated health block matches the contract shape, all keys present") {
        gpu.health = gpu_qual::GpuHealth{
            .ecc_mode_enabled = true,
            .volatile_uncorrectable_ecc = 0,
            .aggregate_uncorrectable_ecc = 0,
            .row_remap_pending = false,
            .row_remap_failure = false,
            .pending_retired_pages = false,
            .recovery_action = gpu_qual::RecoveryAction::NONE,
        };

        const nlohmann::json expected_health = {
            {"ecc_mode_enabled", true},
            {"volatile_uncorrectable_ecc", 0},
            {"aggregate_uncorrectable_ecc", 0},
            {"row_remap_pending", false},
            {"row_remap_failure", false},
            {"pending_retired_pages", false},
            {"recovery_action", "none"},
        };

        const auto j = gpu_qual::to_json(gpu);
        REQUIRE(j.contains("health"));
        CHECK(j["health"] == expected_health);
    }

    SECTION("unset fields inside an engaged health block serialize as explicit null") {
        gpu.health = gpu_qual::GpuHealth{
            .ecc_mode_enabled = true,
            .volatile_uncorrectable_ecc = 0,
            .aggregate_uncorrectable_ecc = std::nullopt,
            .row_remap_pending = false,
            .row_remap_failure = false,
            .pending_retired_pages = false,
            .recovery_action = std::nullopt,
        };

        const auto j = gpu_qual::to_json(gpu);
        REQUIRE(j.contains("health"));
        // Every key is still present; only the unset optionals are null.
        CHECK(j["health"]["aggregate_uncorrectable_ecc"] == nullptr);
        CHECK(j["health"]["recovery_action"] == nullptr);
        CHECK(j["health"]["volatile_uncorrectable_ecc"] == 0);
        CHECK(j["health"]["ecc_mode_enabled"] == true);
    }

    SECTION("an unset health optional omits the health key entirely (include_health=false)") {
        gpu.health = std::nullopt;

        const auto j = gpu_qual::to_json(gpu);
        CHECK_FALSE(j.contains("health"));
    }
}

TEST_CASE("to_string(RecoveryAction) returns the exact lowercase string for every enumerator") {
    CHECK(std::string{gpu_qual::to_string(gpu_qual::RecoveryAction::NONE)} == "none");
    CHECK(std::string{gpu_qual::to_string(gpu_qual::RecoveryAction::RESET)} == "reset");
    CHECK(std::string{gpu_qual::to_string(gpu_qual::RecoveryAction::RESET_AND_DRAIN)} == "reset_and_drain");
    CHECK(std::string{gpu_qual::to_string(gpu_qual::RecoveryAction::REBOOT)} == "reboot");
    CHECK(std::string{gpu_qual::to_string(gpu_qual::RecoveryAction::FIELD_RMA)} == "field_rma");
}

TEST_CASE("to_json emits observed fabric only when set, with applicable always present") {
    gpu_qual::ObservedState observed{};

    SECTION("an engaged fabric block is emitted with applicable and ready") {
        observed.fabric = gpu_qual::FabricState{.applicable = true, .ready = true};

        const auto j = gpu_qual::to_json(observed);
        REQUIRE(j.contains("fabric"));
        CHECK(j["fabric"] == nlohmann::json{{"applicable", true}, {"ready", true}});
    }

    SECTION("ready is explicit null when not applicable / not read") {
        observed.fabric = gpu_qual::FabricState{.applicable = false, .ready = std::nullopt};

        const auto j = gpu_qual::to_json(observed);
        REQUIRE(j.contains("fabric"));
        CHECK(j["fabric"] == nlohmann::json{{"applicable", false}, {"ready", nullptr}});
    }

    SECTION("an unset fabric optional omits the fabric key entirely") {
        observed.fabric = std::nullopt;

        const auto j = gpu_qual::to_json(observed);
        CHECK_FALSE(j.contains("fabric"));
    }
}
