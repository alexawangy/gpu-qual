#pragma once

#include <gpu_qual/observed.hpp>
#include <gpu_qual/verdict.hpp>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace gpu_qual {
json to_json(const GpuHealth&);
json to_json(const FabricState&);
json to_json(const NvmlState&);
json to_json(const CudaState&);
json to_json(const GpuInfo&);
json to_json(const FallbackSignals&);
json to_json(const Result&);
json to_json(const ObservedState&);
} // namespace gpu_qual
