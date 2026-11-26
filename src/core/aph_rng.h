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

#ifndef APH_RNG_H
#define APH_RNG_H

#include <stddef.h>
#include <stdint.h>

#ifndef __wasm__
#include <assert.h>
#define APH_RNG_ASSERT(cond) assert(cond)
#else
#define APH_RNG_ASSERT(cond) ((void)0)
#endif

/**
 * 64-bit pseudo-random number generators based on SplitMix64.
 *
 * This module provides two generation models:
 *   1. Sequential stateful generator (aph_rng):
 *      For setup routines, procedural mesh variation, and local non-concurrent streams.
 *   2. Stateless counter-based hash generator (aph_rand_hash, aph_rand_f32):
 *      For evaluating perturbations (e.g. wind gusts, tear thresholds) directly as a pure
 *      function of (step, element) coordinates without advancing or sharing a mutable state.
 *
 * Algorithm and Reference:
 *   The core mixing transform is David Stafford's Mix13 variant of MurmurHash3, utilized
 *   by Sebastiano Vigna in the reference SplitMix64 generator (2015, CC0 1.0 Universal /
 *   Public Domain, http://prng.di.unimi.it/splitmix64.c), which is derived from Java 8's
 *   SplittableRandom (Guy L. Steele Jr., Doug Lea, Christine H. Flood, OOPSLA 2014).
 *
 * Security and Statistical Quality Notice:
 *   - NOT cryptographically secure. Never use for cryptographic keys or security tokens.
 *   - No statistical quality claim is made beyond passing the verified test suite in
 *     tests/test_rng.c and passing BigCrush as reported in reference literature.
 *
 * Domain Separation Notice:
 *   When multiple simulation subsystems (such as aerodynamic wind gusting and cloth tearing
 *   thresholds) share the same world seed, calling aph_rand_hash(seed, step, element) with
 *   identical parameters produces identical pseudo-random values in both systems. To prevent
 *   subsystem cross-talk, each subsystem must derive its own distinct domain seed first,
 *   for example via aph_rand_hash(world_seed, DOMAIN_ID, 0).
 */

/**
 * Core 64-bit mixer based on David Stafford's Mix13 variant used in SplitMix64.
 *
 * Applies shift-xor-multiplication rounds to provide full bit diffusion across 64-bit inputs.
 *
 * Inputs:
 *   z: 64-bit integer to mix.
 *
 * Returns:
 *   Mixed 64-bit unsigned integer.
 */
static inline uint64_t aph_splitmix64_mix(uint64_t z) {
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

/**
 * Converts a 64-bit integer into a single-precision float strictly in the half-open interval [0,
 * 1).
 *
 * Extracts the most significant 24 bits (u >> 40), yielding an integer in [0, 16777215],
 * and multiplies by 2^-24 (1.0f / 16777216.0f). Because 2^-24 is an exact power of two in
 * IEEE 754 single-precision float, scaling adjusts the binary exponent without mantissa rounding.
 *
 * Output bounds:
 *   Minimum: 0.0f (when top 24 bits are 0).
 *   Maximum: 0.99999994f (0x3f7fffff, when top 24 bits are all 1s).
 *   Guaranteed strictly in [0, 1). Never returns 1.0f or negative values.
 *
 * Inputs:
 *   u: 64-bit unsigned integer.
 *
 * Returns:
 *   Single-precision float in [0, 1).
 */
static inline float aph_rng_f32_from_u64(uint64_t u) {
    uint32_t bits24 = (uint32_t)(u >> 40);
    return (float)bits24 * (1.0f / 16777216.0f);
}

/**
 * Stateful pseudo-random number generator instance.
 */
typedef struct aph_rng {
    uint64_t state;
} aph_rng;

/**
 * Initializes a stateful generator instance with a 64-bit seed.
 *
 * Preconditions:
 *   r must not be NULL.
 *
 * Inputs:
 *   r: Pointer to generator instance.
 *   seed: 64-bit seed. Any 64-bit value is valid, including 0.
 */
static inline void aph_rng_seed(aph_rng *r, uint64_t seed) {
    APH_RNG_ASSERT(r != NULL);
    r->state = seed;
}

/**
 * Advances the generator and returns the next 64-bit pseudo-random unsigned integer.
 *
 * Increments internal state by the golden ratio constant 0x9e3779b97f4a7c15ULL and passes
 * the updated state through aph_splitmix64_mix().
 *
 * Preconditions:
 *   r must not be NULL.
 *
 * Inputs:
 *   r: Pointer to generator instance.
 *
 * Returns:
 *   Next pseudo-random 64-bit unsigned integer.
 */
static inline uint64_t aph_rng_next_u64(aph_rng *r) {
    APH_RNG_ASSERT(r != NULL);
    uint64_t z = (r->state += 0x9e3779b97f4a7c15ULL);
    return aph_splitmix64_mix(z);
}

/**
 * Advances the generator and returns the next single-precision float in [0, 1).
 *
 * Preconditions:
 *   r must not be NULL.
 *
 * Inputs:
 *   r: Pointer to generator instance.
 *
 * Returns:
 *   Next pseudo-random float strictly in [0, 1).
 */
static inline float aph_rng_next_f32(aph_rng *r) {
    return aph_rng_f32_from_u64(aph_rng_next_u64(r));
}

/**
 * Computes a deterministic 64-bit pseudo-random hash from a seed and two 64-bit coordinates.
 *
 * Implements Option A (chained two-stage mixing):
 *   1. Mixes seed with coordinate a (e.g. simulation step):
 *        h = aph_splitmix64_mix(seed + a * 0x9e3779b97f4a7c15ULL)
 *   2. Mixes intermediate state h with coordinate b (e.g. element index):
 *        res = aph_splitmix64_mix(h + (b + 1ULL) * 0x9e3779b97f4a7c15ULL)
 *
 * Mathematical Relation:
 *   aph_rand_hash(seed, a, b) equals the (b+1)-th output of an aph_rng stateful generator
 *   initialized with seed aph_splitmix64_mix(seed + a * 0x9e3779b97f4a7c15ULL).
 *
 * Frozen Algorithm Notice:
 *   This formula is frozen and guarded by golden unit tests. Modifying this formula
 *   or its constants changes the output of all seeded simulation bakes and requires
 *   an explicit changelog entry.
 *
 * Inputs:
 *   seed: 64-bit base seed or subsystem domain seed.
 *   a: First coordinate (e.g. simulation step or frame index).
 *   b: Second coordinate (e.g. particle or vertex index).
 *
 * Returns:
 *   Deterministic 64-bit pseudo-random value.
 */
static inline uint64_t aph_rand_hash(uint64_t seed, uint64_t a, uint64_t b) {
    uint64_t h = aph_splitmix64_mix(seed + a * 0x9e3779b97f4a7c15ULL);
    return aph_splitmix64_mix(h + (b + 1ULL) * 0x9e3779b97f4a7c15ULL);
}

/**
 * Computes a deterministic single-precision float in [0, 1) from a seed and two coordinates.
 *
 * Converts aph_rand_hash(seed, a, b) via aph_rng_f32_from_u64().
 *
 * Inputs:
 *   seed: 64-bit base seed or subsystem domain seed.
 *   a: First coordinate.
 *   b: Second coordinate.
 *
 * Returns:
 *   Deterministic float strictly in [0, 1).
 */
static inline float aph_rand_f32(uint64_t seed, uint64_t a, uint64_t b) {
    return aph_rng_f32_from_u64(aph_rand_hash(seed, a, b));
}

#endif /* APH_RNG_H */
