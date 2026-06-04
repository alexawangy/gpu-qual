#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "version.hpp"

namespace gpu_qual {
  enum class Verdict { PASS, PASS_WITH_WARNINGS, FAIL, QUARANTINE };
  enum class ExitCode : int { PASS=0, PASS_WITH_WARNINGS=10, FAIL=20, QUARANTINE_CONTRACT=30, QUARANTINE_USABILITY=40, STACK_ABSENT=50 };

  // Add as we go
  enum class ReasonCode {};

  std::string_view to_string(Verdict);
  std::string_view to_string(ReasonCode);

  struct Result {
    std::string tool_version = kToolVersion;
    std::string schema_version = kSchemaVersion;
    Verdict verdit = Verdict::QUARANTINE;
    ExitCode exit_code = ExitCode::STACK_ABSENT;
    std::vector<ReasonCode> reasons;
  };
}
