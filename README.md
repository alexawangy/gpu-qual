# gpu-qual

**Status: active development**

`gpu-qual` inventories NVIDIA GPU identity and reports stable JSON and an exit
code.

It has two modes:

- **Inventory** (default): observe and report the local GPU stack. No input needed.
- **Check**: compare observed GPU identity against a caller-supplied expected
  spec ([schemas/contract_schema.json](schemas/contract_schema.json)).

The binary knows nothing about cloud providers, instance shapes, regions, or
reservations. Callers own those details.

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

Implemented: runtime NVML identity inventory, explicit simulation, spec parsing,
reconciliation, JSON output, and tests.

Next: check-mode CLI input, supervised execution, real-node validation, and
additional GPU signals.

NVML is loaded at runtime, so builds require neither a GPU nor NVML headers.

## Build

Install build tools. macOS: `brew install cmake ninja`; Ubuntu:
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
