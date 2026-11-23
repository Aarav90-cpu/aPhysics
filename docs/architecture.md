# Architecture Design: Zero-Copy Memory & Determinism (Planned)

> **Notice**: The features described in this document represent the planned architectural specification for aPhysics. Implementation is in progress.

aPhysics is designed specifically to eliminate performance bottlenecks when streaming simulation data into Three.js inside Remotion video rendering pipelines.

---

## 1. [Planned] Zero-Copy Memory Sharing with Three.js

### The Problem
Traditional physics bridges serialize simulation state to JSON or copy large arrays across the WebAssembly-to-JavaScript boundary on every frame. When simulating cloth meshes with tens of thousands of vertices, copying buffers triggers heavy CPU usage and Garbage Collection (GC) pauses.

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
         v
+-----------------------------------------------------------+
| GPU Vertex Buffer Object (VBO)                            |
| Uploaded directly by WebGL from wasmMemory.buffer         |
+-----------------------------------------------------------+
```

Because `Float32Array` directly views the WebAssembly memory buffer, **zero buffer copies** occur in JavaScript. WebGL uploads the updated vertex data directly to the GPU VBO.

---

## 2. [Planned] Deterministic Simulation via Seeds

For Remotion animation rendering, frame-accurate determinism is critical. Remotion renders frames concurrently across multiple worker processes or cloud renderers.

1. **Stateful Pseudo-Random Number Generation**:
   - Internal perturbation functions (wrinkle noise, fabric tearing thresholds, structural crumple vectors) will rely on a deterministic 64-bit PRNG (e.g., SplitMix64 or PCG32).
2. **Fixed Seed Mode**:
   - Passing an explicit seed (e.g. `1337ULL`) will guarantee that every simulation run produces the exact same vertex trajectories on every machine.
3. **Random Seed Mode**:
   - Passing `0` or omitting the seed will initialize the generator with time entropy, providing unique behavior for interactive previews.
4. **Decoupled Fixed Delta Time ($\Delta t$)**:
   - The physics integrator calculates states at fixed time steps (e.g., $1/60\text{ s}$), preventing frame-rate fluctuations from altering the simulation outcome.
