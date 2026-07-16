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

// Loads libnvidia-ml.so.1 at runtime and collects physical GPU identity.
ProbeOutcome probe_nvml();

// Explicit local/demo path. The returned values are conspicuously simulated;
// production callers should use probe_nvml().
ProbeOutcome probe_simulated_nvml();

} // namespace gpu_qual
