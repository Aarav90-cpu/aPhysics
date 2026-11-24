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

#include "core/aph_arena.h"
#include "tests/aph_test.h"
#include <stdalign.h>
#include <stdint.h>

int aph_test_failures = 0;

static void test_arena_init(void) {
    alignas(64) uint8_t buffer[256];
    aph_arena a;

    /* 1. Successful initialization with 64-byte aligned memory */
    aph_status status = aph_arena_init(&a, buffer, sizeof(buffer));
    APH_CHECK(status == APH_OK);
    APH_CHECK(a.base == buffer);
    APH_CHECK(a.capacity == sizeof(buffer));
    APH_CHECK(a.used == 0);
    APH_CHECK(aph_arena_mark(&a) == 0);

    /* 2. NULL arena or memory pointer */
    APH_CHECK(aph_arena_init(NULL, buffer, sizeof(buffer)) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(aph_arena_init(&a, NULL, sizeof(buffer)) == APH_ERR_INVALID_ARGUMENT);

    /* 3. Non-64-byte aligned memory is rejected */
    for (int offset = 1; offset < 64; ++offset) {
        status = aph_arena_init(&a, buffer + offset, sizeof(buffer) - (size_t)offset);
        APH_CHECK(status == APH_ERR_INVALID_ARGUMENT);
    }
}

static void test_arena_alloc_alignment(void) {
    alignas(64) uint8_t buffer[512];
    aph_arena a;
    aph_status status = aph_arena_init(&a, buffer, sizeof(buffer));
    APH_CHECK(status == APH_OK);

    const size_t test_alignments[] = {1, 2, 4, 8, 16, 32, 64};
    const size_t align_count = sizeof(test_alignments) / sizeof(test_alignments[0]);

    void *prev_end = NULL;
    for (size_t i = 0; i < align_count; ++i) {
        size_t align = test_alignments[i];
        /* Allocate odd sizes (e.g. 3, 5, 7 bytes) to force padding for subsequent allocations */
        size_t size = 3 + i * 2;
        void *ptr = NULL;

        status = aph_arena_alloc(&a, size, align, &ptr);
        APH_CHECK(status == APH_OK);
        APH_CHECK(ptr != NULL);
        APH_CHECK(((uintptr_t)ptr % align) == 0);

        if (prev_end != NULL) {
            APH_CHECK((uintptr_t)ptr >= (uintptr_t)prev_end);
        }
        prev_end = (void *)((uintptr_t)ptr + size);
    }
}

static void test_arena_alloc_invalid_arguments(void) {
    alignas(64) uint8_t buffer[128];
    aph_arena a;
    aph_status status = aph_arena_init(&a, buffer, sizeof(buffer));
    APH_CHECK(status == APH_OK);

    void *ptr = (void *)0x1234;

    /* 1. NULL arguments */
    APH_CHECK(aph_arena_alloc(NULL, 16, 8, &ptr) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(ptr == NULL);

    APH_CHECK(aph_arena_alloc(&a, 16, 8, NULL) == APH_ERR_INVALID_ARGUMENT);

    aph_arena uninit;
    uninit.base = NULL;
    uninit.capacity = 128;
    uninit.used = 0;
    APH_CHECK(aph_arena_alloc(&uninit, 16, 8, &ptr) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(ptr == NULL);

    /* 2. Invalid alignment values: 0, 3 (non-power of 2), 128 (> 64) */
    APH_CHECK(aph_arena_alloc(&a, 16, 0, &ptr) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(ptr == NULL);

    APH_CHECK(aph_arena_alloc(&a, 16, 3, &ptr) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(ptr == NULL);

    APH_CHECK(aph_arena_alloc(&a, 16, 128, &ptr) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(ptr == NULL);
}

static void test_arena_zero_fill_and_padding(void) {
    alignas(64) uint8_t buffer[256];
    /* Pre-poison buffer with non-zero bytes */
    for (size_t i = 0; i < sizeof(buffer); ++i) {
        buffer[i] = 0xAA;
    }

    aph_arena a;
    aph_status status = aph_arena_init(&a, buffer, sizeof(buffer));
    APH_CHECK(status == APH_OK);

    /* 1. Allocate 3 bytes with align 1 (consumes offsets 0..2) */
    void *ptr1 = NULL;
    status = aph_arena_alloc(&a, 3, 1, &ptr1);
    APH_CHECK(status == APH_OK);
    uint8_t *b1 = (uint8_t *)ptr1;
    for (size_t i = 0; i < 3; ++i) {
        APH_CHECK(b1[i] == 0x00);
    }

    /*
     * 2. Next allocation: 10 bytes with align 16.
     * Starts at offset 3, so padding of 13 bytes is required to reach offset 16.
     * Total span used advances by 13 (padding) + 10 (size) = 23 bytes (used becomes 26).
     * Verify all 13 padding bytes AND all 10 allocated bytes are zero-filled.
     */
    void *ptr2 = NULL;
    status = aph_arena_alloc(&a, 10, 16, &ptr2);
    APH_CHECK(status == APH_OK);
    APH_CHECK((uintptr_t)ptr2 == (uintptr_t)(buffer + 16));
    APH_CHECK(a.used == 26);

    for (size_t i = 0; i < 26; ++i) {
        APH_CHECK(buffer[i] == 0x00);
    }

    /* 3. Write non-zero pattern, rewind, and re-allocate: must re-zero */
    for (size_t i = 0; i < 26; ++i) {
        buffer[i] = 0x55;
    }
    status = aph_arena_rewind(&a, 0);
    APH_CHECK(status == APH_OK);
    APH_CHECK(a.used == 0);

    void *ptr3 = NULL;
    status = aph_arena_alloc(&a, 26, 16, &ptr3);
    APH_CHECK(status == APH_OK);
    uint8_t *b3 = (uint8_t *)ptr3;
    for (size_t i = 0; i < 26; ++i) {
        APH_CHECK(b3[i] == 0x00);
    }
}

static void test_arena_zero_size(void) {
    alignas(64) uint8_t buffer[128];
    aph_arena a;
    aph_status status = aph_arena_init(&a, buffer, sizeof(buffer));
    APH_CHECK(status == APH_OK);

    /* Zero-size allocation at start */
    void *ptr = NULL;
    status = aph_arena_alloc(&a, 0, 16, &ptr);
    APH_CHECK(status == APH_OK);
    APH_CHECK(ptr != NULL);
    APH_CHECK(((uintptr_t)ptr % 16) == 0);
    APH_CHECK(a.used == 0);

    /* Allocate some bytes */
    void *p1 = NULL;
    status = aph_arena_alloc(&a, 10, 8, &p1);
    APH_CHECK(status == APH_OK);
    APH_CHECK(a.used == 10);

    /* Zero-size allocation with align 32: leaves used unchanged */
    void *p2 = NULL;
    status = aph_arena_alloc(&a, 0, 32, &p2);
    APH_CHECK(status == APH_OK);
    APH_CHECK(p2 != NULL);
    APH_CHECK(((uintptr_t)p2 % 32) == 0);
    APH_CHECK(a.used == 10);
}

static void test_arena_exhaustion(void) {
    alignas(64) uint8_t buffer[64];
    aph_arena a;
    aph_status status = aph_arena_init(&a, buffer, 64);
    APH_CHECK(status == APH_OK);

    /* Exact numbers using align = 1 */
    void *p1 = NULL;
    status = aph_arena_alloc(&a, 32, 1, &p1);
    APH_CHECK(status == APH_OK);
    APH_CHECK(a.used == 32);

    /* Request 40 bytes: exceeds remaining 32 bytes */
    void *p2 = (void *)0x1234;
    status = aph_arena_alloc(&a, 40, 1, &p2);
    APH_CHECK(status == APH_ERR_OUT_OF_MEMORY);
    APH_CHECK(p2 == NULL);
    APH_CHECK(a.used == 32); /* Used is unchanged */

    /* Request exactly 32 bytes: fills the arena */
    void *p3 = NULL;
    status = aph_arena_alloc(&a, 32, 1, &p3);
    APH_CHECK(status == APH_OK);
    APH_CHECK(a.used == 64);

    /* Arena is full: even 1 byte fails */
    void *p4 = (void *)0x1234;
    status = aph_arena_alloc(&a, 1, 1, &p4);
    APH_CHECK(status == APH_ERR_OUT_OF_MEMORY);
    APH_CHECK(p4 == NULL);
    APH_CHECK(a.used == 64);

    /* Request with huge size does not overflow or wrap around */
    void *p_huge = (void *)0x1234;
    status = aph_arena_alloc(&a, (size_t)-1, 1, &p_huge);
    APH_CHECK(status == APH_ERR_OUT_OF_MEMORY);
    APH_CHECK(p_huge == NULL);
}

static void test_arena_mark_and_rewind(void) {
    alignas(64) uint8_t buffer[128];
    aph_arena a;
    aph_status status = aph_arena_init(&a, buffer, sizeof(buffer));
    APH_CHECK(status == APH_OK);

    void *p1 = NULL;
    status = aph_arena_alloc(&a, 16, 8, &p1);
    APH_CHECK(status == APH_OK);
    size_t mark1 = aph_arena_mark(&a);
    APH_CHECK(mark1 == 16);

    void *p2 = NULL;
    status = aph_arena_alloc(&a, 32, 8, &p2);
    APH_CHECK(status == APH_OK);
    size_t mark2 = aph_arena_mark(&a);
    APH_CHECK(mark2 == 48);

    /* Rewind to mark1 */
    status = aph_arena_rewind(&a, mark1);
    APH_CHECK(status == APH_OK);
    APH_CHECK(a.used == mark1);

    /* Re-allocate 32 bytes: gets same pointer as p2 */
    void *p2_re = NULL;
    status = aph_arena_alloc(&a, 32, 8, &p2_re);
    APH_CHECK(status == APH_OK);
    APH_CHECK(p2_re == p2);
    APH_CHECK(a.used == mark2);

    /* Rewind with mark > used must be rejected with APH_ERR_INVALID_ARGUMENT */
    status = aph_arena_rewind(&a, mark2 + 10);
    APH_CHECK(status == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(a.used == mark2); /* Unmodified */

    /* Rewind with NULL arena */
    APH_CHECK(aph_arena_rewind(NULL, 0) == APH_ERR_INVALID_ARGUMENT);
}

int main(void) {
    test_arena_init();
    test_arena_alloc_alignment();
    test_arena_alloc_invalid_arguments();
    test_arena_zero_fill_and_padding();
    test_arena_zero_size();
    test_arena_exhaustion();
    test_arena_mark_and_rewind();

    if (aph_test_failures == 0) {
        printf("arena_test: all checks passed.\n");
    } else {
        fprintf(stderr, "arena_test: %d check(s) failed.\n", aph_test_failures);
    }

    return aph_test_failures == 0 ? 0 : 1;
}
