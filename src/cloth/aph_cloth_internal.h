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

#ifndef APH_CLOTH_INTERNAL_H
#define APH_CLOTH_INTERNAL_H

#include "core/aph_arena.h"
#include "core/aph_status.h"
#include "mesh/aph_mesh.h"
#include <stdint.h>

/**
 * Internal per-cloth state stored inside the world arena as an insertion-order linked list.
 */
typedef struct aph_cloth {
    uint32_t id;
    uint32_t vertex_count;
    float mass;
    float damping;
    float thickness;
    float stretch_compliance;
    float default_inv_mass;
    float *positions;
    float *prev_positions;
    float *velocities;
    float *inv_masses;
    aph_mesh_edges mesh_edges;
    float *stretch_rest_lengths;
    struct aph_cloth *next;
} aph_cloth;

/**
 * Builds deduplicated mesh edge topology and rest lengths for a cloth's initial positions.
 *
 * Inputs:
 *   c: Initialized cloth record with valid vertex_count and positions. Must not be NULL.
 *   indices: Triangle vertex indices (3 * tri_count). Must not be NULL when tri_count > 0.
 *   tri_count: Number of triangles in the cloth mesh.
 *   arena: World arena for edge and rest-length allocations. Must not be NULL.
 *
 * Returns:
 *   APH_OK on success.
 *   APH_ERR_INVALID_ARGUMENT if topology or initial edge rest lengths are invalid.
 *   APH_ERR_UNSUPPORTED if a mesh edge is shared by three or more triangles.
 *   APH_ERR_OUT_OF_MEMORY if arena lacks capacity.
 */
aph_status aph_cloth_stretch_build(aph_cloth *c, const uint32_t *indices, uint32_t tri_count,
                                   aph_arena *arena);

/**
 * Runs one Gauss-Seidel XPBD pass over all edge distance constraints in ascending edge order.
 *
 * Assumes lambda = 0 at the start of each substep (single iteration per substep).
 *
 * Inputs:
 *   edges: Edge array of length edge_count.
 *   rest_lengths: Initial edge rest lengths in meters of length edge_count.
 *   edge_count: Number of stretch constraints.
 *   compliance: Cloth stretch compliance in s^2/kg (>= 0).
 *   inv_masses: Per-vertex inverse masses (0.0f for pinned vertices).
 *   h: Substep duration in seconds (> 0).
 *
 * Outputs:
 *   positions: Interleaved xyz particle positions updated in place.
 */
void aph_cloth_stretch_solve(const aph_mesh_edge *edges, const float *rest_lengths,
                             uint32_t edge_count, float compliance, const float *inv_masses,
                             float *positions, float h);

/**
 * Appends stretch constraint topology and rest lengths to an FNV-1a world state hash.
 *
 * Inputs:
 *   edges: Edge array of length edge_count.
 *   rest_lengths: Rest lengths of length edge_count.
 *   edge_count: Number of stretch constraints.
 *   state: Running 64-bit FNV-1a hash state.
 *
 * Returns:
 *   Updated 64-bit FNV-1a hash state.
 */
uint64_t aph_cloth_stretch_hash(const aph_mesh_edge *edges, const float *rest_lengths,
                                uint32_t edge_count, uint64_t state);

#endif /* APH_CLOTH_INTERNAL_H */
