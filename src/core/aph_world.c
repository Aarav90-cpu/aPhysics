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

#include "core/aph_world.h"
#include "core/aph_arena.h"
#include "core/aph_hash.h"
#include "core/aph_log.h"
#include "core/aph_world_internal.h"
#include <math.h>
#include <stdlib.h>

/*
 * Host allocation functions.
 *
 * In native builds, memory is allocated and freed via malloc and free.
 * Notice: Step 3.2 replaces these functions for freestanding WebAssembly builds.
 */
static void *aph_world_host_alloc(size_t size) {
    return malloc(size);
}

static void aph_world_host_free(void *ptr) {
    free(ptr);
}

aph_status aph_world_create(const aph_world_config *cfg, aph_world **out_world) {
    if (out_world == NULL) {
        return APH_ERR_INVALID_ARGUMENT;
    }
    *out_world = NULL;

    if (cfg == NULL) {
        aph_log(APH_LOG_ERROR, "world",
                "World configuration pointer is NULL. Pass a valid aph_world_config pointer.");
        return APH_ERR_INVALID_ARGUMENT;
    }

    if (cfg->fixed_dt <= 0.0f || !isfinite(cfg->fixed_dt) ||
        cfg->fixed_dt > APH_WORLD_MAX_FIXED_DT) {
        aph_log(APH_LOG_ERROR, "world",
                "Invalid fixed_dt: time step must be positive, finite, and <= 1.0 s. Set fixed_dt "
                "(e.g. 1/60 s) in world config.");
        return APH_ERR_INVALID_ARGUMENT;
    }

    if (cfg->substeps == 0 || cfg->substeps > APH_WORLD_MAX_SUBSTEPS) {
        aph_log(APH_LOG_ERROR, "world",
                "Invalid substeps: must be between 1 and 1000. Set substeps (e.g. 8) in world "
                "config.");
        return APH_ERR_INVALID_ARGUMENT;
    }

    if (!isfinite(cfg->gravity.x) || !isfinite(cfg->gravity.y) || !isfinite(cfg->gravity.z)) {
        aph_log(APH_LOG_ERROR, "world",
                "Invalid gravity: vector components must be finite numbers. Check gravity in world "
                "config.");
        return APH_ERR_INVALID_ARGUMENT;
    }

    if (cfg->memory_bytes < APH_WORLD_MIN_MEMORY_BYTES) {
        aph_log(APH_LOG_ERROR, "world",
                "Memory budget too small: must be at least 1024 bytes. Increase memory_bytes in "
                "world config.");
        return APH_ERR_INVALID_ARGUMENT;
    }

    const size_t header_and_pad = sizeof(aph_world) + APH_ARENA_MAX_ALIGN;
    if (cfg->memory_bytes > SIZE_MAX - header_and_pad) {
        aph_log(APH_LOG_ERROR, "world",
                "Invalid memory_bytes: size causes integer overflow. Reduce memory_bytes in world "
                "config.");
        return APH_ERR_INVALID_ARGUMENT;
    }

    const size_t total_size = header_and_pad + cfg->memory_bytes;
    void *raw = aph_world_host_alloc(total_size);
    if (raw == NULL) {
        aph_log(APH_LOG_ERROR, "world",
                "Out of memory: host failed to allocate world memory budget. Reduce memory_bytes "
                "in world config or free system memory.");
        return APH_ERR_OUT_OF_MEMORY;
    }

    /* Zero-fill entire allocated world block */
    __builtin_memset(raw, 0, total_size);

    aph_world *w = (aph_world *)raw;
    w->raw_memory = raw;
    w->config = *cfg;
    w->step_count = 0;

    /* Align arena start to 64-byte boundary */
    uintptr_t unaligned_arena = (uintptr_t)raw + sizeof(aph_world);
    uintptr_t aligned_arena =
        (unaligned_arena + (APH_ARENA_MAX_ALIGN - 1)) & ~((uintptr_t)(APH_ARENA_MAX_ALIGN - 1));

    aph_status status = aph_arena_init(&w->arena, (void *)aligned_arena, cfg->memory_bytes);
    if (status != APH_OK) {
        aph_world_host_free(raw);
        return status;
    }

    *out_world = w;
    return APH_OK;
}

void aph_world_destroy(aph_world *w) {
    if (w == NULL) {
        return;
    }
    aph_world_host_free(w->raw_memory);
}

aph_status aph_world_step(aph_world *w) {
    if (w == NULL) {
        return APH_ERR_INVALID_ARGUMENT;
    }
    if (w->substep_fn != NULL) {
        float h = w->config.fixed_dt / (float)w->config.substeps;
        for (uint32_t s = 0; s < w->config.substeps; ++s) {
            aph_status st = w->substep_fn(w, h);
            if (st != APH_OK) {
                return st;
            }
        }
    }
    w->step_count++;
    return APH_OK;
}

uint64_t aph_world_step_count(const aph_world *w) {
    if (w == NULL) {
        return 0;
    }
    return w->step_count;
}

uint64_t aph_world_hash(const aph_world *w) {
    if (w == NULL) {
        return 0;
    }
    uint64_t h = APH_HASH_INIT;
    h = aph_hash_bytes(&w->config.seed, sizeof(w->config.seed), h);
    h = aph_hash_bytes(&w->step_count, sizeof(w->step_count), h);
    h = aph_hash_bytes(&w->config.fixed_dt, sizeof(w->config.fixed_dt), h);
    h = aph_hash_bytes(&w->config.substeps, sizeof(w->config.substeps), h);
    h = aph_hash_bytes(&w->config.gravity.x, sizeof(w->config.gravity.x), h);
    h = aph_hash_bytes(&w->config.gravity.y, sizeof(w->config.gravity.y), h);
    h = aph_hash_bytes(&w->config.gravity.z, sizeof(w->config.gravity.z), h);
    if (w->hash_ext_fn != NULL) {
        h = w->hash_ext_fn(w, h);
    }
    return h;
}
