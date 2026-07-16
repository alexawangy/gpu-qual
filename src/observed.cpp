#include <gpu_qual/observed.hpp>

namespace gpu_qual {
std::string_view to_string(NvmlStatus status) {
  switch (status) {
  case NvmlStatus::NOT_PROBED: return "not_probed";
  case NvmlStatus::LIBRARY_NOT_FOUND: return "library_not_found";
  case NvmlStatus::INITIALIZATION_FAILED: return "initialization_failed";
  case NvmlStatus::NO_PERMISSION: return "no_permission";
  case NvmlStatus::READY: return "ready";
  }

  return "unknown";
}
} // namespace gpu_qual
