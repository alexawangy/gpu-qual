#pragma once

#include <gpu_qual/observed.hpp>
#include <gpu_qual/verdict.hpp>

#include <nlohmann/json.hpp>

namespace gpu_qual {
json to_json(const Result&);
json to_json(const ObservedState&);
} // namespace gpu_qual
