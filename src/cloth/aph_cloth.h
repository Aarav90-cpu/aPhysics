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

#include "core/aph_math.h"
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
    const uint32_t
        *indices;       /* Triangle vertex indices (3 * tri_count), or NULL if tri_count == 0 */
    uint32_t tri_count; /* Number of triangles; 0 creates constraint-free point particles */
    float mass;         /* Total cloth mass in kg (distributed evenly); finite and > 0 */
    float damping;      /* Velocity damping coefficient in s^-1; finite and >= 0 */
    float thickness;    /* Collision surface thickness in m; finite and >= 0 */
    float
        stretch_compliance; /* XPBD edge stretch compliance (s^2/kg, 0 = infinitely stiff); >= 0 */
} aph_cloth_config;

/**
 * Creates a cloth simulation object in the world's arena and appends it in insertion order.
 *
 * Allocates all vertex buffers (interleaved xyz positions, previous positions, velocities,
 * and inverse masses) aligned to 16 bytes inside the world's linear arena. When tri_count > 0,
 * builds unique sorted mesh edges and computes per-edge stretch rest lengths from the initial
 * positions. If any validation or allocation step fails, rewinds the arena to its prior mark so
 * no memory remains consumed.
 *
 * World State Hash Extension:
 *   When cloths exist on a world, aph_world_hash() hashes each cloth in insertion order
 *   immediately after the 36-byte world header using the following little-endian layout:
 *     1.  id                 (uint32_t, 4 bytes)
 *     2.  vertex_count       (uint32_t, 4 bytes)
 *     3.  mass               (float,    4 bytes, IEEE 754)
 *     4.  damping            (float,    4 bytes, IEEE 754)
 *     5.  thickness          (float,    4 bytes, IEEE 754)
 *     6.  stretch_compliance (float,    4 bytes, IEEE 754)
 *     7.  edge_count         (uint32_t, 4 bytes)
 *     8.  positions          (vertex_count * 3 * 4 bytes, interleaved xyz floats)
 *     9.  velocities         (vertex_count * 3 * 4 bytes, interleaved xyz floats)
 *     10. inv_masses         (vertex_count * 4 bytes, per-vertex inverse masses; 0.0f if pinned)
 *     11. stretch            (edge_count * 12 bytes: v0 uint32, v1 uint32, rest_length float each)
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
 *                            is 0, if any position is non-finite, if tri_count and indices
 * mismatch, if any triangle or edge rest length is invalid/zero, if mass <= 0 or non-finite, or if
 * damping, thickness, or stretch_compliance < 0 or non-finite. APH_ERR_UNSUPPORTED if a mesh edge
 * is shared by three or more triangles. APH_ERR_OUT_OF_MEMORY if the world arena lacks capacity for
 * the cloth buffers.
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
aph_status aph_cloth_pin(const aph_world *w, uint32_t cloth_id, uint32_t vertex_index);

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
aph_status aph_cloth_unpin(const aph_world *w, uint32_t cloth_id, uint32_t vertex_index);

/**
 * Sets a cloth vertex position, syncing previous position and zeroing its velocity.
 *
 * Does not modify constraint rest lengths derived at cloth creation.
 *
 * Inputs:
 *   w: Simulation world containing the cloth. Must not be NULL.
 *   cloth_id: Cloth identifier returned by aph_cloth_create().
 *   vertex_index: Zero-based vertex index (< vertex_count).
 *   pos: New vertex position with finite components.
 *
 * Returns:
 *   APH_OK on success.
 *   APH_ERR_INVALID_ARGUMENT if w is NULL, cloth_id is invalid, vertex_index >= vertex_count, or
 *                            any component of pos is non-finite.
 */
aph_status aph_cloth_set_position(const aph_world *w, uint32_t cloth_id, uint32_t vertex_index,
                                  aph_vec3 pos);

#endif /* APH_CLOTH_H */
