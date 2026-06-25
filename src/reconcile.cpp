#include "gpu_qual/spec.hpp"
#include "gpu_qual/verdict.hpp"
#include <algorithm>
#include <charconv>
#include <compare>
#include <cstddef>
#include <gpu_qual/reconcile.hpp>
#include <iterator>
#include <ranges>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <vector>

namespace gpu_qual {
std::vector<Reason> reconcile(const ExpectedSpec& expected, const ObservedState& observed) {
  std::vector<Reason> reasons = {};

  return {};
}

ReasonClass to_reason_class(Severity sev) {
  switch (sev) {
  case Severity::HARD: return ReasonClass::HARD;
  case Severity::WARN: return ReasonClass::WARN;
  case Severity::REPORT: return ReasonClass::REPORT;
  }
}

std::vector<unsigned> parse_version(std::string_view version) {
  std::vector<unsigned> parts;

  for (auto part : version | std::views::split('.')) {
    unsigned value = 0;

    auto begin = part.begin();

    const char* first = &*begin;
    const char* last = first + std::ranges::distance(part);

    auto [ptr, ec] = std::from_chars(first, last, value);

    if (ec != std::errc{} || ptr != last) {
      throw std::invalid_argument("Invalid version component");
    }

    parts.push_back(value);
  }

  return parts;
}

std::strong_ordering compare_driver_versions(std::string_view observed, std::string_view min_req) {
  auto left = parse_version(observed);
  auto right = parse_version(min_req);

  const auto n = std::max(left.size(), right.size());

  for (size_t i{0}; i < n; ++i) {
    unsigned l = i < left.size() ? left[i] : 0;
    unsigned r = i < right.size() ? right[i] : 0;

    if (l < r)
      return std::strong_ordering::less;
    if (l > r)
      return std::strong_ordering::greater;
  }

  return std::strong_ordering::equal;
}
} // namespace gpu_qual
