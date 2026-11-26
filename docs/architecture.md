# Architecture: Engine Design & Core Subsystems

> **Notice**: The higher-level physics simulation models (cloth, collisions, rigid bodies) described in this document represent planned features. **Phase 1 core foundation modules (math, PRNG, arena allocator, state hashing, logging, status codes, and world skeleton) are implemented in `src/core/`.**

aPhysics is designed specifically to eliminate performance bottlenecks when streaming simulation data into Three.js inside Remotion video rendering pipelines.

---

## 1. [Planned] Zero-Copy Memory Sharing with Three.js

### The Problem
Traditional physics bridges serialize simulation state to JSON or copy large arrays across the WebAssembly-to-JavaScript boundary on every frame. When simulating cloth meshes with tens of thousands of vertices, copying buffers in JavaScript triggers heavy CPU usage and Garbage Collection (GC) pauses.

### The Planned Zero-Copy Architecture
WebAssembly memory is backed by a contiguous `ArrayBuffer`. aPhysics plans to allocate vertex buffers inside this memory and export the memory offset pointer:

```
+-----------------------------------------------------------+
| WebAssembly Linear Memory (ArrayBuffer)                   |
|                                                           |
|   +-----------------------------------------------------+ |
|   | [Planned] aPhysics Vertex Buffer                    | |
|   | [ x0, y0, z0, x1, y1, z1, ..., xN, yN, zN ]         | |
|   +-----------------------------------------------------+ |
|        ^                                                  |
+--------|--------------------------------------------------+
         | Direct Memory Pointer Offset
         |
+--------|--------------------------------------------------+
| Three.js / JavaScript Layer                               |
|                                                           |
|   const positions = new Float32Array(                     |
|     wasmMemory.buffer,                                    |
|     wasmInstance.exports.aphysics_get_positions(),        |
|     vertexCount * 3                                       |
|   );                                                      |
|                                                           |
|   geometry.setAttribute(                                  |
|     'position',                                           |
|     new THREE.BufferAttribute(positions, 3)               |
|   );                                                      |
|                                                           |
|   // Frame update in Remotion:                            |
|   wasmInstance.exports.aphysics_step(dt);                 |
|   geometry.attributes.position.needsUpdate = true;        |
+-----------------------------------------------------------+
         |
         v (Hardware copy over bus)
+-----------------------------------------------------------+
| GPU Vertex Buffer Object (VBO)                            |
| Uploaded by WebGL driver to GPU VRAM                      |
+-----------------------------------------------------------+
```

### Accurate Scope & Boundaries
- **No JavaScript-Side Copy**: Constructing a `Float32Array` view over `wasmMemory.buffer` allows JavaScript to read simulation vertex data without copying bytes in the JS runtime.
- **GPU Upload Is Still a Copy**: The transfer of vertex data from system RAM (WebAssembly memory) to the GPU VBO is a hardware memory copy across the internal bus. "No JavaScript-side copy" is accurate; "no copies at all" is not.
- **Memory Growth Handling**: If WebAssembly memory expands via `memory.grow()`, the previous `ArrayBuffer` becomes detached and existing typed array views are invalidated. The memory size must either be fixed upfront or views recreated upon growth.

---

## 2. [Planned] Deterministic Simulation via Seeds

For Remotion animation rendering, frame-accurate determinism is critical. Remotion renders frames concurrently across multiple worker processes or cloud renderers.

### Determinism Rules & Pure C Engine
1. **Pure C Engine (No Time Entropy)**:
   - The C core is strictly pure and deterministic. It **never** reads system clocks, timers, or OS entropy sources.
   - The caller must supply the 64-bit seed explicitly.
   - The JavaScript wrapper is responsible for turning "no seed" into a pseudo-random seed and returning it to the user so every run can be replayed identically.
2. **Deterministic PRNG**:
   - Internal perturbation functions (wrinkle noise, fabric tearing thresholds, structural crumple vectors) will rely on a deterministic 64-bit PRNG (e.g., SplitMix64 or PCG32).
3. **Fixed Seed Mode**:
   - Passing an explicit seed (e.g. `1337ULL`) guarantees that every simulation run produces the exact same vertex trajectories on every machine.
4. **Decoupled Fixed Delta Time ($\Delta t$)**:
   - The physics integrator calculates states at fixed time steps (e.g., $1/60\text{ s}$), preventing frame-rate fluctuations from altering the simulation outcome.
5. **State Hashing Verification**:
   - Simulation state will be hashed after $N$ steps to verify bit-level determinism between Linux and macOS CI runners, and between native binaries and WebAssembly builds.

---

## 3. Core Subsystems (Implemented Baseline)

The core foundation is implemented in `src/core/` and tested on Linux x86_64 (macOS builds remain experimental and unverified):

1. **Vector & Orientation Math (`src/core/aph_math.h`)**:
   - Single-precision 3D vector arithmetic (`aph_vec3`), quaternions (`aph_quat`), and 3x3 matrices (`aph_mat3`) in row-major order.
   - Vector normalization with sub-micrometre threshold guarding (`APH_VEC3_NORMALIZE_EPSILON_SQ = 1e-12f`) to prevent division by near-zero without suppressing NaNs.
   - Rodrigues quaternion vector rotation (`aph_quat_rotate`) and Hamilton quaternion product (`aph_quat_mul`).
2. **Deterministic PRNG (`src/core/aph_rng.h`)**:
   - 64-bit SplitMix64 generator based on David Stafford's Mix13 variant of MurmurHash3.
   - Exact power-of-two 24-bit mantissa scaling (`aph_rng_f32_from_u64`) producing single-precision floats strictly in the half-open interval $[0, 1)$.
   - Stateless 2D coordinate hashing (`aph_rand_hash(seed, step, element)`) for perturbation queries without mutable state sharing.
3. **Linear Memory Arena (`src/core/aph_arena.h`)**:
   - 64-byte aligned linear allocation (`aph_arena_alloc`) guaranteed compatible with AVX-512, ARM NEON, and WebAssembly SIMD128.
   - Zero-filled allocation and padding to ensure memory is never left undefined.
   - Zero-allocation policy during simulation steps (`aph_world_step`).
4. **State Hashing (`src/core/aph_hash.h`)**:
   - 64-bit Fowler-Noll-Vo (FNV-1a) byte hashing (`aph_hash_bytes`).
   - Bitwise floating-point divergence detection (e.g. discriminating $+0.0f$ from $-0.0f$ and 1-ULP numerical drift).
5. **Status Codes & ABI Stability (`src/core/aph_status.h`)**:
   - Standardized `aph_status` codes (`APH_OK`, `APH_ERR_INVALID_ARGUMENT`, `APH_ERR_OUT_OF_MEMORY`, etc.) with frozen numeric values.
6. **Diagnostic Logging (`src/core/aph_log.h`, `src/core/aph_log.c`)**:
   - Global log callback system with severity filtering (`APH_LOG_ERROR` through `APH_LOG_DEBUG`).
   - Freestanding safe formatting (`aph_log_format`) without C library dependencies, outputting standard format: `[aphysics] <LEVEL> <module>: <msg>`.
7. **Simulation World Skeleton (`src/core/aph_world.h`, `src/core/aph_world.c`)**:
   - Opaque `aph_world` handle managing contiguous host memory for the world struct and arena backing buffer.
   - Strict input validation on time-step (`fixed_dt <= 1.0s`), substep limits, and non-NaN gravity.
   - Little-endian discrete field state hashing (`aph_world_hash`) covering 36 bytes of state without struct padding.
