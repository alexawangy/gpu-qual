#include "gpu_qual/observed.hpp"
#include "gpu_qual/verdict.hpp"
#include <algorithm>
#include <cctype>
#include <gpu_qual/spec.hpp>
#include <iterator>
#include <optional>
#include <string_view>

namespace gpu_qual {

// Used by spec parser to return invalid spr.
static SpecParseResult invalid(std::string_view field, std::string_view observed) {
  SpecParseResult spr;
  spr.reasons.push_back(make_reason(ReasonCode::EXPECTED_SPEC_INVALID, std::string(field), {},
                                    std::string(observed)));
  return spr;
}

static void add_unknown(SpecParseResult& spr, std::string_view field) {
  spr.reasons.push_back(make_reason(ReasonCode::UNKNOWN_FIELD_IGNORED, std::string(field), {},
                                    "unknown field ignored"));
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

std::string_view to_string(gpu_qual::MigExpectation mgexp) {
  switch (mgexp) {
  case MigExpectation::ANY: return "any";
  case MigExpectation::DISABLED: return "disabled";
  case MigExpectation::ENABLED: return "enabled";
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

  if (res == "none")
    return RecoveryAction::NONE;
  if (res == "reset")
    return RecoveryAction::RESET;
  if (res == "reset_and_drain")
    return RecoveryAction::RESET_AND_DRAIN;
  if (res == "field_rma")
    return RecoveryAction::FIELD_RMA;
  if (res == "reboot")
    return RecoveryAction::REBOOT;

  return std::nullopt;
}

// Validate a JSON value as an integer (rejecting floats such as 8.0) that is at
// least `min`. On success the value is written to `out`. On failure an engaged
// optional holding the EXPECTED_SPEC_INVALID result is returned.
static std::optional<SpecParseResult> require_int_min(const json& v, std::string_view path,
                                                      long long min, long long& out) {
  if (!v.is_number_integer()) {
    return invalid(path, "must be an integer");
  }
  long long n = v.get<long long>();
  if (n < min) {
    return invalid(path, "below minimum");
  }
  out = n;
  return std::nullopt;
}

static std::optional<SpecParseResult> require_bool(const json& v, std::string_view path,
                                                   std::optional<bool>& out) {
  if (!v.is_boolean()) {
    return invalid(path, "must be a boolean");
  }
  out = v.get<bool>();
  return std::nullopt;
}

static std::optional<SpecParseResult> require_severity(const json& v, std::string_view path,
                                                       Severity& out) {
  if (!v.is_string()) {
    return invalid(path, "must be a string");
  }
  auto sev = severity_from_string(v.get<std::string>());
  if (!sev) {
    return invalid(path, "must be one of hard/warn/report");
  }
  out = *sev;
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
  } catch (const json::parse_error&) {
    return invalid("", "malformed json");
  }

  if (!obj.is_object()) {
    return invalid("", "root must be a JSON object");
  }

  for (const auto& [key, value] : obj.items()) {
    if (key != "schema_version" && key != "metadata" && key != "expected" && key != "policy" &&
        key != "options") {
      add_unknown(spr, key);
    }
  }

  // schema_version (required)
  auto schema_it = obj.find("schema_version");
  if (schema_it == obj.end()) {
    return invalid("schema_version", "missing required field");
  }
  if (!schema_it->is_string()) {
    return invalid("schema_version", "must be a string");
  }
  std::string schema_version = schema_it->get<std::string>();

  // metadata (optional)
  json metadata = nullptr;
  if (auto it = obj.find("metadata"); it != obj.end()) {
    if (!it->is_object()) {
      return invalid("metadata", "must be an object");
    }
    if (it->dump().size() > kMaxMetadataBytes) {
      return invalid("metadata", "exceeds size cap");
    }
    metadata = *it;
  }

  Expected expected{};
  Policy policy{};
  Options options{};

  // expected
  if (auto exp_it = obj.find("expected"); exp_it != obj.end()) {
    if (!exp_it->is_object()) {
      return invalid("expected", "must be an object");
    }
    const json& e = *exp_it;

    for (const auto& [k, v] : e.items()) {
      if (k != "gpu_count" && k != "allowed_names" && k != "min_memory_mib" && k != "mig_mode" &&
          k != "min_cuda_visible_devices" && k != "min_driver_version" && k != "health" &&
          k != "fabric") {
        add_unknown(spr, "expected." + k);
      }
    }

    if (auto it = e.find("gpu_count"); it != e.end()) {
      long long n;
      if (auto err = require_int_min(*it, "expected.gpu_count", 1, n)) {
        return *err;
      }
      expected.gpu_count = static_cast<int>(n);
    }

    if (auto it = e.find("min_memory_mib"); it != e.end()) {
      long long n;
      if (auto err = require_int_min(*it, "expected.min_memory_mib", 1, n)) {
        return *err;
      }
      expected.min_memory_mib = n;
    }

    // null is accepted as "unset" to match the contract example.
    if (auto it = e.find("min_cuda_visible_devices"); it != e.end() && !it->is_null()) {
      long long n;
      if (auto err = require_int_min(*it, "expected.min_cuda_visible_devices", 1, n)) {
        return *err;
      }
      expected.min_cuda_visible_devices = static_cast<int>(n);
    }

    if (auto it = e.find("allowed_names"); it != e.end()) {
      if (!it->is_array()) {
        return invalid("expected.allowed_names", "must be an array");
      }
      if (it->empty()) {
        return invalid("expected.allowed_names", "must have at least one item");
      }
      std::vector<std::string> names;
      for (const auto& item : *it) {
        if (!item.is_string()) {
          return invalid("expected.allowed_names", "items must be strings");
        }
        std::string s = item.get<std::string>();
        if (s.empty()) {
          return invalid("expected.allowed_names", "items must be non-empty");
        }
        if (std::find(names.begin(), names.end(), s) != names.end()) {
          return invalid("expected.allowed_names", "items must be unique");
        }
        names.push_back(std::move(s));
      }
      expected.allowed_names = std::move(names);
    }

    if (auto it = e.find("mig_mode"); it != e.end()) {
      if (!it->is_string()) {
        return invalid("expected.mig_mode", "must be a string");
      }
      auto m = mig_expectation_from_string(it->get<std::string>());
      if (!m) {
        return invalid("expected.mig_mode", "must be one of enabled/disabled/any");
      }
      expected.mig_mode = *m;
    }

    if (auto it = e.find("min_driver_version"); it != e.end()) {
      if (!it->is_string()) {
        return invalid("expected.min_driver_version", "must be a string");
      }
      expected.min_driver_version = it->get<std::string>();
    }

    // expected.health
    if (auto h_it = e.find("health"); h_it != e.end()) {
      if (!h_it->is_object()) {
        return invalid("expected.health", "must be an object");
      }
      const json& h = *h_it;

      for (const auto& [k, v] : h.items()) {
        if (k != "ecc_mode_enabled" && k != "max_volatile_uncorrectable_ecc" &&
            k != "max_aggregate_uncorrectable_ecc" && k != "allow_row_remap_pending" &&
            k != "allow_row_remap_failure" && k != "allow_pending_retired_pages" &&
            k != "disallowed_recovery_actions") {
          add_unknown(spr, "expected.health." + k);
        }
      }

      if (auto it = h.find("ecc_mode_enabled"); it != h.end()) {
        if (auto err = require_bool(*it, "expected.health.ecc_mode_enabled",
                                    expected.health.ecc_mode_enabled)) {
          return *err;
        }
      }
      if (auto it = h.find("allow_row_remap_pending"); it != h.end()) {
        if (auto err = require_bool(*it, "expected.health.allow_row_remap_pending",
                                    expected.health.allow_row_remap_pending)) {
          return *err;
        }
      }
      if (auto it = h.find("allow_row_remap_failure"); it != h.end()) {
        if (auto err = require_bool(*it, "expected.health.allow_row_remap_failure",
                                    expected.health.allow_row_remap_failure)) {
          return *err;
        }
      }
      if (auto it = h.find("allow_pending_retired_pages"); it != h.end()) {
        if (auto err = require_bool(*it, "expected.health.allow_pending_retired_pages",
                                    expected.health.allow_pending_retired_pages)) {
          return *err;
        }
      }

      if (auto it = h.find("max_volatile_uncorrectable_ecc"); it != h.end()) {
        long long n;
        if (auto err =
                require_int_min(*it, "expected.health.max_volatile_uncorrectable_ecc", 0, n)) {
          return *err;
        }
        expected.health.max_volatile_uncorrectable_ecc = n;
      }

      // integer >= 0 or JSON null; null leaves the optional unset.
      if (auto it = h.find("max_aggregate_uncorrectable_ecc"); it != h.end() && !it->is_null()) {
        long long n;
        if (auto err =
                require_int_min(*it, "expected.health.max_aggregate_uncorrectable_ecc", 0, n)) {
          return *err;
        }
        expected.health.max_aggregate_uncorrectable_ecc = n;
      }

      if (auto it = h.find("disallowed_recovery_actions"); it != h.end()) {
        if (!it->is_array()) {
          return invalid("expected.health.disallowed_recovery_actions", "must be an array");
        }
        std::vector<RecoveryAction> actions;
        for (const auto& item : *it) {
          if (!item.is_string()) {
            return invalid("expected.health.disallowed_recovery_actions", "items must be strings");
          }
          auto a = recovery_action_from_string(item.get<std::string>());
          if (!a) {
            return invalid("expected.health.disallowed_recovery_actions",
                           "unknown recovery action");
          }
          if (std::find(actions.begin(), actions.end(), *a) != actions.end()) {
            return invalid("expected.health.disallowed_recovery_actions", "items must be unique");
          }
          actions.push_back(*a);
        }
        expected.health.disallowed_recovery_actions = std::move(actions);
      }
    }

    // expected.fabric
    if (auto f_it = e.find("fabric"); f_it != e.end()) {
      if (!f_it->is_object()) {
        return invalid("expected.fabric", "must be an object");
      }
      const json& f = *f_it;

      for (const auto& [k, v] : f.items()) {
        if (k != "require_fabric_ready") {
          add_unknown(spr, "expected.fabric." + k);
        }
      }

      if (auto it = f.find("require_fabric_ready"); it != f.end()) {
        if (auto err = require_bool(*it, "expected.fabric.require_fabric_ready",
                                    expected.fabric.require_fabric_ready)) {
          return *err;
        }
      }
    }
  }

  // policy
  if (auto p_it = obj.find("policy"); p_it != obj.end()) {
    if (!p_it->is_object()) {
      return invalid("policy", "must be an object");
    }
    for (const auto& [k, v] : p_it->items()) {
      Severity* slot = nullptr;
      if (k == "gpu_count")
        slot = &policy.gpu_count;
      else if (k == "gpu_name")
        slot = &policy.gpu_name;
      else if (k == "memory")
        slot = &policy.memory;
      else if (k == "mig_mode")
        slot = &policy.mig_mode;
      else if (k == "cuda_visible_devices")
        slot = &policy.cuda_visible_devices;
      else if (k == "driver_version")
        slot = &policy.driver_version;
      else if (k == "ecc")
        slot = &policy.ecc;
      else if (k == "row_remap")
        slot = &policy.row_remap;
      else if (k == "retired_pages")
        slot = &policy.retired_pages;
      else if (k == "recovery_action")
        slot = &policy.recovery_action;
      else if (k == "fabric")
        slot = &policy.fabric;
      else if (k == "cuda_smoke")
        slot = &policy.cuda_smoke;

      if (slot == nullptr) {
        add_unknown(spr, "policy." + k);
        continue;
      }
      if (auto err = require_severity(v, "policy." + k, *slot)) {
        return *err;
      }
    }
  }

  // options
  if (auto o_it = obj.find("options"); o_it != obj.end()) {
    if (!o_it->is_object()) {
      return invalid("options", "must be an object");
    }
    for (const auto& [k, v] : o_it->items()) {
      if (k == "cuda_smoke" || k == "include_health") {
        std::optional<bool> b;
        if (auto err = require_bool(v, "options." + k, b)) {
          return *err;
        }
        (k == "cuda_smoke" ? options.cuda_smoke : options.include_health) = *b;
      } else if (k == "timeout_ms" || k == "cuda_smoke_timeout_ms") {
        long long n;
        if (auto err = require_int_min(v, "options." + k, 1, n)) {
          return *err;
        }
        (k == "timeout_ms" ? options.timeout_ms : options.cuda_smoke_timeout_ms) =
            static_cast<int>(n);
      } else {
        add_unknown(spr, "options." + k);
      }
    }
  }

  ExpectedSpec spec;
  spec.schema_version = std::move(schema_version);
  spec.metadata = std::move(metadata);
  spec.expected = std::move(expected);
  spec.policy = policy;
  spec.options = options;
  spr.spec = std::move(spec);

  return spr;
}

} // namespace gpu_qual
