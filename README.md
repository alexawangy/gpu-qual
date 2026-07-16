# gpu-qual

**Status: early development — physical GPU identity inventory is implemented
with a runtime-loaded NVML probe and an explicit simulator.**

`gpu-qual` is a small, provider-agnostic GPU node qualification probe. Its first
slice answers one question: *is the NVIDIA stack available, and are the physical
GPUs I expected present?* It emits stable JSON and a stable exit code, then exits.

It has two modes:

- **Inventory** (default) — observe and report the local GPU stack. No input needed.
- **Check** — compare observed GPU identity against a caller-supplied expected
  spec ([schemas/contract_schema.json](schemas/contract_schema.json)).

The binary knows nothing about cloud providers, instance shapes, regions, or
reservations — callers own all of that.

## Exit codes

| Exit | Verdict | Meaning |
|---:|---|---|
| 0 | `observed` / `pass` | Inventory succeeded, or check passed. |
| 10 | `warn` | Reserved for future advisory checks. |
| 20 | `retry` | Reserved for the supervised timeout/retry path. |
| 30 | `fail` | Expected identity does not match the observed node. |
| 40 | `fail` | Reserved for a future GPU-usability probe. |
| 50 | `fail` | GPU stack absent or inaccessible, or input is invalid. |

Detailed reason codes accompany every non-zero result in the JSON output.

## Current state

Implemented: runtime NVML loading, physical GPU identity collection, an
injectable NVML function table, explicit simulation, the identity-only observed
model and check contract, strict spec parsing, reconciliation, stable result
JSON and exit-code routing, CMake + Ninja builds, and unit tests.

Not yet implemented: check-mode CLI input, supervised child execution,
real-node validation, and capability expansion such as MIG or health signals.

The binary never links against NVML at build time. It loads
`libnvidia-ml.so.1` at runtime, so compilation needs neither an NVIDIA GPU nor
the NVML development headers. A missing library, failed initialization,
permission error, or required-query failure is reported in JSON and fails
closed with exit code `50`.

## Build

Install build tools — macOS: `brew install cmake ninja`; Ubuntu:
`sudo apt install -y build-essential cmake ninja-build`.

```sh
make dev        # build + test (default preset)
make run        # build and run the binary
make strict     # warnings-as-errors build
make release    # release build + tests
make install PRESET=relwithdebinfo   # install to ./dist
make list       # list CMake presets
make clean      # remove build outputs
```

## Run

On a Linux NVIDIA node, run the real probe without a flag:

```sh
make release
./build/release/gpu-qual > result.json
exit_code=$?
python3 -m json.tool result.json
printf 'gpu-qual exit: %s\n' "$exit_code"
```

The NVIDIA driver must expose the runtime SONAME `libnvidia-ml.so.1`. You can
compare the inventory with:

```sh
nvidia-smi --query-gpu=index,name,uuid,pci.bus_id,memory.total \
  --format=csv,noheader,nounits
```

macOS has no NVML runtime. Use the explicit simulator to exercise the complete
collection and JSON pipeline locally:

```sh
make strict
./build/strict/gpu-qual --simulate
```

Simulation is never selected automatically. It prints a warning to stderr and
uses conspicuous `SIMULATED` identity values. Running without `--simulate` on a
Mac truthfully returns `NVML_LIBRARY_NOT_FOUND` and exit code `50`.
