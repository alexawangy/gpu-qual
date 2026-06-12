#include <gpu_qual/observed.hpp>
#include <string_view>

std::string_view gpu_qual::to_string(gpu_qual::MigMode mig_mode) {
  switch (mig_mode) {
    case gpu_qual::MigMode::DISABLED:
      return "disabled";
    case gpu_qual::MigMode::ENABLED:
      return "enabled";
  }

  return "unknown";
}
