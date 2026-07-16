#pragma once

#include "probe.hpp"
#include "spec.hpp"
#include "verdict.hpp"

namespace gpu_qual {
Result evaluate_inventory(ProbeOutcome outcome);
Result evaluate_check(ProbeOutcome outcome, const ExpectedSpec& spec);
} // namespace gpu_qual
