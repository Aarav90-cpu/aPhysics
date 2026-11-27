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
#include "core/aph_arena.h"
#include "core/aph_hash.h"
#include "core/aph_log.h"
#include "core/aph_world_internal.h"
#include <math.h>
#include <stddef.h>
#include <stdint.h>

typedef struct aph_cloth {
    uint32_t id;
    uint32_t vertex_count;
    float mass;
    float damping;
    float thickness;
    float default_inv_mass;
    float *positions;
    float *prev_positions;
    float *velocities;
    float *inv_masses;
    struct aph_cloth *next;
} aph_cloth;

static aph_cloth *find_cloth(const aph_world *w, uint32_t cloth_id) {
    if (w == NULL || cloth_id >= w->cloth_count) {
        return NULL;
    }
    aph_cloth *curr = (aph_cloth *)w->cloths_head;
    while (curr != NULL) {
        if (curr->id == cloth_id) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

static aph_status aph_cloth_world_substep(aph_world *w, float h) {
    const float gx = w->config.gravity.x;
    const float gy = w->config.gravity.y;
    const float gz = w->config.gravity.z;

    aph_cloth *c = (aph_cloth *)w->cloths_head;
    while (c != NULL) {
        const uint32_t n = c->vertex_count;

        /* Explicit integration prediction for unpinned particles (inv_mass > 0). */
        for (uint32_t i = 0; i < n; ++i) {
            if (c->inv_masses[i] > 0.0f) {
                const size_t idx = (size_t)i * 3U;
                float vx = c->velocities[idx + 0U] + gx * h;
                float vy = c->velocities[idx + 1U] + gy * h;
                float vz = c->velocities[idx + 2U] + gz * h;

                const float px = c->positions[idx + 0U];
                const float py = c->positions[idx + 1U];
                const float pz = c->positions[idx + 2U];

                c->prev_positions[idx + 0U] = px;
                c->prev_positions[idx + 1U] = py;
                c->prev_positions[idx + 2U] = pz;

                c->positions[idx + 0U] = px + vx * h;
                c->positions[idx + 1U] = py + vy * h;
                c->positions[idx + 2U] = pz + vz * h;
            }
        }

        /* Step 2.1: constraints (step 2.2+) and colliders (step 2.6+) are not active yet. */

        /* Derive post-solve velocities and apply linear substep damping. */
        float damp_factor = 1.0f - c->damping * h;
        if (damp_factor < 0.0f) {
            damp_factor = 0.0f;
        }

        for (uint32_t i = 0; i < n; ++i) {
            const size_t idx = (size_t)i * 3U;
            if (c->inv_masses[i] > 0.0f) {
                float vx = (c->positions[idx + 0U] - c->prev_positions[idx + 0U]) / h;
                float vy = (c->positions[idx + 1U] - c->prev_positions[idx + 1U]) / h;
                float vz = (c->positions[idx + 2U] - c->prev_positions[idx + 2U]) / h;

                c->velocities[idx + 0U] = vx * damp_factor;
                c->velocities[idx + 1U] = vy * damp_factor;
                c->velocities[idx + 2U] = vz * damp_factor;
            }
        }

        c = c->next;
    }

    return APH_OK;
}

static uint64_t aph_cloth_world_hash(const aph_world *w, uint64_t state) {
    uint64_t h = state;
    const aph_cloth *c = (const aph_cloth *)w->cloths_head;
    while (c != NULL) {
        const size_t vec_bytes = (size_t)c->vertex_count * 3U * sizeof(float);
        const size_t invm_bytes = (size_t)c->vertex_count * sizeof(float);

        h = aph_hash_bytes(&c->id, sizeof(c->id), h);
        h = aph_hash_bytes(&c->vertex_count, sizeof(c->vertex_count), h);
        h = aph_hash_bytes(&c->mass, sizeof(c->mass), h);
        h = aph_hash_bytes(&c->damping, sizeof(c->damping), h);
        h = aph_hash_bytes(&c->thickness, sizeof(c->thickness), h);
        h = aph_hash_bytes(c->positions, vec_bytes, h);
        h = aph_hash_bytes(c->velocities, vec_bytes, h);
        h = aph_hash_bytes(c->inv_masses, invm_bytes, h);

        c = c->next;
    }
    return h;
}

aph_status aph_cloth_create(aph_world *w, const aph_cloth_config *cfg, uint32_t *out_cloth_id) {
    if (out_cloth_id != NULL) {
        *out_cloth_id = 0U;
    }
    if (w == NULL || out_cloth_id == NULL) {
        return APH_ERR_INVALID_ARGUMENT;
    }
    if (cfg == NULL) {
        aph_log(APH_LOG_ERROR, "cloth",
                "Cloth configuration pointer is NULL. Pass a valid aph_cloth_config pointer.");
        return APH_ERR_INVALID_ARGUMENT;
    }
    if (cfg->vertex_count == 0U) {
        aph_log(APH_LOG_ERROR, "cloth",
                "Invalid vertex_count: must be greater than zero. Provide a non-empty mesh.");
        return APH_ERR_INVALID_ARGUMENT;
    }
    if (cfg->positions == NULL) {
        aph_log(APH_LOG_ERROR, "cloth",
                "Initial positions pointer is NULL. Provide interleaved xyz vertex positions.");
        return APH_ERR_INVALID_ARGUMENT;
    }
    if (!isfinite(cfg->mass) || cfg->mass <= 0.0f) {
        aph_log(APH_LOG_ERROR, "cloth",
                "Invalid mass: cloth mass must be positive and finite. Check mass in cloth "
                "config.");
        return APH_ERR_INVALID_ARGUMENT;
    }
    if (!isfinite(cfg->damping) || cfg->damping < 0.0f) {
        aph_log(APH_LOG_ERROR, "cloth",
                "Invalid damping: damping must be non-negative and finite. Check damping in cloth "
                "config.");
        return APH_ERR_INVALID_ARGUMENT;
    }
    if (!isfinite(cfg->thickness) || cfg->thickness < 0.0f) {
        aph_log(APH_LOG_ERROR, "cloth",
                "Invalid thickness: thickness must be non-negative and finite. Check thickness in "
                "cloth config.");
        return APH_ERR_INVALID_ARGUMENT;
    }

    const float inv_mass = (float)cfg->vertex_count / cfg->mass;
    if (!isfinite(inv_mass) || inv_mass <= 0.0f) {
        aph_log(APH_LOG_ERROR, "cloth",
                "Invalid mass ratio: per-vertex inverse mass is out of range. Adjust mass or "
                "vertex_count in cloth config.");
        return APH_ERR_INVALID_ARGUMENT;
    }

    if ((size_t)cfg->vertex_count > SIZE_MAX / (3U * sizeof(float))) {
        aph_log(APH_LOG_ERROR, "cloth",
                "Invalid vertex_count: buffer size causes integer overflow. Reduce vertex_count.");
        return APH_ERR_INVALID_ARGUMENT;
    }

    const size_t float_count = (size_t)cfg->vertex_count * 3U;
    for (size_t k = 0; k < float_count; ++k) {
        if (!isfinite(cfg->positions[k])) {
            aph_log(APH_LOG_ERROR, "cloth",
                    "Invalid position: vertex coordinates must be finite numbers. Check initial "
                    "positions.");
            return APH_ERR_INVALID_ARGUMENT;
        }
    }

    if (w->cloth_count == UINT32_MAX) {
        aph_log(APH_LOG_ERROR, "cloth",
                "Cloth limit reached: maximum supported cloths per world exceeded.");
        return APH_ERR_OUT_OF_MEMORY;
    }

    const size_t vec_bytes = float_count * sizeof(float);
    const size_t scalar_bytes = (size_t)cfg->vertex_count * sizeof(float);
    const size_t mark = aph_arena_mark(&w->arena);

    void *cloth_mem = NULL;
    void *pos_mem = NULL;
    void *prev_mem = NULL;
    void *vel_mem = NULL;
    void *invm_mem = NULL;

    aph_status status =
        aph_arena_alloc(&w->arena, sizeof(aph_cloth), APH_CLOTH_ARRAY_ALIGN, &cloth_mem);
    if (status == APH_OK) {
        status = aph_arena_alloc(&w->arena, vec_bytes, APH_CLOTH_ARRAY_ALIGN, &pos_mem);
    }
    if (status == APH_OK) {
        status = aph_arena_alloc(&w->arena, vec_bytes, APH_CLOTH_ARRAY_ALIGN, &prev_mem);
    }
    if (status == APH_OK) {
        status = aph_arena_alloc(&w->arena, vec_bytes, APH_CLOTH_ARRAY_ALIGN, &vel_mem);
    }
    if (status == APH_OK) {
        status = aph_arena_alloc(&w->arena, scalar_bytes, APH_CLOTH_ARRAY_ALIGN, &invm_mem);
    }

    if (status != APH_OK) {
        (void)aph_arena_rewind(&w->arena, mark);
        aph_log(APH_LOG_ERROR, "cloth",
                "Out of memory: world arena cannot fit cloth buffers. Increase memory_bytes in "
                "world config.");
        return status;
    }

    aph_cloth *c = (aph_cloth *)cloth_mem;
    c->id = w->cloth_count;
    c->vertex_count = cfg->vertex_count;
    c->mass = cfg->mass;
    c->damping = cfg->damping;
    c->thickness = cfg->thickness;
    c->default_inv_mass = inv_mass;
    c->positions = (float *)pos_mem;
    c->prev_positions = (float *)prev_mem;
    c->velocities = (float *)vel_mem;
    c->inv_masses = (float *)invm_mem;
    c->next = NULL;

    __builtin_memcpy(c->positions, cfg->positions, vec_bytes);
    __builtin_memcpy(c->prev_positions, cfg->positions, vec_bytes);
    for (uint32_t i = 0; i < cfg->vertex_count; ++i) {
        c->inv_masses[i] = inv_mass;
    }

    if (w->cloths_tail == NULL) {
        w->cloths_head = c;
        w->cloths_tail = c;
    } else {
        ((aph_cloth *)w->cloths_tail)->next = c;
        w->cloths_tail = c;
    }
    w->cloth_count++;
    w->substep_fn = aph_cloth_world_substep;
    w->hash_ext_fn = aph_cloth_world_hash;

    *out_cloth_id = c->id;
    return APH_OK;
}

aph_status aph_cloth_positions(const aph_world *w, uint32_t cloth_id, const float **out_positions,
                               uint32_t *out_vertex_count) {
    if (out_positions == NULL) {
        return APH_ERR_INVALID_ARGUMENT;
    }
    *out_positions = NULL;
    if (out_vertex_count != NULL) {
        *out_vertex_count = 0U;
    }

    const aph_cloth *c = find_cloth(w, cloth_id);
    if (c == NULL) {
        return APH_ERR_INVALID_ARGUMENT;
    }

    *out_positions = c->positions;
    if (out_vertex_count != NULL) {
        *out_vertex_count = c->vertex_count;
    }
    return APH_OK;
}

aph_status aph_cloth_pin(aph_world *w, uint32_t cloth_id, uint32_t vertex_index) {
    aph_cloth *c = find_cloth(w, cloth_id);
    if (c == NULL || vertex_index >= c->vertex_count) {
        return APH_ERR_INVALID_ARGUMENT;
    }

    c->inv_masses[vertex_index] = 0.0f;
    const size_t idx = (size_t)vertex_index * 3U;
    c->prev_positions[idx + 0U] = c->positions[idx + 0U];
    c->prev_positions[idx + 1U] = c->positions[idx + 1U];
    c->prev_positions[idx + 2U] = c->positions[idx + 2U];
    c->velocities[idx + 0U] = 0.0f;
    c->velocities[idx + 1U] = 0.0f;
    c->velocities[idx + 2U] = 0.0f;
    return APH_OK;
}

aph_status aph_cloth_unpin(aph_world *w, uint32_t cloth_id, uint32_t vertex_index) {
    aph_cloth *c = find_cloth(w, cloth_id);
    if (c == NULL || vertex_index >= c->vertex_count) {
        return APH_ERR_INVALID_ARGUMENT;
    }

    c->inv_masses[vertex_index] = c->default_inv_mass;
    return APH_OK;
}
