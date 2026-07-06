#include <gpu_qual/backend.hpp>

#include <utility>

namespace gpu_qual {

MockBackend::MockBackend(ProbeOutcome outcome) : outcome_(std::move(outcome)) {}

ProbeOutcome MockBackend::probe() const {
  return outcome_;
}

} // namespace gpu_qual
