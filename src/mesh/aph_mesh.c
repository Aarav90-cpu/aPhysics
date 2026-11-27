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

#include "mesh/aph_mesh.h"
#include "core/aph_log.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static bool edge_record_less(const aph_mesh_edge *a, const aph_mesh_edge *b) {
    if (a->v0 != b->v0) {
        return a->v0 < b->v0;
    }
    if (a->v1 != b->v1) {
        return a->v1 < b->v1;
    }
    return a->tri0 < b->tri0;
}

static void sift_down(aph_mesh_edge *arr, size_t root, size_t end) {
    while (root * 2U + 1U < end) {
        size_t child = root * 2U + 1U;
        if (child + 1U < end && edge_record_less(&arr[child], &arr[child + 1U])) {
            child++;
        }
        if (!edge_record_less(&arr[root], &arr[child])) {
            return;
        }
        aph_mesh_edge tmp = arr[root];
        arr[root] = arr[child];
        arr[child] = tmp;
        root = child;
    }
}

static void heapsort_edge_records(aph_mesh_edge *arr, size_t count) {
    for (size_t i = count / 2U; i > 0U; --i) {
        sift_down(arr, i - 1U, count);
    }
    for (size_t end = count; end > 1U; --end) {
        aph_mesh_edge tmp = arr[0];
        arr[0] = arr[end - 1U];
        arr[end - 1U] = tmp;
        sift_down(arr, 0U, end - 1U);
    }
}

aph_status aph_mesh_build_edges(const uint32_t *indices, uint32_t tri_count, uint32_t vertex_count,
                                aph_arena *arena, aph_mesh_edges *out) {
    if (out != NULL) {
        out->edges = NULL;
        out->edge_count = 0U;
    }
    if (out == NULL) {
        return APH_ERR_INVALID_ARGUMENT;
    }
    if (arena == NULL || arena->base == NULL) {
        aph_log(APH_LOG_ERROR, "mesh",
                "Arena pointer is NULL or uninitialized. Pass an initialized aph_arena pointer.");
        return APH_ERR_INVALID_ARGUMENT;
    }
    if (indices == NULL) {
        aph_log(APH_LOG_ERROR, "mesh",
                "Triangle index buffer pointer is NULL. Pass a valid triangle index array.");
        return APH_ERR_INVALID_ARGUMENT;
    }
    if (tri_count == 0U) {
        aph_log(APH_LOG_ERROR, "mesh",
                "Invalid tri_count: mesh must contain at least one triangle.");
        return APH_ERR_INVALID_ARGUMENT;
    }
    if (tri_count > UINT32_MAX / 3U ||
        (size_t)tri_count > SIZE_MAX / (3U * sizeof(aph_mesh_edge))) {
        aph_log(APH_LOG_ERROR, "mesh",
                "Invalid tri_count: index or edge buffer size exceeds supported limits. Reduce "
                "tri_count.");
        return APH_ERR_INVALID_ARGUMENT;
    }

    for (uint32_t t = 0U; t < tri_count; ++t) {
        const size_t base_idx = (size_t)t * 3U;
        const uint32_t i0 = indices[base_idx + 0U];
        const uint32_t i1 = indices[base_idx + 1U];
        const uint32_t i2 = indices[base_idx + 2U];

        if (i0 >= vertex_count || i1 >= vertex_count || i2 >= vertex_count) {
            aph_log(APH_LOG_ERROR, "mesh",
                    "Triangle index out of range. Ensure every vertex index is strictly less than "
                    "vertex_count.");
            return APH_ERR_INVALID_ARGUMENT;
        }
        if (i0 == i1 || i1 == i2 || i2 == i0) {
            aph_log(APH_LOG_ERROR, "mesh",
                    "Degenerate triangle found with repeated vertex indices. Remove degenerate "
                    "triangles from the mesh.");
            return APH_ERR_INVALID_ARGUMENT;
        }
    }

    const size_t raw_count = (size_t)tri_count * 3U;
    const size_t raw_bytes = raw_count * sizeof(aph_mesh_edge);
    const size_t initial_mark = aph_arena_mark(arena);

    void *block_mem = NULL;
    aph_status status = aph_arena_alloc(arena, raw_bytes, APH_MESH_ARRAY_ALIGN, &block_mem);
    if (status != APH_OK) {
        (void)aph_arena_rewind(arena, initial_mark);
        aph_log(APH_LOG_ERROR, "mesh",
                "Out of memory: arena cannot fit temporary edge records. Increase arena capacity.");
        return status;
    }

    const size_t block_offset = (size_t)((const uint8_t *)block_mem - arena->base);
    aph_mesh_edge *edges = (aph_mesh_edge *)block_mem;

    /* Emit three undirected edge records per triangle; each (v0, v1, tri0) triple is unique. */
    for (uint32_t t = 0U; t < tri_count; ++t) {
        const size_t base_idx = (size_t)t * 3U;
        const uint32_t tri_verts[3] = {
            indices[base_idx + 0U],
            indices[base_idx + 1U],
            indices[base_idx + 2U],
        };

        for (uint32_t e = 0U; e < 3U; ++e) {
            const uint32_t a = tri_verts[e];
            const uint32_t b = tri_verts[(e + 1U) % 3U];
            const uint32_t opp = tri_verts[(e + 2U) % 3U];

            aph_mesh_edge *rec = &edges[base_idx + (size_t)e];
            rec->v0 = (a < b) ? a : b;
            rec->v1 = (a < b) ? b : a;
            rec->tri0 = t;
            rec->tri1 = APH_MESH_INVALID_INDEX;
            rec->opp0 = opp;
            rec->opp1 = APH_MESH_INVALID_INDEX;
        }
    }

    heapsort_edge_records(edges, raw_count);

    /* Compact unique edges in place; write_idx <= read_idx holds for every step. */
    size_t write_idx = 0U;
    size_t read_idx = 0U;
    while (read_idx < raw_count) {
        aph_mesh_edge merged = edges[read_idx];

        if (read_idx + 1U < raw_count && edges[read_idx + 1U].v0 == merged.v0 &&
            edges[read_idx + 1U].v1 == merged.v1) {
            const aph_mesh_edge second = edges[read_idx + 1U];

            if (read_idx + 2U < raw_count && edges[read_idx + 2U].v0 == merged.v0 &&
                edges[read_idx + 2U].v1 == merged.v1) {
                (void)aph_arena_rewind(arena, initial_mark);
                aph_log(APH_LOG_ERROR, "mesh",
                        "Edge shared by more than two triangles. Split the mesh so each edge "
                        "belongs to at most two triangles.");
                return APH_ERR_UNSUPPORTED;
            }

            if (second.opp0 == merged.opp0) {
                (void)aph_arena_rewind(arena, initial_mark);
                aph_log(APH_LOG_ERROR, "mesh",
                        "Duplicate triangle shares an edge with identical opposite vertex. Remove "
                        "duplicate triangles from the mesh.");
                return APH_ERR_INVALID_ARGUMENT;
            }

            merged.tri1 = second.tri0;
            merged.opp1 = second.opp0;
            read_idx += 2U;
        } else {
            read_idx += 1U;
        }

        edges[write_idx++] = merged;
    }

    const size_t trimmed_used = block_offset + write_idx * sizeof(aph_mesh_edge);
    (void)aph_arena_rewind(arena, trimmed_used);

    out->edges = edges;
    out->edge_count = (uint32_t)write_idx;
    return APH_OK;
}
