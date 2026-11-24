# aPhysics Naming, Architecture & Layout Rules

> **Important**: This document defines the naming conventions, API design patterns, and repository layout for aPhysics. All AI coding agents and human contributors must follow these rules without exception.

---

## 1. C Naming Conventions

### Public API Names (`aphysics_`)
All identifiers exposed in the public header (`include/aphysics.h`) must begin with the `aphysics_` prefix:
- **Functions**: `aphysics_world_create`, `aphysics_world_step`, `aphysics_cloth_create`
- **Structs / Types**: `aphysics_config`, `aphysics_world`, `aphysics_cloth`, `aphysics_vertex_view`
- **Constants & Enums**: `APHYSICS_OK`, `APHYSICS_VERSION_MAJOR`

### Internal Names (`aph_`)
All internal functions, variables, macros, and subsystem headers within `src/` must begin with the `aph_` prefix:
- **Functions**: `aph_arena_alloc`, `aph_rng_next_u64`, `aph_broadphase_update`
- **Structs / Types**: `aph_status`, `aph_arena`, `aph_vec3`, `aph_mat3`, `aph_spring`
- **Internal Constants**: `APH_OK`, `APH_DEFAULT_ARENA_SIZE`

### Type Naming Style
- **Rule**: Do **not** use the `_t` suffix for types (e.g. use `aph_status`, `aph_arena`, `aphysics_world`, `aph_vec3`).
- **Rationale**: POSIX reserves identifiers ending in `_t`. Omitting `_t` keeps names clean, consistent, and collision-free.

---

## 2. Public Struct Layout & ABI Versioning

To ensure binary forward and backward compatibility as the physics engine evolves:
- **Rule**: Every public struct **must** begin with `uint32_t struct_size;` as its very first field.
- **Example**:
  ```c
  typedef struct aphysics_config {
      uint32_t struct_size;  /* Set to sizeof(aphysics_config) by caller */
      float gravity[3];
      uint32_t max_particles;
      uint64_t seed;
  } aphysics_config;
  ```
- **Rationale**: This allows the library to inspect `struct_size` to detect which revision of a struct the caller compiled against, enabling future fields to be added without breaking existing callers or WebAssembly bindings.
- **Internal vs. Public Note**: Internal engine structs (prefixed with `aph_`, such as `aph_world_config`) do not use `struct_size`. A public `aphysics_*` layer with versioned structs, `float[3]` vectors and public status names is designed with the wasm exports (step 3.3).

---

## 3. Error Handling & Function Signatures

- **Rule**: Every function capable of encountering an error condition **must** return `aph_status` (an enum).
- **Rule**: Function outputs must be returned via output pointer arguments (prefixed with `out_`):
  ```c
  typedef enum aph_status {
      APH_OK = 0,
      APH_ERR_INVALID_ARGUMENT = 1,
      APH_ERR_OUT_OF_MEMORY = 2,
      APH_ERR_BUFFER_TOO_SMALL = 3,
      APH_ERR_NOT_INITIALIZED = 4,
      APH_ERR_UNSUPPORTED = 5,
      APH_ERR_INTERNAL = 6
  } aph_status;

  /* Example: returns status, output object returned via out pointer */
  aph_status aphysics_world_create(
      const aphysics_config *config,
      aphysics_world **out_world
  );
  ```

---

## 4. Repository Directory Layout

```text
include/
└── aphysics.h            # Public C API (small, stable, self-contained)

src/
├── core/                 # Math, RNG (SplitMix64/PCG), arena allocators, hashing, logging, status
├── mesh/                 # Mesh topology, edge and triangle adjacency construction
├── cloth/                # Position-based dynamics (PBD) cloth & soft-body solver
├── collision/            # Colliders, bounding volume hierarchies (BVH), broadphase, contact constraints
├── rigid/                # Rigid body dynamics, inertia tensors, impulse solver (Phase 6)
├── soft/                 # Volumetric lattice soft bodies (Phase 7)
├── bake/                 # Bake engine, keyframe serialization, frame cache formats
└── wasm/                 # WebAssembly entrypoints, linear memory views, JS bindings

tests/                    # Test suite: one cc_test per module using custom assert harness
tools/                    # Dump utilities, standalone baker CLI, viewer tools (contains aph.bzl)
js/                       # JavaScript and TypeScript wrappers for Three.js & Remotion
examples/                 # Three.js canvas demos, Remotion video composition examples
benchmarks/               # Performance measurement and profiling benchmarks
agentrules/               # Persistent AI coding agent rules (NOTE.md, NamingRules.md)
```

---

## 5. Bazel Compiler Flags & Build Rules

- **Single Source of Truth (`.bazelrc`)**: All global compiler flags, dialects, and sanitizer configurations are centralized exclusively in `.bazelrc`:
  - Always enabled: `-std=c17`, `-Wall`, `-Wextra`, `-Werror`, `-ffp-contract=off`, `-fno-math-errno`.
  - Strictly forbidden: `-ffast-math`.
  - AddressSanitizer: `--config=asan` (`-fsanitize=address`, `-fno-omit-frame-pointer`, `-g`, `--linkopt=-fsanitize=address`).
  - UndefinedBehaviorSanitizer: `--config=ubsan` (`-fsanitize=undefined`, `-g`, `--linkopt=-fsanitize=undefined`).
- **Target Copts**: `tools/aph.bzl` exports `APH_COPTS = []` so that targets do not duplicate compiler flags already enforced globally by `.bazelrc`. Target-specific flags (if ever required) can be added cleanly without collision.
