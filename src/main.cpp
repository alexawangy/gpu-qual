#include <iostream>
#include <ostream>
#include <nlohmann/json.hpp>
#include <string_view>

int main() {
  std::string_view msg{"alexander", 4};
  std::cout << msg << std::endl;
  return 0;
}
