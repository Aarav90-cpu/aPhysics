# Key Design Decisions

> **Important**: Read this document before designing or implementing simulation features.

---

## 1. Remotion Needs Random Access to Frames

Remotion renders each video frame as a pure function of the frame number ($f(\text{frame})$), and renders frames concurrently across separate browser tabs/workers. A standard iterative physics loop (`step() -> step() -> step()`) does not natively fit a random-access rendering model.

### Execution Modes
1. **Bake (Primary Mode)**:
   - Run the simulation forward once from frame 0 to frame $N$.
   - Store the computed vertex positions for every frame in memory/cache.
   - Play back and sample the geometry directly by frame index during rendering.
   - This matches the project's purpose of "baking in" physical animation.
2. **Re-simulate (Secondary Mode)**:
   - Each rendering worker simulates from frame 0 up to the specific frame it needs to render.
   - Guaranteed deterministic, but exhibits $O(n^2)$ cumulative render overhead for longer sequences. Offer this as an optional fallback.

### Memory Sizing & Budgeting for Baked Meshes
- **10,000 vertices**:
  - $10{,}000\text{ vertices} \times 3\text{ floats} \times 4\text{ bytes} = 120\text{ KB per frame}$.
  - $10\text{ seconds} @ 60\text{ fps} = 600\text{ frames} \approx 72\text{ MB}$.
- **Optimization Strategy**:
  - Keep vertex counts conservative for cloth simulations.
  - Support 30 fps baking with linear interpolation between keyframes.
  - Plan for 16-bit quantized vertex coordinates to halve memory requirements where acceptable.

*Prerequisite*: Consult the official Remotion documentation on concurrency and rendering workflows before implementing Phase 4.

---

## 2. Accurate Definition of "Zero-Copy"

1. **JavaScript & WebAssembly**:
   - WebAssembly memory can be viewed directly from JavaScript without memory copying using typed array views (e.g., `new Float32Array(wasmMemory.buffer, ptr, length)`).
   - *Accuracy Note*: "No JavaScript-side copy" is accurate. "No copies at all" is not.
2. **WebAssembly Memory Growth**:
   - If WebAssembly memory expands dynamically via `memory.grow()`, the previous `ArrayBuffer` becomes detached/invalidated. Every existing `Float32Array` view becomes detached and throws on access.
   - *Requirement*: Either fix total memory size at build/initialization time, or implement a mechanism to recreate/refresh typed array views whenever memory growth occurs.
3. **GPU Uploads**:
   - The transfer of vertex data from system RAM to the GPU VBO (via `THREE.BufferAttribute` and WebGL) remains a hardware memory copy across the PCIe / internal bus.

---

## 3. Determinism: Guarantees & Constraints

Physics state can be deterministic across machines, provided strict numerical constraints are followed in the C core:

### Core Rules
- **Fixed Time-Step**: The simulation must always advance by a fixed $\Delta t$, regardless of rendering frame rate.
- **Fixed Iteration Order**: Constraint resolution and particle iteration order must be strictly deterministic. Never depend on raw pointer addresses or non-deterministic hash maps.
- **Compiler Flags**:
  - Do not use `-ffast-math`.
  - Enforce `-ffp-contract=off` to prevent differing fused multiply-add (FMA) hardware instructions across distinct CPU architectures.
- **Standard Math Functions**:
  - Results from standard math library calls (`sin`, `cos`, `pow`, `sqrt`) can vary slightly across different standard C library implementations (e.g., glibc, musl, macOS libSystem).
  - Test numerical consistency or provide dedicated cross-platform approximations where exact bit-level matching is required.
- **Pure C Engine**:
  - The C physics core must never query system clocks, time functions, or system entropy devices.
  - The caller must supply the seed explicitly.
  - The JavaScript wrapper is responsible for turning "no seed" into a pseudo-random seed and returning it to the user so any simulation run can be reproduced.
- **Verification via State Hashing**:
  - Test determinism by hashing the entire simulation state (e.g. via xxHash or SHA-256) after $N$ simulation steps.
  - Assert that this hash matches identically across runs, between Linux and macOS CI runners, and between native binaries and WebAssembly targets.
- **Scope of Guarantee**:
  - Determinism applies strictly to the **physics numerical state** (positions, velocities, constraints).
  - Rendered pixels (WebGL, Three.js, GPU drivers, browser compositors) may still exhibit minor rasterization variances across machines. Only claim determinism for the physics state.
