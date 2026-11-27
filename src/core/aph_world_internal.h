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

#ifndef APH_WORLD_INTERNAL_H
#define APH_WORLD_INTERNAL_H

#include "core/aph_arena.h"
#include "core/aph_world.h"
#include <stdint.h>

/**
 * Internal hook executed once per substep across subsystems registered on the world.
 */
typedef aph_status (*aph_world_substep_fn)(aph_world *w, float h);

/**
 * Internal hook executed during aph_world_hash() to include subsystem state.
 */
typedef uint64_t (*aph_world_hash_ext_fn)(const aph_world *w, uint64_t state);

struct aph_world {
    aph_arena arena;
    void *raw_memory;
    aph_world_config config;
    uint64_t step_count;
    void *cloths_head;
    void *cloths_tail;
    uint32_t cloth_count;
    aph_world_substep_fn substep_fn;
    aph_world_hash_ext_fn hash_ext_fn;
};

#endif /* APH_WORLD_INTERNAL_H */
