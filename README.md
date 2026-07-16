# gpu-qual

**Status: early development — the identity contract is implemented and tested;
the real NVML probe is not implemented yet.**

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

Implemented: the identity-only observed model and check contract, strict spec
parsing, identity reconciliation, stable result JSON and exit-code routing,
pure inventory/check evaluation, CMake + Ninja builds, and unit tests.

Not yet implemented: dynamic NVML loading and identity collection, supervised
child execution, production CLI wiring, real-node validation, and capability
expansion such as MIG or health signals.

Until NVML collection is connected, the binary fails closed with
`PROBE_OUTPUT_INVALID` and exit code `50`; it does not report a synthetic
successful inventory.

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
