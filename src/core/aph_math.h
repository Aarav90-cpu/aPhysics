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

#ifndef APH_MATH_H
#define APH_MATH_H

#include <math.h>

/**
 * Squared length threshold below which a vector is treated as zero during normalization.
 *
 * (1e-6 m)^2 = 1e-12 m^2. Vectors shorter than approximately 1 micrometre are
 * normalized to the zero vector to avoid numerical instability or division by near-zero.
 */
#define APH_VEC3_NORMALIZE_EPSILON_SQ 1e-12f

/**
 * 3D single-precision floating-point vector.
 */
typedef struct aph_vec3 {
    float x;
    float y;
    float z;
} aph_vec3;

/**
 * Constructs a 3D vector from component values.
 *
 * Inputs:
 *   x: X component.
 *   y: Y component.
 *   z: Z component.
 *
 * Returns:
 *   Vector with the specified components.
 */
static inline aph_vec3 aph_vec3_make(float x, float y, float z) {
    aph_vec3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

/**
 * Adds two 3D vectors.
 *
 * Inputs:
 *   a: First vector operand.
 *   b: Second vector operand.
 *
 * Returns:
 *   Component-wise sum a + b.
 */
static inline aph_vec3 aph_vec3_add(aph_vec3 a, aph_vec3 b) {
    return aph_vec3_make(a.x + b.x, a.y + b.y, a.z + b.z);
}

/**
 * Subtracts vector b from vector a.
 *
 * Inputs:
 *   a: Minuend vector.
 *   b: Subtrahend vector.
 *
 * Returns:
 *   Component-wise difference a - b.
 */
static inline aph_vec3 aph_vec3_sub(aph_vec3 a, aph_vec3 b) {
    return aph_vec3_make(a.x - b.x, a.y - b.y, a.z - b.z);
}

/**
 * Multiplies a 3D vector by a scalar.
 *
 * Inputs:
 *   v: Vector to scale.
 *   s: Scalar scaling factor.
 *
 * Returns:
 *   Vector with each component multiplied by s.
 */
static inline aph_vec3 aph_vec3_scale(aph_vec3 v, float s) {
    return aph_vec3_make(v.x * s, v.y * s, v.z * s);
}

/**
 * Computes the dot (scalar) product of two 3D vectors.
 *
 * Inputs:
 *   a: First vector operand.
 *   b: Second vector operand.
 *
 * Returns:
 *   Scalar product (a.x * b.x + a.y * b.y + a.z * b.z).
 */
static inline float aph_vec3_dot(aph_vec3 a, aph_vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

/**
 * Computes the cross product of two 3D vectors in a right-handed coordinate system.
 *
 * Inputs:
 *   a: First vector operand.
 *   b: Second vector operand.
 *
 * Returns:
 *   Cross product vector a x b perpendicular to both a and b.
 */
static inline aph_vec3 aph_vec3_cross(aph_vec3 a, aph_vec3 b) {
    return aph_vec3_make(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

/**
 * Computes the squared Euclidean length (magnitude) of a 3D vector.
 *
 * Inputs:
 *   v: Vector operand.
 *
 * Returns:
 *   Squared length (x^2 + y^2 + z^2).
 */
static inline float aph_vec3_length_sq(aph_vec3 v) {
    return aph_vec3_dot(v, v);
}

/**
 * Computes the Euclidean length (magnitude) of a 3D vector.
 *
 * Square root is computed with sqrtf(), which is correctly rounded under IEEE 754
 * and deterministic across supported platforms.
 *
 * Inputs:
 *   v: Vector operand.
 *
 * Returns:
 *   Length sqrt(x^2 + y^2 + z^2).
 */
static inline float aph_vec3_length(aph_vec3 v) {
    return sqrtf(aph_vec3_length_sq(v));
}

/**
 * Normalizes a 3D vector to unit length, or returns a zero vector if too short.
 *
 * Checks if length squared <= APH_VEC3_NORMALIZE_EPSILON_SQ (approx. 1 micrometre).
 * Using the <= comparison ensures that NaN inputs do not evaluate as <= epsilon and
 * therefore propagate as NaN rather than being silently suppressed to zero.
 *
 * Normalization is performed by dividing each component directly by the length
 * rather than multiplying by an approximate reciprocal, preserving reproducible
 * golden state hashes.
 *
 * Inputs:
 *   v: Vector to normalize.
 *
 * Returns:
 *   Unit vector v / |v|, or (0, 0, 0) if |v|^2 <= APH_VEC3_NORMALIZE_EPSILON_SQ.
 *   If v contains NaN, returns NaN.
 */
static inline aph_vec3 aph_vec3_normalize_or_zero(aph_vec3 v) {
    float len_sq = aph_vec3_length_sq(v);
    if (len_sq <= APH_VEC3_NORMALIZE_EPSILON_SQ) {
        return aph_vec3_make(0.0f, 0.0f, 0.0f);
    }
    float len = sqrtf(len_sq);
    return aph_vec3_make(v.x / len, v.y / len, v.z / len);
}

/**
 * Linearly interpolates between vector a and vector b by factor t.
 *
 * Implemented as a * (1 - t) + b * t so that t = 0.0f returns exactly a and
 * t = 1.0f returns exactly b without endpoint drift.
 *
 * Inputs:
 *   a: Start vector (t = 0.0f).
 *   b: End vector (t = 1.0f).
 *   t: Interpolation parameter.
 *
 * Returns:
 *   Interpolated vector a * (1 - t) + b * t.
 */
static inline aph_vec3 aph_vec3_lerp(aph_vec3 a, aph_vec3 b, float t) {
    float one_minus_t = 1.0f - t;
    return aph_vec3_make(a.x * one_minus_t + b.x * t, a.y * one_minus_t + b.y * t,
                         a.z * one_minus_t + b.z * t);
}

#endif /* APH_MATH_H */
