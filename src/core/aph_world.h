/*
 * Copyright 2026 Aarav Ravindra Kharade
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef APH_WORLD_H
#define APH_WORLD_H

#include "core/aph_math.h"
#include "core/aph_status.h"
#include <stddef.h>
#include <stdint.h>

/**
 * Minimum memory budget for an aph_world arena in bytes (1024 bytes).
 * Ensures adequate capacity for internal arena bookkeeping and baseline structures.
 */
#define APH_WORLD_MIN_MEMORY_BYTES 1024ULL

/**
 * Maximum supported fixed time-step in seconds (1.0 second).
 * Catches accidental unit errors, such as callers passing milliseconds (e.g. 16.6f).
 */
#define APH_WORLD_MAX_FIXED_DT 1.0f

/**
 * Maximum supported substeps per simulation step (1000).
 * Prevents accidental or unbounded substep counts that would hang execution.
 */
#define APH_WORLD_MAX_SUBSTEPS 1000U

/**
 * Opaque simulation world handle.
 */
typedef struct aph_world aph_world;

/**
 * World configuration parameters.
 *
 * Notice:
 *   Internal configuration struct. Does not contain struct_size (which is reserved
 *   for the public aphysics_* API in step 3.3).
 */
typedef struct aph_world_config {
    uint64_t seed;       /* Simulation PRNG seed; core never queries a system clock */
    float fixed_dt;      /* Fixed simulation time-step in seconds (e.g. 1.0f / 60.0f) */
    uint32_t substeps;   /* Number of solver substeps per step (e.g. 8) */
    aph_vec3 gravity;    /* Gravity acceleration vector in m/s^2 (e.g. {0.0f, -9.81f, 0.0f}) */
    size_t memory_bytes; /* Total linear arena memory budget in bytes */
} aph_world_config;

/**
 * Creates and initializes a new simulation world.
 *
 * Preconditions:
 *   cfg must point to a valid aph_world_config.
 *   out_world must not be NULL.
 *
 * Thread Safety:
 *   aph_world is not thread-safe. Synchronization across threads or workers
 *   must be managed by the caller.
 *
 * Inputs:
 *   cfg: World configuration parameters.
 *   out_world: Destination pointer receiving the created aph_world pointer.
 *              Guaranteed to be set to NULL on any failure.
 *
 * Returns:
 *   APH_OK on success.
 *   APH_ERR_INVALID_ARGUMENT if cfg or out_world is NULL, or any field in cfg is invalid.
 *   APH_ERR_OUT_OF_MEMORY if the memory budget cannot be allocated.
 */
aph_status aph_world_create(const aph_world_config *cfg, aph_world **out_world);

/**
 * Destroys a simulation world and releases all associated memory.
 *
 * Inputs:
 *   w: Pointer to world to destroy. If NULL, does nothing.
 */
void aph_world_destroy(aph_world *w);

/**
 * Advances the simulation world by exactly one fixed time-step.
 *
 * Preconditions:
 *   w must not be NULL.
 *
 * Invariant:
 *   Strictly zero dynamic allocations occur inside aph_world_step.
 *
 * Inputs:
 *   w: Pointer to simulation world.
 *
 * Returns:
 *   APH_OK on success.
 *   APH_ERR_INVALID_ARGUMENT if w is NULL.
 */
aph_status aph_world_step(aph_world *w);

/**
 * Returns the total number of simulation steps completed by the world.
 *
 * Inputs:
 *   w: Pointer to simulation world. May be NULL.
 *
 * Returns:
 *   Cumulative step count, or 0 if w is NULL.
 */
uint64_t aph_world_step_count(const aph_world *w);

/**
 * Computes the 64-bit FNV-1a hash of the simulation state.
 *
 * Byte layout:
 *   Hashes discrete state fields in little-endian order without struct padding:
 *     1. seed       (uint64_t, 8 bytes, little-endian)
 *     2. step_count (uint64_t, 8 bytes, little-endian)
 *     3. fixed_dt   (float,    4 bytes, IEEE 754 little-endian)
 *     4. substeps   (uint32_t, 4 bytes, little-endian)
 *     5. gravity.x  (float,    4 bytes, IEEE 754 little-endian)
 *     6. gravity.y  (float,    4 bytes, IEEE 754 little-endian)
 *     7. gravity.z  (float,    4 bytes, IEEE 754 little-endian)
 *   Total bytes hashed: 36 bytes.
 *
 * Preconditions:
 *   If w is non-NULL, state must not contain NaNs.
 *
 * Inputs:
 *   w: Pointer to simulation world. May be NULL.
 *
 * Returns:
 *   64-bit state hash, or 0 if w is NULL.
 */
uint64_t aph_world_hash(const aph_world *w);

#endif /* APH_WORLD_H */
