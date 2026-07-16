#include <gpu_qual/version.hpp>

#include <string_view>

namespace gpu_qual {
namespace {

std::string_view take_component(std::string_view& version) noexcept {
  const std::size_t separator = version.find('.');
  if (separator == std::string_view::npos) {
    const std::string_view component = version;
    version = {};
    return component;
  }

  const std::string_view component = version.substr(0, separator);
  version.remove_prefix(separator + 1);
  return component;
}

std::string_view normalize_component(std::string_view component) noexcept {
  const std::size_t first_nonzero = component.find_first_not_of('0');
  if (first_nonzero == std::string_view::npos) {
    return component.substr(component.size() - 1);
  }
  return component.substr(first_nonzero);
}

std::strong_ordering compare_components(std::string_view left, std::string_view right) noexcept {
  left = normalize_component(left);
  right = normalize_component(right);

  if (left.size() < right.size()) {
    return std::strong_ordering::less;
  }
  if (left.size() > right.size()) {
    return std::strong_ordering::greater;
  }
  if (left < right) {
    return std::strong_ordering::less;
  }
  if (left > right) {
    return std::strong_ordering::greater;
  }
  return std::strong_ordering::equal;
}

} // namespace

bool is_valid_driver_version(std::string_view version) noexcept {
  if (version.empty() || version.front() == '.' || version.back() == '.') {
    return false;
  }

  bool previous_was_separator = false;
  for (const char character : version) {
    if (character == '.') {
      if (previous_was_separator) {
        return false;
      }
      previous_was_separator = true;
      continue;
    }

    if (character < '0' || character > '9') {
      return false;
    }
    previous_was_separator = false;
  }

  return true;
}

std::optional<std::strong_ordering> compare_driver_versions(std::string_view observed,
                                                            std::string_view minimum) noexcept {
  if (!is_valid_driver_version(observed) || !is_valid_driver_version(minimum)) {
    return std::nullopt;
  }

  while (!observed.empty() || !minimum.empty()) {
    const std::string_view observed_component =
        observed.empty() ? std::string_view{"0"} : take_component(observed);
    const std::string_view minimum_component =
        minimum.empty() ? std::string_view{"0"} : take_component(minimum);

    const std::strong_ordering comparison =
        compare_components(observed_component, minimum_component);
    if (comparison != std::strong_ordering::equal) {
      return comparison;
    }
  }

  return std::strong_ordering::equal;
}

} // namespace gpu_qual
