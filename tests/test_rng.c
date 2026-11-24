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

#include "core/aph_rng.h"
#include "tests/aph_test.h"

int aph_test_failures = 0;

/**
 * Verifies that aph_rng_next_u64 produces identical values to Sebastiano Vigna's
 * reference SplitMix64 C implementation (http://prng.di.unimi.it/splitmix64.c).
 */
static void test_splitmix64_reference_vectors(void) {
    aph_rng r0;
    aph_rng_seed(&r0, 0ULL);

    /* First 5 values for seed 0 from official SplitMix64 reference code */
    const uint64_t expected_seed0[5] = {
        0xe220a8397b1dcdafULL, 0x6e789e6aa1b965f4ULL, 0x06c45d188009454fULL,
        0xf88bb8a8724c81ecULL, 0x1b39896a51a8749bULL,
    };
    for (int i = 0; i < 5; ++i) {
        uint64_t val = aph_rng_next_u64(&r0);
        APH_CHECK(val == expected_seed0[i]);
    }

    aph_rng r1;
    aph_rng_seed(&r1, 1ULL);

    /* First 5 values for seed 1 from official SplitMix64 reference code */
    const uint64_t expected_seed1[5] = {
        0x910a2dec89025cc1ULL, 0xbeeb8da1658eec67ULL, 0xf893a2eefb32555eULL,
        0x71c18690ee42c90bULL, 0x71bb54d8d101b5b9ULL,
    };
    for (int i = 0; i < 5; ++i) {
        uint64_t val = aph_rng_next_u64(&r1);
        APH_CHECK(val == expected_seed1[i]);
    }
}

/**
 * Tests conversion of 64-bit integers to single-precision float in [0, 1) using
 * synthetic bit patterns.
 */
static void test_f32_synthetic_patterns(void) {
    /* 1. All zeros yields exactly 0.0f */
    float f_zero = aph_rng_f32_from_u64(0ULL);
    APH_CHECK(f_zero == 0.0f);

    /* 2. Top bit set (0x8000000000000000ULL): top 24 bits = 0x800000 (8388608), yields exactly 0.5f
     */
    float f_half = aph_rng_f32_from_u64(0x8000000000000000ULL);
    APH_CHECK(f_half == 0.5f);

    /* 3. All ones in top 24 bits (0xffffff0000000000ULL): 16777215 / 16777216 = 0.99999994f */
    float f_max_24 = aph_rng_f32_from_u64(0xffffff0000000000ULL);
    APH_CHECK(f_max_24 >= 0.0f);
    APH_CHECK(f_max_24 < 1.0f);
    APH_CHECK_NEAR(f_max_24, 0.99999994f, 1e-8f);

    /* 4. All 64 bits set to 1: top 24 bits are still 16777215, lower bits ignored */
    float f_all_ones = aph_rng_f32_from_u64(0xffffffffffffffffULL);
    APH_CHECK(f_all_ones >= 0.0f);
    APH_CHECK(f_all_ones < 1.0f);
    APH_CHECK(f_all_ones == f_max_24);

    /* 5. Alternating bit patterns */
    float f_alt1 = aph_rng_f32_from_u64(0xaaaaaaaaaaaaaaaaULL);
    APH_CHECK(f_alt1 >= 0.0f);
    APH_CHECK(f_alt1 < 1.0f);
    APH_CHECK_NEAR(f_alt1, (float)0xaaaaaa / 16777216.0f, 1e-8f);

    float f_alt2 = aph_rng_f32_from_u64(0x5555555555555555ULL);
    APH_CHECK(f_alt2 >= 0.0f);
    APH_CHECK(f_alt2 < 1.0f);
    APH_CHECK_NEAR(f_alt2, (float)0x555555 / 16777216.0f, 1e-8f);
}

/**
 * Golden regression tests for aph_rand_hash.
 *
 * Expected values were computed independently using an external Python script with explicit
 * 64-bit arithmetic masking:
 *   def splitmix64_mix(z):
 *       z = ((z ^ (z >> 30)) * 0xbf58476d1ce4e5b9) & 0xffffffffffffffff
 *       z = ((z ^ (z >> 27)) * 0x94d049bb133111eb) & 0xffffffffffffffff
 *       return (z ^ (z >> 31)) & 0xffffffffffffffff
 *   def rand_hash(seed, a, b):
 *       C = 0x9e3779b97f4a7c15
 *       h = splitmix64_mix((seed + (a * C)) & 0xffffffffffffffff)
 *       return splitmix64_mix((h + ((b + 1) * C)) & 0xffffffffffffffff)
 *
 * These golden outputs are frozen: any change to them alters all baked physics simulations.
 */
static void test_rand_hash_golden_values(void) {
    typedef struct {
        uint64_t seed;
        uint64_t a;
        uint64_t b;
        uint64_t expected;
    } golden_case;

    const golden_case cases[] = {
        {0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
         0xe220a8397b1dcdafULL},
        {0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000001ULL,
         0x6e789e6aa1b965f4ULL},
        {0x0000000000000000ULL, 0x0000000000000001ULL, 0x0000000000000000ULL,
         0xa706dd2f4d197e6fULL},
        {0x0000000000000000ULL, 0x0000000000000001ULL, 0x0000000000000001ULL,
         0xb382a305f4414f5eULL},
        {0x0000000000003039ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
         0x7fb6fc5796d17578ULL},
        {0x0000000000003039ULL, 0x0000000000000064ULL, 0x0000000000000032ULL,
         0x9bc5541a865ef877ULL},
        {0xdeadbeefcafebabeULL, 0x000000000000002aULL, 0x00000000000003e7ULL,
         0x47e3ecc07a02e493ULL},
        {0xffffffffffffffffULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL,
         0x4dab2c1ce9210c4cULL},
    };

    const size_t count = sizeof(cases) / sizeof(cases[0]);
    for (size_t i = 0; i < count; ++i) {
        uint64_t actual = aph_rand_hash(cases[i].seed, cases[i].a, cases[i].b);
        APH_CHECK(actual == cases[i].expected);
    }
}

/**
 * Relation test: aph_rand_hash(seed, a, b) must equal the (b+1)-th output of an
 * aph_rng stateful generator seeded with aph_splitmix64_mix(seed + a * 0x9e3779b97f4a7c15ULL).
 */
static void test_relation_stateless_and_stateful(void) {
    const uint64_t test_seeds[] = {0ULL, 42ULL, 0x123456789abcdef0ULL};
    const uint64_t test_a_coords[] = {0ULL, 1ULL, 55ULL};

    for (size_t s_idx = 0; s_idx < 3; ++s_idx) {
        uint64_t seed = test_seeds[s_idx];
        for (size_t a_idx = 0; a_idx < 3; ++a_idx) {
            uint64_t a = test_a_coords[a_idx];

            /* Initialize stateful generator with the stream intermediate seed */
            uint64_t intermediate_seed = aph_splitmix64_mix(seed + a * 0x9e3779b97f4a7c15ULL);
            aph_rng r;
            aph_rng_seed(&r, intermediate_seed);

            /* For each b, the next output of the stream must match aph_rand_hash */
            for (uint64_t b = 0; b < 10; ++b) {
                uint64_t seq_val = aph_rng_next_u64(&r);
                uint64_t hash_val = aph_rand_hash(seed, a, b);
                APH_CHECK(seq_val == hash_val);

                /* Float outputs must also match identically */
                float seq_f = aph_rng_f32_from_u64(seq_val);
                float hash_f = aph_rand_f32(seed, a, b);
                APH_CHECK(seq_f == hash_f);
            }
        }
    }
}

/**
 * Verifies that aph_rand_hash is order-independent (stateless).
 *
 * Notice:
 *   Evaluating in reverse order only confirms the absence of hidden mutable state.
 *   It is not a proof of statistical quality or entropy.
 */
static void test_rand_hash_order_independence(void) {
    const uint64_t seed = 0xcafebabedeadbeefULL;
    const uint64_t step = 42ULL;
    const int count = 100;

    uint64_t forward_vals[100];
    for (int b = 0; b < count; ++b) {
        forward_vals[b] = aph_rand_hash(seed, step, (uint64_t)b);
    }

    /* Evaluate in reverse order */
    for (int b = count - 1; b >= 0; --b) {
        uint64_t rev_val = aph_rand_hash(seed, step, (uint64_t)b);
        APH_CHECK(rev_val == forward_vals[b]);
    }

    /* Asymmetry check: swapping coordinates produces different values */
    APH_CHECK(aph_rand_hash(seed, 10ULL, 20ULL) != aph_rand_hash(seed, 20ULL, 10ULL));
}

/**
 * Checks that aph_rng_next_f32 and aph_rand_f32 stay strictly in [0, 1) across
 * large iterations.
 */
static void test_f32_range_invariants(void) {
    aph_rng r;
    aph_rng_seed(&r, 0x1337c0deULL);

    for (int i = 0; i < 100000; ++i) {
        float v = aph_rng_next_f32(&r);
        APH_CHECK(v >= 0.0f);
        APH_CHECK(v < 1.0f);
    }

    const uint64_t seed = 0x987654321ULL;
    for (uint64_t i = 0; i < 10000; ++i) {
        float vf = aph_rand_f32(seed, i, i + 1ULL);
        APH_CHECK(vf >= 0.0f);
        APH_CHECK(vf < 1.0f);
    }
}

/**
 * Sanity check: verifies that the empirical mean of 100,000 float samples from
 * aph_rng_next_f32 is close to the expected mean (0.5).
 *
 * Notice:
 *   This check only guards against gross systematic scaling defects or truncation bugs.
 *   It is a sanity check, not a statistical certification of uniformity.
 */
static void test_f32_mean_sanity_check(void) {
    aph_rng r;
    aph_rng_seed(&r, 1337ULL);

    double sum = 0.0;
    const int count = 100000;
    for (int i = 0; i < count; ++i) {
        sum += (double)aph_rng_next_f32(&r);
    }

    double mean = sum / (double)count;
    APH_CHECK_NEAR((float)mean, 0.5f, 0.01f);
}

int main(void) {
    test_splitmix64_reference_vectors();
    test_f32_synthetic_patterns();
    test_rand_hash_golden_values();
    test_relation_stateless_and_stateful();
    test_rand_hash_order_independence();
    test_f32_range_invariants();
    test_f32_mean_sanity_check();

    if (aph_test_failures == 0) {
        printf("rng_test: all checks passed.\n");
    } else {
        fprintf(stderr, "rng_test: %d check(s) failed.\n", aph_test_failures);
    }

    return aph_test_failures == 0 ? 0 : 1;
}
