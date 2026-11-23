# aPhysics

[![CI](https://github.com/Aarav90-cpu/aPhysics/actions/workflows/ci.yml/badge.svg)](https://github.com/Aarav90-cpu/aPhysics/actions/workflows/ci.yml)
[![CodeQL](https://github.com/Aarav90-cpu/aPhysics/actions/workflows/codeql.yml/badge.svg)](https://github.com/Aarav90-cpu/aPhysics/actions/workflows/codeql.yml)
[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](LICENSE)
[![Compiler](https://img.shields.io/badge/Clang-23%2B-red.svg)](https://clang.llvm.org/)
[![Build System](https://img.shields.io/badge/Bazel-9%2B-green.svg)](https://bazel.build/)
[![Language](https://img.shields.io/badge/Language-C17-blue.svg)](https://en.wikipedia.org/wiki/C17_(C_standard_revision))
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20macOS%20(Experimental)-orange.svg)](#supported-platforms)

A physics engine for Remotion and Three.js animations aiming to bake in cloth movement, crash dynamics, deformation, and physical constraints so that animations appear natural, dynamic, and lifelike instead of static.

---

## Current Status

> **Notice**: The repository is currently in the foundational infrastructure phase. The physics engine components listed below are planned architectural features and are actively under design.

- [x] Bazel 9 build setup (`9.2.0` pinned in `.bazelversion`)
- [x] Clang 23+ toolchain configuration in `.bazelrc`
- [x] Cross-platform GitHub Actions CI (Ubuntu & macOS runners) with `clang-format` and `cppcheck`
- [ ] *[Planned]* Point-mass & spring-mass integration baseline (Milestone 1)
- [ ] *[Planned]* Zero-Copy WebAssembly linear memory sharing with Three.js
- [ ] *[Planned]* Deterministic 64-bit seed reproduction for Remotion
- [ ] *[Planned]* Cloth simulation & soft-body drape mechanics
- [ ] *[Planned]* Crash deformation & fracture mechanics

---

## Planned Architecture & Roadmap

For full design specifications, see [docs/architecture.md](docs/architecture.md).

- **[Planned] Zero-Copy Memory Sharing with Three.js**:
  Mapping C vertex buffers directly in WebAssembly linear memory to Three.js `BufferAttribute` via `Float32Array` views, avoiding per-frame memory copies.
- **[Planned] Deterministic Simulation via Seeds**:
  - *Fixed Seeds (e.g. `1337`)*: Frame-exact, bit-reproducible video rendering across distributed Remotion rendering workers.
  - *Randomized Seeds (`0`)*: Dynamic, unique animation trajectories for interactive previews.
- **[Planned] Cloth & Soft-Body Movement**:
  Simulating realistic drape, wrinkles, tearing, and motion for fabric in 3D scenes.
- **[Planned] Crash & Impact Dynamics**:
  Calculating deformation and fracture for dynamic collisions in animation sequences.

---

## Technical Stack & Requirements

- **Language**: C (C17 standard)
- **Compiler**: Clang (version 23 or higher). *GCC is not supported.*
- **Build System**: Bazel (version 9 or higher, pinned via `.bazelversion`). *Makefiles are not used.*
- **License**: Apache 2.0

---

## Supported Platforms

- **Linux**: Primary development and tested platform. Fully supported.
- **macOS**: Experimental and currently untested on physical hardware. See [docs/macos_testers.md](docs/macos_testers.md).

---

## Build Commands

All builds and tests are managed through Bazel 9:

### Build

```bash
bazel build //...
```

### Run

```bash
bazel run //:app
```

### Test

```bash
bazel test //...
```

### Clean

```bash
bazel clean
```

For setup instructions and prerequisites, see [docs/getting_started.md](docs/getting_started.md).

---

## Documentation

- [Getting Started Guide](docs/getting_started.md)
- [Planned Architecture: Zero-Copy & Seeds](docs/architecture.md)
- [Looking for macOS Testers!](docs/macos_testers.md)
- [Agent Skills & Tool Integration](docs/agent_skills.md)
- [Web Documentation Portal](website/index.html) (or [index.md](index.md))

---

## License

This project is licensed under the Apache License 2.0. See [LICENSE](LICENSE) and [NOTICE](NOTICE) for details.
