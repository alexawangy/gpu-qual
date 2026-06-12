#pragma once

#include <gpu_qual/verdict.hpp>

#include <nlohmann/json.hpp>

namespace gpu_qual {
  nlohmann::json to_json(const Result&);
}
