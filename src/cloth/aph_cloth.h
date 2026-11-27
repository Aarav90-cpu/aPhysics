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

#ifndef APH_CLOTH_H
#define APH_CLOTH_H

#include "core/aph_status.h"
#include "core/aph_world.h"
#include <stdint.h>

/**
 * Alignment in bytes for internal cloth particle arrays (supports 16-byte SIMD width).
 */
#define APH_CLOTH_ARRAY_ALIGN 16U

/**
 * Cloth creation parameters.
 *
 * Notice:
 *   Internal configuration struct. Does not contain struct_size (reserved for the
 *   public aphysics_* API in step 3.3).
 */
typedef struct aph_cloth_config {
    uint32_t vertex_count;  /* Number of cloth vertices; must be > 0 */
    const float *positions; /* Interleaved xyz coordinates (vertex_count * 3 finite floats) */
    float mass;             /* Total cloth mass in kg (distributed evenly); finite and > 0 */
    float damping;          /* Velocity damping coefficient in s^-1; finite and >= 0 */
    float thickness;        /* Collision surface thickness in m; finite and >= 0 */
} aph_cloth_config;

/**
 * Creates a cloth simulation object in the world's arena and appends it in insertion order.
 *
 * Allocates all vertex buffers (interleaved xyz positions, previous positions, velocities,
 * and inverse masses) aligned to 16 bytes inside the world's linear arena. If any validation
 * or allocation step fails, rewinds the arena to its prior mark so no memory remains consumed.
 *
 * World State Hash Extension:
 *   When cloths exist on a world, aph_world_hash() hashes each cloth in insertion order
 *   immediately after the 36-byte world header using the following little-endian layout:
 *     1. id           (uint32_t, 4 bytes)
 *     2. vertex_count (uint32_t, 4 bytes)
 *     3. mass         (float,    4 bytes, IEEE 754)
 *     4. damping      (float,    4 bytes, IEEE 754)
 *     5. thickness    (float,    4 bytes, IEEE 754)
 *     6. positions    (vertex_count * 3 * 4 bytes, interleaved xyz floats)
 *     7. velocities   (vertex_count * 3 * 4 bytes, interleaved xyz floats)
 *     8. inv_masses   (vertex_count * 4 bytes, per-vertex inverse masses; 0.0f if pinned)
 *
 * Inputs:
 *   w: Target simulation world. Must not be NULL.
 *   cfg: Cloth configuration parameters. Must not be NULL.
 *
 * Outputs:
 *   out_cloth_id: Destination receiving the 0-based insertion index id. Must not be NULL.
 *                 Set to 0 on failure.
 *
 * Returns:
 *   APH_OK on success.
 *   APH_ERR_INVALID_ARGUMENT if w, cfg, cfg->positions, or out_cloth_id is NULL, if vertex_count
 *                            is 0, if any position is non-finite, if mass <= 0 or non-finite, if
 *                            damping < 0 or non-finite, or if thickness < 0 or non-finite.
 *   APH_ERR_OUT_OF_MEMORY if the world arena lacks capacity for the cloth arrays.
 */
aph_status aph_cloth_create(aph_world *w, const aph_cloth_config *cfg, uint32_t *out_cloth_id);

/**
 * Returns a read-only pointer to the cloth's interleaved xyz position array.
 *
 * The array layout is [x0, y0, z0, x1, y1, z1, ...] in single-precision float, aligned to 16
 * bytes so callers (such as Float32Array views) can read vertex positions without re-packing.
 *
 * Inputs:
 *   w: Simulation world containing the cloth. Must not be NULL.
 *   cloth_id: Cloth identifier returned by aph_cloth_create().
 *
 * Outputs:
 *   out_positions: Receives pointer to interleaved xyz floats (set to NULL on failure).
 *   out_vertex_count: Optional destination for the vertex count (may be NULL; zeroed on failure).
 *
 * Returns:
 *   APH_OK on success.
 *   APH_ERR_INVALID_ARGUMENT if w or out_positions is NULL, or cloth_id does not exist.
 */
aph_status aph_cloth_positions(const aph_world *w, uint32_t cloth_id, const float **out_positions,
                               uint32_t *out_vertex_count);

/**
 * Pins a cloth vertex in place by setting its inverse mass to 0 and zeroing its velocity.
 *
 * Inputs:
 *   w: Simulation world containing the cloth. Must not be NULL.
 *   cloth_id: Cloth identifier returned by aph_cloth_create().
 *   vertex_index: Zero-based vertex index (< vertex_count).
 *
 * Returns:
 *   APH_OK on success.
 *   APH_ERR_INVALID_ARGUMENT if w is NULL, cloth_id is invalid, or vertex_index >= vertex_count.
 */
aph_status aph_cloth_pin(aph_world *w, uint32_t cloth_id, uint32_t vertex_index);

/**
 * Unpins a cloth vertex, restoring its default uniform inverse mass.
 *
 * Inputs:
 *   w: Simulation world containing the cloth. Must not be NULL.
 *   cloth_id: Cloth identifier returned by aph_cloth_create().
 *   vertex_index: Zero-based vertex index (< vertex_count).
 *
 * Returns:
 *   APH_OK on success.
 *   APH_ERR_INVALID_ARGUMENT if w is NULL, cloth_id is invalid, or vertex_index >= vertex_count.
 */
aph_status aph_cloth_unpin(aph_world *w, uint32_t cloth_id, uint32_t vertex_index);

#endif /* APH_CLOTH_H */
