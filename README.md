# aPhysics

[![CI](https://github.com/Aarav90-cpu/aPhysics/actions/workflows/ci.yml/badge.svg)](https://github.com/Aarav90-cpu/aPhysics/actions/workflows/ci.yml)
[![CodeQL](https://github.com/Aarav90-cpu/aPhysics/actions/workflows/codeql.yml/badge.svg)](https://github.com/Aarav90-cpu/aPhysics/actions/workflows/codeql.yml)
[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](LICENSE)
[![Compiler](https://img.shields.io/badge/Clang-23%2B-red.svg)](https://clang.llvm.org/)
[![Build System](https://img.shields.io/badge/Bazel-9%2B-green.svg)](https://bazel.build/)
[![Language](https://img.shields.io/badge/Language-C17-blue.svg)](https://en.wikipedia.org/wiki/C17_(C_standard_revision))
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20macOS%20(Experimental)-orange.svg)](#supported-platforms)

A physics engine for Remotion and Three.js animations. It bakes in cloth movement, crash dynamics, deformation, and physical constraints so that animations appear natural, dynamic, and lifelike instead of static.

---

## Features

- **Cloth & Soft-Body Movement**: Simulates realistic drape, wrinkles, tearing, and motion for fabric in 3D scenes.
- **Crash & Impact Mechanics**: Calculates deformation, fracture, and collisions for dynamic animation sequences.
- **Remotion & Three.js Integration**: Designed specifically for web-based animation pipelines and programmatic rendering.
- **Deterministic Simulation via Seeds**:
  - **Reproducible Seeds**: Set a 64-bit seed (e.g., `1337`) to produce identical simulation trajectories, crumple patterns, and cloth folds across runs (ideal for frame-accurate Remotion video rendering).
  - **Randomized Seeds**: Omit the seed or pass `0` to generate a fresh, unique simulation run each time.
- **Zero-Copy Memory Sharing with Three.js**: Direct WebAssembly linear memory mapping allows Three.js `BufferAttribute` to read simulation vertex data without copying memory buffers between C/Wasm and JavaScript.

---

## Technical Stack & Requirements

- **Language**: C (C17 standard)
- **Compiler**: Clang (version 23 or higher). *GCC is not supported.*
- **Build System**: Bazel (version 9 or higher, pinned via `.bazelversion`). *Makefiles are not used.*
- **License**: Apache 2.0

---

## Supported Platforms

- **Linux**: Primary development and tested platform. Fully supported.
- **macOS**: Experimental and currently untested. We are actively looking for macOS testers! See [docs/macos_testers.md](docs/macos_testers.md).

---

## Architecture: Zero-Copy Memory Sharing

When simulating complex meshes (e.g., thousands of vertices for cloth or debris), serializing or copying buffer data between WebAssembly and JavaScript causes frame-rate degradation.

aPhysics maps vertex buffers directly to Three.js:

1. **Contiguous Buffer**: The C engine allocates vertex coordinates in contiguous linear memory.
2. **Pointer Export**: The engine exports memory pointers (e.g., `aphysics_get_positions()`).
3. **TypedArray View**: JavaScript instantiates a `Float32Array` view directly into `WebAssembly.Memory.buffer` at the pointer offset.
4. **Three.js Binding**: The `Float32Array` is bound directly into `THREE.BufferAttribute`.
5. **GPU Upload**: On each simulation step in C, setting `geometry.attributes.position.needsUpdate = true` instructs WebGL to upload the buffer directly to the GPU without intermediate copies.

For in-depth architectural details, see [docs/architecture.md](docs/architecture.md).

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
- [Architecture & Zero-Copy Memory](docs/architecture.md)
- [Looking for macOS Testers!](docs/macos_testers.md)
- [Agent Skills & Tool Integration](docs/agent_skills.md)
- [Web Documentation Portal](website/index.html) (or [index.md](index.md))

---

## Hashtags

`#aPhysics` `#Remotion` `#ThreeJS` `#WebGL` `#WebAssembly` `#Wasm` `#PhysicsEngine` `#ClothSimulation` `#CrashPhysics` `#CreativeCoding` `#IndieDev` `#OpenSource` `#Clang23` `#BazelBuild` `#C17` `#ZeroCopy` `#GameDev` `#Simulation` `#AnimationDev` `#LinuxDev`

---

## License

This project is licensed under the Apache License 2.0. See [LICENSE](LICENSE) and [NOTICE](NOTICE) for details.
