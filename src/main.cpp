#include <iostream>

#include <gpu_qual/io_json.hpp>
#include <gpu_qual/pipeline.hpp>

int main() {
  const gpu_qual::Result result = gpu_qual::evaluate_inventory(gpu_qual::ProbeOutcome{});
  std::cout << gpu_qual::to_json(result).dump(2) << std::endl;
  return static_cast<int>(result.exit_code);
}
