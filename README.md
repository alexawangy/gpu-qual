# gpu-qual

**Status: Under progress**

`gpu-qual` is a lightweight, provider-agnostic GPU node inventory and
qualification probe. It runs locally on an NVIDIA GPU node and emits stable JSON
describing whether the GPU stack is present, visible, and minimally usable.

The default mode is inventory-only. Optional validation mode compares the
observed GPU stack with a small caller-supplied expected spec. Cloud providers,
instance shapes, reservations, regions, and provider SDKs stay outside this
binary.

## Contract

The public MVP contract is schema version `0.3` and tool version
`gpu-qual/0.3.0`.

- Inventory success returns verdict `observed` and exit code `0`.
- Validation success returns verdict `pass` and exit code `0`.
- Validation warnings return verdict `warn` and exit code `10`.
- Transient probe failures return verdict `retry` and exit code `20`.
- Contract mismatches return verdict `fail` and exit code `30`.
- Required CUDA smoke failures return verdict `fail` and exit code `40`.
- Missing, inaccessible, invalid, or crashed probe infrastructure returns verdict
  `fail` and exit code `50`.

See [docs/contract-v0.3.md](docs/contract-v0.3.md) for the frozen MVP contract.

## What We Have Now

- C++20 project skeleton.
- CMake + Ninja presets for dev, strict, release, and relwithdebinfo builds.
- Convenience `Makefile`.
- Placeholder `gpu-qual` binary in `src/main.cpp`.
- v0.3 version, verdict, exit-code, reason-code, and result-routing tests.

The expected-spec parser, reconciler, JSON output, fake backend, supervisor,
NVML inventory backend, and optional CUDA smoke path are planned but not
implemented yet.

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
