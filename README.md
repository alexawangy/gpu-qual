# gpu-qual

GPU qualification utilities.

## Requirements

- CMake 3.20 or newer
- Ninja
- A C++20 compiler

On macOS with Homebrew:

```sh
brew install cmake ninja
```

On Ubuntu:

```sh
sudo apt update
sudo apt install -y build-essential cmake ninja-build
```

## Local Build Flow

The repo includes a convenience `Makefile` that wraps the CMake preset commands.
For normal development, run:

```sh
make dev
```

That configures the `dev` preset, builds, runs CTest, and links
`compile_commands.json` into the repo root for editors.

List the available presets:

```sh
make list
```

Configure a debug development build:

```sh
cmake --preset dev
```

Build it:

```sh
cmake --build --preset dev
```

Run it:

```sh
./build/dev/gpu-qual
```

Run tests:

```sh
ctest --preset dev
```

You can still call CMake directly at any time; the Makefile is only a shortcut.

## Presets

- `dev`: debug build for normal development.
- `strict`: debug build with warnings treated as errors.
- `relwithdebinfo`: optimized build with debug symbols, useful for Linux binaries you may need to diagnose later.
- `release`: optimized release build.

Example release-style build:

```sh
cmake --preset relwithdebinfo
cmake --build --preset relwithdebinfo
ctest --preset relwithdebinfo
```

Install into a local staging directory:

```sh
cmake --install build/relwithdebinfo --prefix "$PWD/dist"
```

The installed executable will be:

```sh
./dist/bin/gpu-qual
```

## Linux Binaries

macOS builds produce macOS binaries. For Ubuntu GPU machines, build on Ubuntu:

```sh
cmake --preset relwithdebinfo
cmake --build --preset relwithdebinfo
ctest --preset relwithdebinfo
```

When GPU-specific dependencies are added later, prefer building final release
artifacts on the target Linux distribution or in an Ubuntu container/CI image
that matches the deployment environment. Cross-compiling can be added later with
a toolchain file and `CMakeUserPresets.json`, while keeping project-wide presets
portable.
