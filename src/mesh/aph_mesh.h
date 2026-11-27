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

#ifndef APH_MESH_H
#define APH_MESH_H

#include "core/aph_arena.h"
#include "core/aph_status.h"
#include <stdint.h>

/**
 * Sentinel index representing an absent adjacent triangle or opposite vertex on boundary edges.
 */
#define APH_MESH_INVALID_INDEX UINT32_MAX

/**
 * Alignment in bytes for allocated mesh edge arrays.
 */
#define APH_MESH_ARRAY_ALIGN 16U

/**
 * Unique undirected mesh edge with up to two incident triangles and their opposite vertices.
 */
typedef struct aph_mesh_edge {
    uint32_t v0;   /* Lower vertex index: min(a, b) */
    uint32_t v1;   /* Higher vertex index: max(a, b) */
    uint32_t tri0; /* First incident triangle index (tri0 < tri1 when shared) */
    uint32_t tri1; /* Second incident triangle index, or APH_MESH_INVALID_INDEX if boundary */
    uint32_t opp0; /* Opposite vertex index in tri0 */
    uint32_t opp1; /* Opposite vertex index in tri1, or APH_MESH_INVALID_INDEX if boundary */
} aph_mesh_edge;

/**
 * Deduplicated, deterministically sorted edge topology extracted from a triangle mesh.
 */
typedef struct aph_mesh_edges {
    aph_mesh_edge *edges;
    uint32_t edge_count;
} aph_mesh_edges;

/**
 * Extracts unique sorted edges and triangle adjacency from an indexed triangle mesh.
 *
 * Allocates one temporary block of 3 * tri_count edge records inside arena, sorts them
 * in place with an iterative heapsort by (v0, v1, tri0), compacts unique edges to the front
 * of the block, and trims the unused tail via aph_arena_rewind() so only edge_count entries
 * remain consumed.
 *
 * Notice:
 *   Edges are ordered deterministically by ascending (v0, v1) without hash tables or qsort.
 *   On any error, rewinds arena to its mark on entry so no memory remains consumed.
 *
 * Inputs:
 *   indices: Triangle vertex indices (3 * tri_count entries). Must not be NULL.
 *   tri_count: Number of triangles in the mesh. Must be > 0.
 *   vertex_count: Number of vertices in the mesh. All indices must satisfy index < vertex_count.
 *   arena: Target linear memory arena. Must not be NULL and must be initialized.
 *
 * Outputs:
 *   out: Destination receiving allocated edge array and count. Zeroed on error. Must not be NULL.
 *
 * Returns:
 *   APH_OK on success.
 *   APH_ERR_INVALID_ARGUMENT if indices, arena, or out is NULL; if tri_count == 0; if any index
 *                            is >= vertex_count; if a triangle has repeated vertices; if two
 *                            triangles share an edge with identical opposite vertex (duplicate); or
 *                            if tri_count > UINT32_MAX / 3 or 3 * tri_count * sizeof(aph_mesh_edge)
 *                            overflows size_t.
 *   APH_ERR_UNSUPPORTED if an edge is shared by three or more triangles (non-manifold mesh).
 *   APH_ERR_OUT_OF_MEMORY if arena lacks capacity for 3 * tri_count temporary edge records.
 */
aph_status aph_mesh_build_edges(const uint32_t *indices, uint32_t tri_count, uint32_t vertex_count,
                                aph_arena *arena, aph_mesh_edges *out);

#endif /* APH_MESH_H */
