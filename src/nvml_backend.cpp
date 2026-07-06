#include <gpu_qual/backend.hpp>
#include <iostream>

namespace gpu_qual {

ProbeOutcome NvmlBackend::probe() const {
  std::cout << "(nvml): the backend is not implemented yet" << std::endl;
  return ProbeOutcome();
}

}
