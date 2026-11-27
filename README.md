# aPhysics

[![CI](https://github.com/Aarav90-cpu/aPhysics/actions/workflows/ci.yml/badge.svg)](https://github.com/Aarav90-cpu/aPhysics/actions/workflows/ci.yml)
[![CodeQL](https://github.com/Aarav90-cpu/aPhysics/actions/workflows/codeql.yml/badge.svg)](https://github.com/Aarav90-cpu/aPhysics/actions/workflows/codeql.yml)
[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](LICENSE)
[![Compiler](https://img.shields.io/badge/Clang-23%2B-red.svg)](https://clang.llvm.org/)
[![Build System](https://img.shields.io/badge/Bazel-9%2B-green.svg)](https://bazel.build/)
[![Language](https://img.shields.io/badge/Language-C17-blue.svg)](https://en.wikipedia.org/wiki/C17_(C_standard_revision))
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20macOS%20(Experimental)-orange.svg)](#supported-platforms)

A physics engine for Remotion and Three.js animations aiming to bake in cloth movement, crash dynamics, deformation, and physical constraints so that animations appear natural, dynamic, and lifelike instead of static.

> **Status: pre-alpha (Phase 2 in progress). See [docs/architecture.md](docs/architecture.md) for implemented subsystems.**

---

## Current Status

The build/CI foundation and Phase 1 C core utilities are in place, and Phase 2 (native cloth) is in progress. See [docs/architecture.md](docs/architecture.md) for implemented module details.

- [x] Bazel 9 build system (`9.2.0` pinned in `.bazelversion`)
- [x] Clang 23+ compiler toolchain configuration in `.bazelrc`
- [x] C17 standard enforcement
- [x] GitHub Actions CI (`clang-format`, `cppcheck`, ASan/UBSan on Ubuntu; experimental macOS build job)
- [x] Basic test harness structure in `BUILD.bazel`
- [x] Phase 1 Core & Phase 2 Step 2.1 Cloth Particles — see [docs/architecture.md](docs/architecture.md)

---

## Planned Features

The physics engine is in active design. All simulation features below are planned and not yet implemented:

- **[Planned] Milestone 1**: Single point-mass and damped spring-mass integrator core with test suite.
- **[Planned] Zero-Copy Memory Views**: Direct WebAssembly linear memory mapping with `Float32Array` views to eliminate JavaScript-side copies when streaming to Three.js `BufferAttribute`.
- **[Planned] Seeded Determinism**: Caller-provided 64-bit seed (e.g. `1337`) producing bit-identical simulation states across distributed Remotion workers.
- **[Planned] Cloth Simulation**: Position-based dynamics (PBD) for fabric drape, wrinkles, shear/bend constraints, and tearing.
- **[Planned] Crash & Impact Dynamics**: Collision detection, structural collapse, deformation, and fracture mechanics.
- **[Planned] Bake Engine**: Simulating once forward and caching per-frame vertex buffers for random-access playback by frame number in Remotion.

For the detailed design specification, see [docs/architecture.md](docs/architecture.md) and [agentrules/NOTE.md](agentrules/NOTE.md).

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
bazel run //tools:aphysics_info
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
- [Architecture: Engine Design & Core Subsystems](docs/architecture.md)
- [Looking for macOS Testers!](docs/macos_testers.md)
- [Agent Skills & Tool Integration](docs/agent_skills.md)
- [Key Design Decisions](agentrules/NOTE.md)
- [Naming & Layout Rules](agentrules/NamingRules.md)

---

## License

This project is licensed under the Apache License 2.0. See [LICENSE](LICENSE) and [NOTICE](NOTICE) for details.
