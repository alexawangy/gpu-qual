#pragma once

#include <gpu_qual/observed.hpp>
#include <gpu_qual/spec.hpp>
#include <gpu_qual/verdict.hpp>
#include <vector>

namespace gpu_qual {
std::vector<Reason> reconcile(const ExpectedSpec&, const ObservedState&);
} // namespace gpu_qual
