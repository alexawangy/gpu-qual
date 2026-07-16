#include <iostream>
#include <string_view>
#include <utility>

#include <gpu_qual/io_json.hpp>
#include <gpu_qual/pipeline.hpp>

namespace {
void print_usage(std::ostream& output) {
  output << "usage: gpu-qual [--simulate]\n"
            "  --simulate  use conspicuous deterministic GPU data instead of loading NVML\n";
}
} // namespace

int main(int argc, char* argv[]) {
  bool simulate = false;
  if (argc == 2 && std::string_view(argv[1]) == "--simulate") {
    simulate = true;
  } else if (argc == 2 &&
             (std::string_view(argv[1]) == "--help" || std::string_view(argv[1]) == "-h")) {
    print_usage(std::cout);
    return 0;
  } else if (argc != 1) {
    print_usage(std::cerr);
    return static_cast<int>(gpu_qual::ExitCode::FAIL_STACK);
  }

  if (simulate) {
    std::cerr << "warning: using simulated NVML data\n";
  }

  gpu_qual::ProbeOutcome outcome =
      simulate ? gpu_qual::probe_simulated_nvml() : gpu_qual::probe_nvml();
  const gpu_qual::Result result = gpu_qual::evaluate_inventory(std::move(outcome));
  std::cout << gpu_qual::to_json(result).dump(2) << std::endl;
  return static_cast<int>(result.exit_code);
}
