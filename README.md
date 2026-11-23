# aPhysics

A physics engine for Remotion and Three.js animations. It bakes in cloth movement, crash dynamics, deformation, and physical constraints so that animations appear natural and dynamic instead of static.

---

## Features

- **Cloth & Soft-Body Movement**: Simulates realistic drape, wrinkles, tearing, and motion for fabric in 3D scenes.
- **Crash & Impact Mechanics**: Calculates deformation, fracture, and collisions for dynamic animation sequences.
- **Remotion & Three.js Integration**: Designed specifically for web-based animation pipelines and programmatic rendering.
- **Deterministic Simulation via Seeds**:
  - **Reproducible Seeds**: Set a 64-bit seed (e.g. `1337`) to produce identical simulation trajectories, crumple patterns, and cloth folds across runs (ideal for frame-accurate Remotion video rendering).
  - **Randomized Seeds**: Omit the seed or pass `0` to generate a fresh, unique simulation run each time.
- **Zero-Copy Memory Sharing with Three.js**: Direct WebAssembly linear memory mapping allows Three.js `BufferAttribute` to read simulation vertex data without copying memory buffers between C/Wasm and JavaScript.

---

## Technical Stack & Requirements

- **Language**: C (C17)
- **Compiler**: Clang (version 23 or higher). *GCC is not supported.*
- **Build System**: Bazel (version 9 or higher, pinned via `.bazelversion`). *Makefiles are not used.*
- **License**: Apache 2.0

---

## Architecture: Zero-Copy Memory Sharing

When simulating complex meshes (e.g., thousands of vertices for cloth or debris), serializing or copying buffer data between WebAssembly and JavaScript causes frame-rate degradation.

aPhysics maps vertex buffers directly to Three.js:

1. **Contiguous Buffer**: The C engine allocates vertex coordinates in contiguous linear memory.
2. **Pointer Export**: The engine exports memory pointers (e.g. `aphysics_get_positions()`).
3. **TypedArray View**: JavaScript instantiates a `Float32Array` view directly into `WebAssembly.Memory.buffer` at the pointer offset.
4. **Three.js Binding**: The `Float32Array` is bound directly into `THREE.BufferAttribute`.
5. **GPU Upload**: On each simulation step in C, setting `geometry.attributes.position.needsUpdate = true` instructs WebGL to upload the buffer directly to the GPU without intermediate copies.

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

---

## License

This project is licensed under the Apache License 2.0. See [LICENSE](LICENSE) and [NOTICE](NOTICE) for details.
