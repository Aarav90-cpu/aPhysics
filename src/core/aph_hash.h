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

#ifndef APH_HASH_H
#define APH_HASH_H

#if !defined(__BYTE_ORDER__) || !defined(__ORDER_LITTLE_ENDIAN__) ||                               \
    (__BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__)
#error "aph_hash.h requires a little-endian target"
#endif

#include <stddef.h>
#include <stdint.h>

/**
 * 64-bit Fowler-Noll-Vo (FNV-1a) initial offset basis.
 */
#define APH_FNV1A64_OFFSET_BASIS 0xcbf29ce484222325ULL

/**
 * Initial hash state constant for starting a new 64-bit FNV-1a hash.
 */
#define APH_HASH_INIT APH_FNV1A64_OFFSET_BASIS

/**
 * 64-bit FNV prime (2^40 + 2^8 + 0xb3).
 */
#define APH_FNV1A64_PRIME 0x100000001b3ULL

/**
 * Computes or continues a 64-bit Fowler-Noll-Vo (FNV-1a) hash over a buffer of raw bytes.
 *
 * Purpose:
 *   Detects accidental numerical divergence in simulation state across steps, builds,
 *   and platforms (e.g. Linux, macOS, and WebAssembly).
 *
 * Notice and Limitations:
 *   - NOT cryptographically secure. It is intended solely to detect accidental divergence,
 *     not to protect against adversarial collisions or tampering.
 *   - No performance claim is made; this byte-by-byte loop is intended for verification and state
 * hashing.
 *   - Platform portability: hashes of raw float arrays are only comparable across little-endian
 * targets.
 *   - NaN bit representations: NaN encodings frequently differ between CPU architectures (e.g. x86
 * vs ARM). State hashes are only meaningful if the simulation state contains no NaNs. Call
 * aph_world_validate() first to verify state sanity before hashing.
 *   - Struct padding: never hash whole structs directly, because struct padding bytes contain
 * undefined values. Hash individual fields or contiguous flat arrays of floats (e.g. vertex
 * positions, velocities).
 *   - Preconditions: data may be NULL only if size == 0.
 *
 * State Chaining and Special Case:
 *   Passing state == 0 initializes the hash with APH_HASH_INIT (0xcbf29ce484222325ULL).
 *   To chain multiple buffers, pass the output of the previous aph_hash_bytes call as state.
 *   Caveat: if a running hash state ever evaluates to exactly 0 (which occurs with an approximate
 *   probability of 1 in 2^64), passing it to a subsequent call will restart the hash from
 * APH_HASH_INIT.
 *
 * Inputs:
 *   data: Pointer to bytes to hash. May be NULL only if size == 0.
 *   size: Number of bytes to hash.
 *   state: Running hash state from a previous call, or 0 (or APH_HASH_INIT) to start a new hash.
 *
 * Returns:
 *   Updated 64-bit FNV-1a hash value.
 */
static inline uint64_t aph_hash_bytes(const void *data, size_t size, uint64_t state) {
    uint64_t h = (state == 0) ? APH_HASH_INIT : state;
    if (data == NULL || size == 0) {
        return h;
    }
    const uint8_t *bytes = (const uint8_t *)data;
    for (size_t i = 0; i < size; ++i) {
        h = (h ^ (uint64_t)bytes[i]) * APH_FNV1A64_PRIME;
    }
    return h;
}

#endif /* APH_HASH_H */
