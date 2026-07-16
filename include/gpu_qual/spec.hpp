#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <gpu_qual/verdict.hpp>

namespace gpu_qual {

struct ExpectedSpec {
  std::optional<unsigned> gpu_count;
  std::vector<std::string> allowed_names;
  std::optional<std::uint64_t> min_memory_mib;
  std::optional<std::string> min_driver_version;
};

struct SpecParseResult {
  std::optional<ExpectedSpec> spec;
  std::optional<Reason> error;

  [[nodiscard]] bool ok() const noexcept {
    return spec.has_value();
  }
};

SpecParseResult parse_spec(std::string_view json_text);

} // namespace gpu_qual
