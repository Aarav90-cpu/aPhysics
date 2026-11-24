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

#include "core/aph_hash.h"
#include "tests/aph_test.h"
#include <stdalign.h>
#include <stdint.h>

int aph_test_failures = 0;

/**
 * Tests against published official 64-bit FNV-1a test vectors by Glenn Fowler,
 * Landon Curt Noll, and Phong Vo (http://www.isthe.com/chongo/tech/comp/fnv/).
 */
static void test_hash_published_vectors(void) {
    /* 1. Empty string / zero-length data */
    APH_CHECK(aph_hash_bytes("", 0, 0) == 0xcbf29ce484222325ULL);
    APH_CHECK(aph_hash_bytes("", 0, APH_HASH_INIT) == 0xcbf29ce484222325ULL);
    APH_CHECK(aph_hash_bytes(NULL, 0, 0) == 0xcbf29ce484222325ULL);
    APH_CHECK(aph_hash_bytes(NULL, 0, 0x1234567890abcdefULL) == 0x1234567890abcdefULL);

    /* 2. "a" */
    APH_CHECK(aph_hash_bytes("a", 1, 0) == 0xaf63dc4c8601ec8cULL);

    /* 3. "foobar" */
    APH_CHECK(aph_hash_bytes("foobar", 6, 0) == 0x85944171f73967e8ULL);

    /* 4. "123456789" */
    APH_CHECK(aph_hash_bytes("123456789", 9, 0) == 0x06d5573923c6cdfcULL);

    /* 5. Landon Curt Noll's canonical test string */
    const char *chongo_str = "chongo <Landon Curt Noll> /\\../\\";
    APH_CHECK(aph_hash_bytes(chongo_str, 32, 0) == 0x2c8f4c9af81bcf06ULL);
}

/**
 * Verifies that a buffer split at every possible position hashes identically
 * to the whole continuous buffer.
 */
static void test_hash_split_positions_and_chaining(void) {
    const char text[] = "The quick brown fox jumps over the lazy dog";
    const size_t len = sizeof(text) - 1; /* 43 bytes */
    uint64_t whole_hash = aph_hash_bytes(text, len, 0);

    for (size_t split = 0; split <= len; ++split) {
        uint64_t h1 = aph_hash_bytes(text, split, 0);
        uint64_t h2 = aph_hash_bytes(text + split, len - split, h1);
        APH_CHECK(h2 == whole_hash);
    }

    /* Chaining from an explicit non-zero initial state */
    const uint64_t custom_state = 0xdeadbeefcafebabeULL;
    uint64_t whole_custom = aph_hash_bytes(text, len, custom_state);
    for (size_t split = 0; split <= len; ++split) {
        uint64_t h1 = aph_hash_bytes(text, split, custom_state);
        uint64_t h2 = aph_hash_bytes(text + split, len - split, h1);
        APH_CHECK(h2 == whole_custom);
    }
}

/**
 * Tests golden hash of a floating-point array.
 *
 * Expected hash computed independently using Python struct.pack('<f', ...) with 64-bit FNV-1a:
 *   floats = [0.0, -0.0, 1.0, -1.0, 0.5, 123.456, 1e-6, 1e6]
 *   packed = b"".join(struct.pack("<f", x) for x in floats)
 *   h = 0xcbf29ce484222325
 *   for b in packed: h = ((h ^ b) * 0x100000001b3) & 0xffffffffffffffff
 * Output: 0x5f7d4edf3b394c30ULL
 */
static void test_hash_float_array_golden(void) {
    const float test_floats[8] = {
        0.0f, -0.0f, 1.0f, -1.0f, 0.5f, 123.456f, 1e-6f, 1e6f,
    };
    uint64_t actual_hash = aph_hash_bytes(test_floats, sizeof(test_floats), 0);
    APH_CHECK(actual_hash == 0x5f7d4edf3b394c30ULL);
}

/**
 * Verifies that +0.0f and -0.0f evaluate as equal under IEEE 754 value semantics,
 * but produce distinct FNV-1a hashes because their bit representations differ.
 */
static void test_hash_float_zero_discrimination(void) {
    float pos_zero = 0.0f;
    float neg_zero = -0.0f;

    /* Numerical equality holds under IEEE 754 */
    APH_CHECK(pos_zero == neg_zero);

    /* Raw bits differ: 0x00000000 vs 0x80000000 */
    uint32_t u_pos = 0;
    uint32_t u_neg = 0;
    __builtin_memcpy(&u_pos, &pos_zero, sizeof(u_pos));
    __builtin_memcpy(&u_neg, &neg_zero, sizeof(u_neg));
    APH_CHECK(u_pos != u_neg);
    APH_CHECK(u_pos == 0x00000000U);
    APH_CHECK(u_neg == 0x80000000U);

    /* State hash detects the bitwise difference */
    uint64_t h_pos = aph_hash_bytes(&pos_zero, sizeof(pos_zero), 0);
    uint64_t h_neg = aph_hash_bytes(&neg_zero, sizeof(neg_zero), 0);
    APH_CHECK(h_pos != h_neg);
    APH_CHECK(h_pos == 0x4d25767f9dce13f5ULL);
    APH_CHECK(h_neg == 0x4d24f67f9dcd3a75ULL);
}

/**
 * Verifies that a 1-ULP change in a single float flips the resulting state hash.
 */
static void test_hash_float_1ulp_flip(void) {
    float base_val = 1.0f;
    uint64_t h_base = aph_hash_bytes(&base_val, sizeof(base_val), 0);

    /* Flip lowest significand bit by 1 ULP using __builtin_memcpy */
    uint32_t bits = 0;
    __builtin_memcpy(&bits, &base_val, sizeof(bits));
    bits += 1;
    float ulp_val = 0.0f;
    __builtin_memcpy(&ulp_val, &bits, sizeof(ulp_val));

    uint64_t h_ulp = aph_hash_bytes(&ulp_val, sizeof(ulp_val), 0);
    APH_CHECK(h_base != h_ulp);
}

/**
 * Verifies that unaligned pointers hash identically to aligned pointers.
 */
static void test_hash_unaligned_pointer(void) {
    alignas(64) uint8_t buffer[64];
    for (size_t i = 0; i < sizeof(buffer); ++i) {
        buffer[i] = (uint8_t)(i * 7 + 13);
    }

    const size_t test_len = 16;
    for (size_t offset = 0; offset < 8; ++offset) {
        uint64_t h_unaligned = aph_hash_bytes(buffer + offset, test_len, 0);

        /* Copy bytes to an aligned scratch array */
        alignas(64) uint8_t aligned_scratch[16];
        __builtin_memcpy(aligned_scratch, buffer + offset, test_len);
        uint64_t h_aligned = aph_hash_bytes(aligned_scratch, test_len, 0);

        APH_CHECK(h_unaligned == h_aligned);
    }
}

int main(void) {
    test_hash_published_vectors();
    test_hash_split_positions_and_chaining();
    test_hash_float_array_golden();
    test_hash_float_zero_discrimination();
    test_hash_float_1ulp_flip();
    test_hash_unaligned_pointer();

    if (aph_test_failures == 0) {
        printf("hash_test: all checks passed.\n");
    } else {
        fprintf(stderr, "hash_test: %d check(s) failed.\n", aph_test_failures);
    }

    return aph_test_failures == 0 ? 0 : 1;
}
