#pragma once

#include <compare>
#include <gpu_qual/observed.hpp>
#include <gpu_qual/spec.hpp>
#include <gpu_qual/verdict.hpp>
#include <string_view>
#include <vector>

namespace gpu_qual {
std::vector<Reason> reconcile(const ExpectedSpec&, const ObservedState&);
ReasonClass to_reason_class(Severity);
std::vector<unsigned> parse_version(std::string_view);
std::strong_ordering compare_driver_versions(std::string_view observed, std::string_view min_req);
} // namespace gpu_qual
