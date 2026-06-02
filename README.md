# gpu-qual

**Status: Under progress**

`gpu-qual` is a lightweight C++20 GPU instance contract tester for the allocation
pipeline. It is intended to run on a freshly provisioned GPU instance before the
instance is assigned to a client account.

It checks whether the instance matches the expected lease contract: GPU count,
SKU/family, memory floor, MIG mode, topology policy, driver/NVML availability,
and optional CUDA usability. It is not a hardware diagnostics or stress tool;
deep health checks remain DCGM's job.

## What We Have Now

- C++20 project skeleton.
- CMake + Ninja presets for dev, strict, release, and relwithdebinfo builds.
- Convenience `Makefile`.
- Placeholder `gpu-qual` binary in `src/main.cpp`.
- Basic Catch2 test target stub.

The NVML backend, CUDA smoke test, contract JSON parsing, verdict engine, and
DCGM escalation are planned but not implemented yet.

## Setup

Install build tools.

macOS:

```sh
brew install cmake ninja
```

Ubuntu:

```sh
sudo apt update
sudo apt install -y build-essential cmake ninja-build
```

Build and test:

```sh
make dev
```

Run:

```sh
make run
```

Install to `./dist`:

```sh
make install PRESET=relwithdebinfo
```

## Useful Commands

```sh
make list      # list CMake presets
make strict    # warnings-as-errors dev build
make release   # release build + tests
make clean     # remove generated build outputs
```
