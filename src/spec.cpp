#include "gpu_qual/observed.hpp"
#include "gpu_qual/verdict.hpp"
#include <algorithm>
#include <cctype>
#include <gpu_qual/spec.hpp>
#include <iostream>
#include <iterator>
#include <optional>
#include <string_view>

namespace gpu_qual {

// Used by spec parser to return invalid spr.
static SpecParseResult invalid(std::string_view field, std::string_view observed) {
  SpecParseResult spr;
  spr.reasons.push_back(
    make_reason(ReasonCode::EXPECTED_SPEC_INVALID,
                std::string(field),
                {},
                std::string(observed))
  );
  return spr;
}

static void add_unknown(SpecParseResult &spr, std::string_view field) {
  spr.reasons.push_back(
    make_reason(ReasonCode::UNKNOWN_FIELD_IGNORED,
                std::string(field),
                {},
                "unknown field ignored")
  );
}

namespace {
std::string sv_tolower(std::string_view sv) {
  std::string res;
  std::transform(sv.begin(), sv.end(), std::back_inserter(res),
                 [](unsigned char c) { return std::tolower(c); });
  return res;
}
} // namespace

std::string_view to_string(gpu_qual::Severity sev) {
  switch (sev) {
  case gpu_qual::Severity::HARD: return "hard";
  case gpu_qual::Severity::REPORT: return "report";
  case gpu_qual::Severity::WARN: return "warn";
  }

  return "unknown";
}

std::optional<Severity> severity_from_string(std::string_view sv) {
  std::string res = sv_tolower(sv);

  if (res == "hard")
    return Severity::HARD;
  if (res == "report")
    return Severity::REPORT;
  if (res == "warn")
    return Severity::WARN;

  return std::nullopt;
}

std::optional<MigExpectation> mig_expectation_from_string(std::string_view sv) {
  std::string res = sv_tolower(sv);

  if (res == "disabled")
    return MigExpectation::DISABLED;
  if (res == "enabled")
    return MigExpectation::ENABLED;
  if (res == "any")
    return MigExpectation::ANY;

  return std::nullopt;
}

std::optional<RecoveryAction> recovery_action_from_string(std::string_view sv) {
  std::string res = sv_tolower(sv);

  if (res == "none") return RecoveryAction::NONE;
  if (res == "reset") return RecoveryAction::RESET;
  if (res == "reset_and_drain") return RecoveryAction::RESET_AND_DRAIN;
  if (res == "field_rma") return RecoveryAction::FIELD_RMA;
  if (res == "reboot") return RecoveryAction::REBOOT;

  return std::nullopt;
}


SpecParseResult parse_spec(std::string_view json_text) {
  SpecParseResult spr = {
    .spec = std::nullopt,
    .reasons = {},
  };

  json obj;

  try {
    obj = json::parse(json_text);
  } catch (const json::parse_error &e) {
    std::cerr << "failed to parse spec, something went wrong" << std::endl;
    return invalid("", "failed to parse json");
  }

  if (!obj.is_object()) {
    std::cerr << "failed to populate spec object" << std::endl;
    return invalid("", "root must be JSON object");
  }

  for (const auto& [key, value] : obj.items()) {
    if (key != "schema_version" &&
        key != "metadata" &&
        key != "expected" &&
        key != "policy" &&
        key != "options") {
        add_unknown(spr, key);
      }
  }

  auto schema_it = obj.find("schema_version");

  if (schema_it == obj.end()) {
    return invalid("schema_version", "missing required field");
  }

  if (!schema_it->is_string()) {
    return invalid("schema_version", "must be string");
  }

  std::string schema_version = schema_it->get<std::string>();

  json metadata = nullptr;

  auto metadata_it = obj.find("metadata");

  if (metadata_it != obj.end()) {
    if (!metadata_it->is_object()) {
      return invalid("metadata", "must be object");
    }

    metadata = *metadata_it;
  }

  Expected expected{};
  Policy policy{};
  Options options{};

  
  

  return spr;
}

} // namespace gpu_qual
