#include <gpu_qual/observed.hpp>
#include <string_view>

std::string_view gpu_qual::to_string(gpu_qual::MigMode mig_mode) {
  switch (mig_mode) {
  case gpu_qual::MigMode::DISABLED: return "disabled";
  case gpu_qual::MigMode::ENABLED: return "enabled";
  }

  return "unknown";
}

std::string_view gpu_qual::to_string(gpu_qual::RecoveryAction ra) {
  switch (ra) {
  case RecoveryAction::NONE: return "none";
  case RecoveryAction::RESET: return "reset";
  case RecoveryAction::RESET_AND_DRAIN: return "reset_and_drain";
  case RecoveryAction::REBOOT: return "reboot";
  case RecoveryAction::FIELD_RMA: return "field_rma";
  }

  return "unknown";

  return "UNKNOWN";
}
