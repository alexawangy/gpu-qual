#include <gpu_qual/spec.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <gpu_qual/version.hpp>

#include <nlohmann/json.hpp>

namespace gpu_qual {
namespace {

SpecParseResult invalid(std::string field, std::string message) {
  return {
      .spec = std::nullopt,
      .error = make_reason(ReasonCode::EXPECTED_SPEC_INVALID, std::move(field), std::nullopt,
                           json(std::move(message))),
  };
}

std::optional<std::string> first_unknown_field(const json& object,
                                               std::initializer_list<std::string_view> allowed,
                                               std::string_view prefix = {}) {
  for (const auto& [key, value] : object.items()) {
    static_cast<void>(value);
    if (std::find(allowed.begin(), allowed.end(), key) == allowed.end()) {
      return std::string(prefix) + key;
    }
  }
  return std::nullopt;
}

bool read_positive_uint64(const json& value, std::uint64_t& out) {
  if (value.is_number_unsigned()) {
    out = value.get<std::uint64_t>();
    return out > 0;
  }

  if (value.is_number_integer()) {
    const auto signed_value = value.get<std::int64_t>();
    if (signed_value <= 0) {
      return false;
    }
    out = static_cast<std::uint64_t>(signed_value);
    return true;
  }

  if (value.is_number_float()) {
    const double floating_value = value.get<double>();
    if (!std::isfinite(floating_value) || floating_value <= 0.0 ||
        std::trunc(floating_value) != floating_value || floating_value >= std::ldexp(1.0, 64)) {
      return false;
    }
    out = static_cast<std::uint64_t>(floating_value);
    return true;
  }

  return false;
}

} // namespace

SpecParseResult parse_spec(std::string_view json_text) {
  json root;
  try {
    root = json::parse(json_text);
  } catch (const json::exception&) {
    return invalid("", "malformed JSON");
  }

  if (!root.is_object()) {
    return invalid("", "root must be a JSON object");
  }

  if (const auto field = first_unknown_field(root, {"schema_version", "expected"})) {
    return invalid(*field, "unknown field");
  }

  const auto schema_it = root.find("schema_version");
  if (schema_it == root.end()) {
    return invalid("schema_version", "missing required field");
  }
  if (!schema_it->is_string()) {
    return invalid("schema_version", "must be a string");
  }
  if (schema_it->get_ref<const std::string&>() != kSchemaVersion) {
    return invalid("schema_version", "unsupported schema version");
  }

  ExpectedSpec spec;
  const auto expected_it = root.find("expected");
  if (expected_it == root.end()) {
    return {.spec = std::move(spec), .error = std::nullopt};
  }
  if (!expected_it->is_object()) {
    return invalid("expected", "must be an object");
  }

  const json& expected = *expected_it;
  if (const auto field = first_unknown_field(
          expected, {"gpu_count", "allowed_names", "min_memory_mib", "min_driver_version"},
          "expected.")) {
    return invalid(*field, "unknown field");
  }

  if (const auto it = expected.find("gpu_count"); it != expected.end()) {
    std::uint64_t count = 0;
    if (!read_positive_uint64(*it, count)) {
      return invalid("expected.gpu_count", "must be a positive integer");
    }
    if (count > std::numeric_limits<unsigned>::max()) {
      return invalid("expected.gpu_count", "exceeds supported range");
    }
    spec.gpu_count = static_cast<unsigned>(count);
  }

  if (const auto it = expected.find("allowed_names"); it != expected.end()) {
    if (!it->is_array()) {
      return invalid("expected.allowed_names", "must be an array");
    }
    if (it->empty()) {
      return invalid("expected.allowed_names", "must contain at least one item");
    }

    for (const json& item : *it) {
      if (!item.is_string()) {
        return invalid("expected.allowed_names", "items must be strings");
      }

      std::string name = item.get<std::string>();
      if (name.empty()) {
        return invalid("expected.allowed_names", "items must be non-empty");
      }
      if (std::find(spec.allowed_names.begin(), spec.allowed_names.end(), name) !=
          spec.allowed_names.end()) {
        return invalid("expected.allowed_names", "items must be unique");
      }
      spec.allowed_names.push_back(std::move(name));
    }
  }

  if (const auto it = expected.find("min_memory_mib"); it != expected.end()) {
    std::uint64_t memory_mib = 0;
    if (!read_positive_uint64(*it, memory_mib)) {
      return invalid("expected.min_memory_mib", "must be a positive integer");
    }
    spec.min_memory_mib = memory_mib;
  }

  if (const auto it = expected.find("min_driver_version"); it != expected.end()) {
    if (!it->is_string()) {
      return invalid("expected.min_driver_version", "must be a string");
    }

    std::string version = it->get<std::string>();
    if (!is_valid_driver_version(version)) {
      return invalid("expected.min_driver_version",
                     "must contain dot-separated unsigned decimal components");
    }
    spec.min_driver_version = std::move(version);
  }

  return {.spec = std::move(spec), .error = std::nullopt};
}

} // namespace gpu_qual
