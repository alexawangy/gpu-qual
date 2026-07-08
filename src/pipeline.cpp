#include <gpu_qual/pipeline.hpp>
#include <gpu_qual/reconcile.hpp>

#include <iterator>
#include <utility>

namespace gpu_qual {

Result evaluate_inventory(ProbeOutcome outcome) {
  Result result = compute_result(Mode::INVENTORY, std::move(outcome.reasons));
  result.observed = std::move(outcome.observed);
  return result;
}

Result evaluate_check(ProbeOutcome outcome, const ExpectedSpec& spec,
                      std::vector<Reason> spec_reasons) {
  outcome.reasons.insert(outcome.reasons.end(), std::make_move_iterator(spec_reasons.begin()),
                         std::make_move_iterator(spec_reasons.end()));

  std::vector<Reason> reconciliation_reasons = reconcile(spec, outcome.observed);
  outcome.reasons.insert(outcome.reasons.end(),
                         std::make_move_iterator(reconciliation_reasons.begin()),
                         std::make_move_iterator(reconciliation_reasons.end()));

  Result result = compute_result(Mode::CHECK, std::move(outcome.reasons));
  result.observed = std::move(outcome.observed);
  return result;
}

} // namespace gpu_qual
