# gpu-qual

**Status: active development — core qualification logic is implemented; hardware probing is not.**

`gpu-qual` is a small, provider-agnostic GPU node qualification probe. It runs
locally on an NVIDIA GPU node, answers one question — *is the GPU stack
present, the GPUs I expected, and healthy enough to hand off?* — then emits
stable JSON and a stable exit code, and exits.

It has two modes:

- **Inventory** (default) — observe and report the local GPU stack. No input needed.
- **Check** — compare the observed stack against a caller-supplied expected
  spec ([schemas/contract_schema.json](schemas/contract_schema.json)), including health gates.

The binary knows nothing about cloud providers, instance shapes, regions, or
reservations — callers own all of that.

## Exit codes

| Exit | Verdict | Meaning |
|---:|---|---|
| 0 | `observed` / `pass` | Inventory succeeded, or check passed. |
| 10 | `warn` | Check passed with warnings. |
| 20 | `retry` | Transient probe failure or timeout — try again. |
| 30 | `fail` | Expected-spec mismatch (identity or health). |
| 40 | `fail` | CUDA usability or fabric readiness failed. |
| 50 | `fail` | GPU stack absent or inaccessible, invalid input, or probe infrastructure failure. |

Detailed reason codes accompany every non-zero result in the JSON output.

## Current state

Implemented: C++20 data model, spec parsing, reconciliation, result routing,
JSON serialization, tests, and build/release tooling.

Next: the real CLI, fake and NVML backends, CUDA smoke, and supervision/timeouts.

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
