#pragma once

#include <compare>
#include <optional>
#include <string_view>

namespace gpu_qual {
inline constexpr const char* kToolVersion = "gpu-qual/0.1.0";
inline constexpr const char* kSchemaVersion = "0.1";

bool is_valid_driver_version(std::string_view version) noexcept;
std::optional<std::strong_ordering> compare_driver_versions(std::string_view observed,
                                                            std::string_view minimum) noexcept;
} // namespace gpu_qual
