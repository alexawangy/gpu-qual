#pragma once

#include "observed.hpp"
#include "verdict.hpp"

#include <vector>

namespace gpu_qual {

struct ProbeOutcome {
  ObservedState observed;
  // Additional query failures not already represented by NvmlStatus. The
  // evaluation layer derives baseline load/init/access reasons from status.
  std::vector<Reason> reasons;
};

} // namespace gpu_qual
