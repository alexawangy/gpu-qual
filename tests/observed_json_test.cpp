#include <catch2/catch_test_macros.hpp>
#include <gpu_qual/io_json.hpp>
#include <gpu_qual/observed.hpp>

#include <optional>
#include <string>

using namespace gpu_qual;

TEST_CASE("to_json serializes observed state with the fixed contract shape") {
    SECTION("a populated state, with no health or fabric assessed") {
        const ObservedState observed{
            .nvml = {.init_ok = true, .available = true, .driver_version = "550.144.03"},
            .cuda = {.available = std::nullopt, .device_count = std::nullopt,
                     .smoke_ran = false, .smoke_passed = std::nullopt},
            .gpus = {{
                .index = 0,
                .name = "NVIDIA H100 80GB HBM3",
                .uuid = "GPU-...",
                .pci_bdf = "0000:1b:00.0",
                .memory_mib = 81559,
                .mig_mode = MigMode::DISABLED,
            }},
            .fallback = {.nvidia_device_nodes_present = true,
                         .nvidia_proc_driver_present = true,
                         .nvidia_pci_devices_present = true},
        };

        const nlohmann::json expected = {
            {"nvml", {{"available", true}, {"init_ok", true}, {"driver_version", "550.144.03"}}},
            {"cuda", {{"available", nullptr}, {"visible_device_count", nullptr},
                      {"smoke_ran", false}, {"smoke_passed", nullptr}}},
            {"gpu_count", 1},
            {"gpus", {{
                {"index", 0},
                {"uuid", "GPU-..."},
                {"pci_bdf", "0000:1b:00.0"},
                {"name", "NVIDIA H100 80GB HBM3"},
                {"memory_mib", 81559},
                {"mig_mode", "disabled"},
            }}},
            {"fallback", {{"nvidia_device_nodes_present", true},
                          {"nvidia_proc_driver_present", true},
                          {"nvidia_pci_devices_present", true}}},
        };

        CHECK(to_json(observed) == expected);
    }

    SECTION("a default state keeps the shape with explicit nulls, no health/fabric keys") {
        const nlohmann::json expected = {
            {"nvml", {{"available", false}, {"init_ok", false}, {"driver_version", nullptr}}},
            {"cuda", {{"available", nullptr}, {"visible_device_count", nullptr},
                      {"smoke_ran", false}, {"smoke_passed", nullptr}}},
            {"gpu_count", 0},
            {"gpus", nlohmann::json::array()},
            {"fallback", {{"nvidia_device_nodes_present", false},
                          {"nvidia_proc_driver_present", false},
                          {"nvidia_pci_devices_present", false}}},
        };

        CHECK(to_json(ObservedState{}) == expected);
    }

    SECTION("observed appears on a result only when the optional is set") {
        auto result = compute_result(Mode::INVENTORY, {});
        CHECK_FALSE(to_json(result).contains("observed"));

        result.observed = ObservedState{};
        const auto j = to_json(result);
        REQUIRE(j.contains("observed"));
        CHECK(j["observed"]["gpu_count"] == 0);
    }
}

TEST_CASE("to_json serializes per-GPU health: omit when unset, explicit null when engaged") {
    GpuInfo gpu{
        .index = 0,
        .name = "NVIDIA H100 80GB HBM3",
        .uuid = "GPU-...",
        .pci_bdf = "0000:1b:00.0",
        .memory_mib = 81559,
        .mig_mode = MigMode::DISABLED,
    };

    SECTION("a fully populated health block matches the contract shape") {
        gpu.health = GpuHealth{
            .ecc_mode_enabled = true,
            .volatile_uncorrectable_ecc = 0,
            .aggregate_uncorrectable_ecc = 0,
            .row_remap_pending = false,
            .row_remap_failure = false,
            .pending_retired_pages = false,
            .recovery_action = RecoveryAction::NONE,
        };

        REQUIRE(to_json(gpu).contains("health"));
        CHECK(to_json(gpu)["health"] == nlohmann::json{
            {"ecc_mode_enabled", true},
            {"volatile_uncorrectable_ecc", 0},
            {"aggregate_uncorrectable_ecc", 0},
            {"row_remap_pending", false},
            {"row_remap_failure", false},
            {"pending_retired_pages", false},
            {"recovery_action", "none"},
        });
    }

    SECTION("unset fields inside an engaged block serialize as explicit null") {
        gpu.health = GpuHealth{
            .ecc_mode_enabled = true,
            .volatile_uncorrectable_ecc = 0,
            .aggregate_uncorrectable_ecc = std::nullopt,
            .recovery_action = std::nullopt,
        };

        const auto health = to_json(gpu)["health"];
        CHECK(health["ecc_mode_enabled"] == true);
        CHECK(health["volatile_uncorrectable_ecc"] == 0);
        CHECK(health["aggregate_uncorrectable_ecc"] == nullptr);
        CHECK(health["recovery_action"] == nullptr);
    }

    SECTION("an unset health optional omits the key entirely (include_health=false)") {
        gpu.health = std::nullopt;
        CHECK_FALSE(to_json(gpu).contains("health"));
    }
}

TEST_CASE("to_json serializes observed fabric: omit when unset, explicit null ready") {
    ObservedState observed{};

    SECTION("an engaged fabric block emits applicable and ready") {
        observed.fabric = FabricState{.applicable = true, .ready = true};
        REQUIRE(to_json(observed).contains("fabric"));
        CHECK(to_json(observed)["fabric"] == nlohmann::json{{"applicable", true}, {"ready", true}});
    }

    SECTION("ready is explicit null when not applicable / not read") {
        observed.fabric = FabricState{.applicable = false, .ready = std::nullopt};
        CHECK(to_json(observed)["fabric"] == nlohmann::json{{"applicable", false}, {"ready", nullptr}});
    }

    SECTION("an unset fabric optional omits the key entirely") {
        observed.fabric = std::nullopt;
        CHECK_FALSE(to_json(observed).contains("fabric"));
    }
}

TEST_CASE("to_string(RecoveryAction) covers every enumerator") {
    CHECK(std::string{to_string(RecoveryAction::NONE)}            == "none");
    CHECK(std::string{to_string(RecoveryAction::RESET)}           == "reset");
    CHECK(std::string{to_string(RecoveryAction::RESET_AND_DRAIN)} == "reset_and_drain");
    CHECK(std::string{to_string(RecoveryAction::REBOOT)}          == "reboot");
    CHECK(std::string{to_string(RecoveryAction::FIELD_RMA)}       == "field_rma");
}
