#include <iostream>

#include <gpu_qual/io_json.hpp>

int main() {
  std::cout << gpu_qual::to_json(gpu_qual::compute_result(gpu_qual::Mode::INVENTORY, {})).dump(2) << std::endl;
  return 0;
}
