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

#include "cloth/aph_cloth.h"
#include "cloth/aph_cloth_internal.h"
#include "core/aph_hash.h"
#include "core/aph_log.h"
#include "core/aph_math.h"
#include <math.h>
#include <stddef.h>
#include <stdint.h>

aph_status aph_cloth_stretch_build(aph_cloth *c, const uint32_t *indices, uint32_t tri_count,
                                   aph_arena *arena) {
    if (c == NULL || arena == NULL) {
        return APH_ERR_INVALID_ARGUMENT;
    }

    c->mesh_edges.edges = NULL;
    c->mesh_edges.edge_count = 0U;
    c->stretch_rest_lengths = NULL;

    if (tri_count == 0U) {
        return APH_OK;
    }

    aph_mesh_edges edges;
    aph_status status = aph_mesh_build_edges(indices, tri_count, c->vertex_count, arena, &edges);
    if (status != APH_OK) {
        return status;
    }

    const size_t rest_bytes = (size_t)edges.edge_count * sizeof(float);
    void *rest_mem = NULL;
    status = aph_arena_alloc(arena, rest_bytes, APH_CLOTH_ARRAY_ALIGN, &rest_mem);
    if (status != APH_OK) {
        aph_log(APH_LOG_ERROR, "cloth",
                "Out of memory: world arena cannot fit stretch rest lengths. Increase memory_bytes "
                "in world config.");
        return status;
    }

    float *rest_lengths = (float *)rest_mem;
    for (uint32_t e = 0U; e < edges.edge_count; ++e) {
        const size_t idx_i = (size_t)edges.edges[e].v0 * 3U;
        const size_t idx_j = (size_t)edges.edges[e].v1 * 3U;
        const aph_vec3 xi = aph_vec3_make(c->positions[idx_i + 0U], c->positions[idx_i + 1U],
                                          c->positions[idx_i + 2U]);
        const aph_vec3 xj = aph_vec3_make(c->positions[idx_j + 0U], c->positions[idx_j + 1U],
                                          c->positions[idx_j + 2U]);
        const float len_sq = aph_vec3_length_sq(aph_vec3_sub(xi, xj));
        if (len_sq <= APH_VEC3_NORMALIZE_EPSILON_SQ) {
            aph_log(APH_LOG_ERROR, "cloth",
                    "Invalid mesh edge: initial edge rest length must be non-zero. Separate "
                    "coincident edge vertices.");
            return APH_ERR_INVALID_ARGUMENT;
        }
        rest_lengths[e] = sqrtf(len_sq);
    }

    c->mesh_edges = edges;
    c->stretch_rest_lengths = rest_lengths;
    return APH_OK;
}

void aph_cloth_stretch_solve(const aph_mesh_edge *edges, const float *rest_lengths,
                             uint32_t edge_count, float compliance, const float *inv_masses,
                             float *positions, float h) {
    if (edge_count == 0U || edges == NULL || rest_lengths == NULL || inv_masses == NULL ||
        positions == NULL) {
        return;
    }

    const float alpha_tilde = compliance / (h * h);

    for (uint32_t e = 0U; e < edge_count; ++e) {
        const uint32_t i = edges[e].v0;
        const uint32_t j = edges[e].v1;

        const float wi = inv_masses[i];
        const float wj = inv_masses[j];
        const float w_sum = wi + wj;
        if (w_sum <= 0.0f) {
            continue;
        }

        const size_t idx_i = (size_t)i * 3U;
        const size_t idx_j = (size_t)j * 3U;

        const aph_vec3 xi =
            aph_vec3_make(positions[idx_i + 0U], positions[idx_i + 1U], positions[idx_i + 2U]);
        const aph_vec3 xj =
            aph_vec3_make(positions[idx_j + 0U], positions[idx_j + 1U], positions[idx_j + 2U]);
        const aph_vec3 delta = aph_vec3_sub(xi, xj);
        const float len_sq = aph_vec3_length_sq(delta);
        if (len_sq <= APH_VEC3_NORMALIZE_EPSILON_SQ) {
            continue;
        }

        const float denom = w_sum + alpha_tilde;
        if (denom <= 0.0f || !isfinite(denom)) {
            continue;
        }

        const float len = sqrtf(len_sq);
        const float c_val = len - rest_lengths[e];
        const float dlambda = -c_val / denom;
        const float corr = dlambda / len;

        if (wi > 0.0f) {
            const float si = wi * corr;
            positions[idx_i + 0U] += delta.x * si;
            positions[idx_i + 1U] += delta.y * si;
            positions[idx_i + 2U] += delta.z * si;
        }
        if (wj > 0.0f) {
            const float sj = wj * corr;
            positions[idx_j + 0U] -= delta.x * sj;
            positions[idx_j + 1U] -= delta.y * sj;
            positions[idx_j + 2U] -= delta.z * sj;
        }
    }
}

uint64_t aph_cloth_stretch_hash(const aph_mesh_edge *edges, const float *rest_lengths,
                                uint32_t edge_count, uint64_t state) {
    uint64_t h = state;
    for (uint32_t e = 0U; e < edge_count; ++e) {
        h = aph_hash_bytes(&edges[e].v0, sizeof(edges[e].v0), h);
        h = aph_hash_bytes(&edges[e].v1, sizeof(edges[e].v1), h);
        h = aph_hash_bytes(&rest_lengths[e], sizeof(rest_lengths[e]), h);
    }
    return h;
}
