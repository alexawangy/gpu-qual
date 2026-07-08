#pragma once

#include "backend.hpp"
#include "spec.hpp"
#include "verdict.hpp"

#include <vector>

namespace gpu_qual {
Result evaluate_inventory(ProbeOutcome outcome);
Result evaluate_check(ProbeOutcome outcome, const ExpectedSpec& spec,
                      std::vector<Reason> spec_reasons = {});
} // namespace gpu_qual
