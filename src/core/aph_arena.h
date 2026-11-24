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

#ifndef APH_ARENA_H
#define APH_ARENA_H

#include "core/aph_status.h"
#include <stddef.h>
#include <stdint.h>

/**
 * Maximum supported alignment in bytes.
 *
 * Guarantees compatibility with standard hardware vector and cache line boundaries
 * (including AVX-512, ARM NEON, and WebAssembly SIMD128).
 */
#define APH_ARENA_MAX_ALIGN 64

/**
 * Linear memory arena allocator.
 *
 * Architectural Invariant:
 *   Persistent allocations only happen during world and object creation.
 *   aph_arena_mark() and aph_arena_rewind() are strictly intended for temporary
 *   scratch memory during creation routines (for example, temporary edge-building
 *   sort buffers).
 *
 *   The simulation step loop (aph_world_step) must never allocate memory.
 *   All simulation buffers (vertex positions, velocities, constraints, BVH nodes)
 *   must be fully pre-allocated at creation time.
 */
typedef struct aph_arena {
    uint8_t *base;
    size_t capacity;
    size_t used;
} aph_arena;

/**
 * Initializes an arena allocator with a pre-allocated memory buffer.
 *
 * Preconditions:
 *   a must not be NULL.
 *   memory must be aligned to a 64-byte boundary.
 *
 * Inputs:
 *   a: Pointer to arena structure to initialize.
 *   memory: Pointer to backing memory buffer. Must be 64-byte aligned.
 *   capacity: Total size of the backing buffer in bytes.
 *
 * Returns:
 *   APH_OK on success.
 *   APH_ERR_INVALID_ARGUMENT if a or memory is NULL, or if memory is not 64-byte aligned.
 */
static inline aph_status aph_arena_init(aph_arena *a, void *memory, size_t capacity) {
    if (a == NULL || memory == NULL) {
        return APH_ERR_INVALID_ARGUMENT;
    }
    if (((uintptr_t)memory & (APH_ARENA_MAX_ALIGN - 1)) != 0) {
        return APH_ERR_INVALID_ARGUMENT;
    }
    a->base = (uint8_t *)memory;
    a->capacity = capacity;
    a->used = 0;
    return APH_OK;
}

/**
 * Allocates a contiguous block of memory with guaranteed alignment and zero-fill.
 *
 * Both the alignment padding bytes and the requested payload bytes are zero-filled
 * via __builtin_memset to ensure memory is never left undefined and memory dumps
 * remain deterministic.
 *
 * Inputs:
 *   a: Pointer to arena structure.
 *   size: Number of bytes to allocate. If 0, succeeds with a non-NULL aligned pointer
 *         and leaves arena used offset unchanged.
 *   align: Required alignment in bytes. Must be a power of two between 1 and 64 inclusive.
 *   out_ptr: Output pointer to receive the allocated address. Set to NULL on error.
 *
 * Returns:
 *   APH_OK on success.
 *   APH_ERR_INVALID_ARGUMENT if a or out_ptr is NULL, or if align is not a valid power of two.
 *   APH_ERR_OUT_OF_MEMORY if the requested size and padding exceed remaining capacity.
 */
static inline aph_status aph_arena_alloc(aph_arena *a, size_t size, size_t align, void **out_ptr) {
    if (out_ptr == NULL) {
        return APH_ERR_INVALID_ARGUMENT;
    }
    *out_ptr = NULL;

    if (a == NULL || a->base == NULL) {
        return APH_ERR_INVALID_ARGUMENT;
    }
    if (align == 0 || align > APH_ARENA_MAX_ALIGN || (align & (align - 1)) != 0) {
        return APH_ERR_INVALID_ARGUMENT;
    }

    uintptr_t current_addr = (uintptr_t)(a->base + a->used);
    size_t padding = (size_t)((0 - current_addr) & (uintptr_t)(align - 1));

    size_t remaining = a->capacity - a->used;
    if (padding > remaining) {
        return APH_ERR_OUT_OF_MEMORY;
    }
    if (size > remaining - padding) {
        return APH_ERR_OUT_OF_MEMORY;
    }

    uint8_t *ptr = a->base + a->used + padding;

    if (size == 0) {
        *out_ptr = (void *)ptr;
        return APH_OK;
    }

    /* Zero-fill both padding bytes and allocated payload */
    __builtin_memset(a->base + a->used, 0, padding + size);
    a->used += padding + size;
    *out_ptr = (void *)ptr;
    return APH_OK;
}

/**
 * Returns the current allocation offset within the arena.
 *
 * Inputs:
 *   a: Pointer to arena structure.
 *
 * Returns:
 *   Current byte offset (0 if a is NULL).
 */
static inline size_t aph_arena_mark(const aph_arena *a) {
    if (a == NULL) {
        return 0;
    }
    return a->used;
}

/**
 * Rewinds the arena to a previously recorded mark, releasing all allocations made after the mark.
 *
 * Notice:
 *   If mark > a->used, the function fails with APH_ERR_INVALID_ARGUMENT.
 *   Note that a stale mark that happens to be below or equal to a->used cannot be
 *   detected; callers must ensure marks correspond to the appropriate allocation scope.
 *
 * Inputs:
 *   a: Pointer to arena structure.
 *   mark: Previous allocation offset returned by aph_arena_mark().
 *
 * Returns:
 *   APH_OK on success.
 *   APH_ERR_INVALID_ARGUMENT if a is NULL or mark > a->used.
 */
static inline aph_status aph_arena_rewind(aph_arena *a, size_t mark) {
    if (a == NULL) {
        return APH_ERR_INVALID_ARGUMENT;
    }
    if (mark > a->used) {
        return APH_ERR_INVALID_ARGUMENT;
    }
    a->used = mark;
    return APH_OK;
}

#endif /* APH_ARENA_H */
