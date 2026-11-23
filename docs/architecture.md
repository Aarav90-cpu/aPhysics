# Architecture: Design (not implemented)

> **Notice**: The features described in this document represent the planned architectural specification for aPhysics. **No physics simulation code is implemented yet.**

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
