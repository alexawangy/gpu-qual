#pragma once

#include "observed.hpp"
#include "verdict.hpp"
#include <concepts>
#include <string_view>
#include <vector>

namespace gpu_qual {
struct ProbeOutcome {
  ObservedState observed;
  std::vector<Reason> reasons;
};

template <typename B>
concept Backend = requires(const B& b) {
  { b.probe() } -> std::same_as<ProbeOutcome>;
  { b.name() } -> std::convertible_to<std::string_view>;
};

class MockBackend {
public:
  explicit MockBackend(ProbeOutcome outcome);

  ProbeOutcome probe() const;
  std::string_view name() const noexcept {
    return "mock";
  }

private:
  ProbeOutcome outcome_;
};

static_assert(Backend<MockBackend>);

class NvmlBackend {
public:
  ProbeOutcome probe() const;
  std::string_view name() const {
    return "nvml";
  };
};

static_assert(Backend<NvmlBackend>);

} // namespace gpu_qual
