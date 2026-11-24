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
 * Squared magnitude threshold below which a quaternion is treated as zero during normalization.
 *
 * Quaternions are dimensionless. If length squared <= APH_QUAT_NORMALIZE_EPSILON_SQ,
 * normalization returns the identity quaternion (0, 0, 0, 1) to prevent division by near-zero.
 */
#define APH_QUAT_NORMALIZE_EPSILON_SQ 1e-12f

/**
 * 3D single-precision floating-point vector.
 */
typedef struct aph_vec3 {
    float x;
    float y;
    float z;
} aph_vec3;

/**
 * 3D orientation quaternion with single-precision floating point components.
 *
 * Stored in (x, y, z, w) order matching Three.js, where (x, y, z) is the vector
 * imaginary part and w is the real scalar part (q = w + x*i + y*j + z*k).
 */
typedef struct aph_quat {
    float x;
    float y;
    float z;
    float w;
} aph_quat;

/**
 * 3x3 matrix stored in row-major order:
 *   m[0] m[1] m[2]   (row 0)
 *   m[3] m[4] m[5]   (row 1)
 *   m[6] m[7] m[8]   (row 2)
 *
 * Preconditions:
 *   Three.js Matrix3 stores elements in column-major order.
 *   Transpose or convert when crossing the JavaScript boundary.
 */
typedef struct aph_mat3 {
    float m[9];
} aph_mat3;

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

/**
 * Returns the identity quaternion representing zero rotation.
 *
 * Returns:
 *   Quaternion (0, 0, 0, 1).
 */
static inline aph_quat aph_quat_identity(void) {
    aph_quat q;
    q.x = 0.0f;
    q.y = 0.0f;
    q.z = 0.0f;
    q.w = 1.0f;
    return q;
}

/**
 * Returns the complex conjugate of a quaternion.
 *
 * For a unit quaternion, the conjugate equals the inverse rotation (q* = q^-1).
 *
 * Inputs:
 *   q: Source quaternion.
 *
 * Returns:
 *   Conjugate (-q.x, -q.y, -q.z, q.w).
 */
static inline aph_quat aph_quat_conjugate(aph_quat q) {
    aph_quat res;
    res.x = -q.x;
    res.y = -q.y;
    res.z = -q.z;
    res.w = q.w;
    return res;
}

/**
 * Multiplies two quaternions using the Hamilton product.
 *
 * With rotation defined as v' = q * v * q*, multiplying a * b produces the composite
 * rotation that applies b first, then a: v' = a(b(v)).
 *
 * Preconditions:
 *   Does not re-normalize the result. If repeated multiplications accumulate
 *   numerical drift, the caller must call aph_quat_normalize().
 *
 * Inputs:
 *   a: Left-hand rotation applied second.
 *   b: Right-hand rotation applied first.
 *
 * Returns:
 *   Hamilton product a * b.
 */
static inline aph_quat aph_quat_mul(aph_quat a, aph_quat b) {
    aph_quat q;
    q.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
    q.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
    q.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
    q.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
    return q;
}

/**
 * Normalizes a quaternion to unit length.
 *
 * Checks if length squared <= APH_QUAT_NORMALIZE_EPSILON_SQ.
 * Using the <= comparison ensures that NaN inputs do not evaluate as <= epsilon
 * and propagate as NaN rather than being silently suppressed to identity.
 *
 * Normalization divides each component directly by length rather than multiplying
 * by an approximate reciprocal, preserving reproducible golden state hashes.
 *
 * Inputs:
 *   q: Quaternion to normalize.
 *
 * Returns:
 *   Unit quaternion q / |q|, or identity (0, 0, 0, 1) if |q|^2 <= APH_QUAT_NORMALIZE_EPSILON_SQ.
 *   If q contains NaN, returns NaN.
 */
static inline aph_quat aph_quat_normalize(aph_quat q) {
    float len_sq = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
    if (len_sq <= APH_QUAT_NORMALIZE_EPSILON_SQ) {
        return aph_quat_identity();
    }
    float len = sqrtf(len_sq);
    aph_quat res;
    res.x = q.x / len;
    res.y = q.y / len;
    res.z = q.z / len;
    res.w = q.w / len;
    return res;
}

/**
 * Constructs a unit quaternion from an axis of rotation and an angle in radians.
 *
 * Preconditions:
 *   sinf() and cosf() are for setup only (e.g. initial conditions or keyframe import).
 *   They must not be called inside the hot simulation step loop. Note that standard
 *   trigonometric functions are not available in freestanding WebAssembly builds until
 *   provided by the engine (Step 3.2).
 *
 * Inputs:
 *   axis: Axis of rotation. If axis length squared <= APH_VEC3_NORMALIZE_EPSILON_SQ,
 *         returns the identity quaternion.
 *   radians: Rotation angle in radians. If radians is NaN, propagates NaN.
 *
 * Returns:
 *   Unit rotation quaternion, or identity if axis length squared <= epsilon.
 */
static inline aph_quat aph_quat_from_axis_angle(aph_vec3 axis, float radians) {
    float axis_len_sq = aph_vec3_length_sq(axis);
    if (axis_len_sq <= APH_VEC3_NORMALIZE_EPSILON_SQ) {
        return aph_quat_identity();
    }
    float axis_len = sqrtf(axis_len_sq);
    float half_angle = 0.5f * radians;
    float s = sinf(half_angle);
    float c = cosf(half_angle);

    aph_quat q;
    q.x = (axis.x / axis_len) * s;
    q.y = (axis.y / axis_len) * s;
    q.z = (axis.z / axis_len) * s;
    q.w = c;
    return q;
}

/**
 * Rotates a 3D vector by a unit quaternion.
 *
 * Computes v' = q * v * q* using the Rodrigues vector formula:
 *   v' = v + 2*w*(u x v) + 2*(u x (u x v))
 * where u = (q.x, q.y, q.z) and w = q.w.
 *
 * Preconditions:
 *   q must be a unit quaternion. This is not checked at runtime.
 *
 * Inputs:
 *   q: Unit rotation quaternion.
 *   v: Vector to rotate.
 *
 * Returns:
 *   Rotated vector v'.
 */
static inline aph_vec3 aph_quat_rotate(aph_quat q, aph_vec3 v) {
    aph_vec3 u = aph_vec3_make(q.x, q.y, q.z);
    aph_vec3 u_cross_v = aph_vec3_cross(u, v);
    aph_vec3 t = aph_vec3_scale(u_cross_v, 2.0f);
    aph_vec3 w_t = aph_vec3_scale(t, q.w);
    aph_vec3 u_cross_t = aph_vec3_cross(u, t);
    return aph_vec3_add(v, aph_vec3_add(w_t, u_cross_t));
}

/**
 * Constructs a 3x3 rotation matrix from a unit quaternion in row-major order.
 *
 * Preconditions:
 *   q must be a unit quaternion. This is not checked at runtime.
 *
 * Inputs:
 *   q: Unit rotation quaternion.
 *
 * Returns:
 *   Row-major 3x3 rotation matrix representing the same orientation as q.
 */
static inline aph_mat3 aph_mat3_from_quat(aph_quat q) {
    float x2 = q.x + q.x;
    float y2 = q.y + q.y;
    float z2 = q.z + q.z;

    float xx = q.x * x2;
    float xy = q.x * y2;
    float xz = q.x * z2;
    float yy = q.y * y2;
    float yz = q.y * z2;
    float zz = q.z * z2;
    float wx = q.w * x2;
    float wy = q.w * y2;
    float wz = q.w * z2;

    aph_mat3 res;
    res.m[0] = 1.0f - (yy + zz);
    res.m[1] = xy - wz;
    res.m[2] = xz + wy;

    res.m[3] = xy + wz;
    res.m[4] = 1.0f - (xx + zz);
    res.m[5] = yz - wx;

    res.m[6] = xz - wy;
    res.m[7] = yz + wx;
    res.m[8] = 1.0f - (xx + yy);
    return res;
}

/**
 * Multiplies a row-major 3x3 matrix by a 3D vector (M * v).
 *
 * Preconditions:
 *   m must not be NULL.
 *
 * Inputs:
 *   m: Pointer to row-major 3x3 matrix.
 *   v: Vector to transform.
 *
 * Returns:
 *   Transformed vector M * v.
 */
static inline aph_vec3 aph_mat3_mul_vec3(const aph_mat3 *m, aph_vec3 v) {
    return aph_vec3_make(m->m[0] * v.x + m->m[1] * v.y + m->m[2] * v.z,
                         m->m[3] * v.x + m->m[4] * v.y + m->m[5] * v.z,
                         m->m[6] * v.x + m->m[7] * v.y + m->m[8] * v.z);
}

/**
 * Transposes a 3x3 matrix (swaps rows and columns).
 *
 * Preconditions:
 *   m must not be NULL.
 *
 * Inputs:
 *   m: Pointer to 3x3 matrix.
 *
 * Returns:
 *   Transposed matrix M^T.
 */
static inline aph_mat3 aph_mat3_transpose(const aph_mat3 *m) {
    aph_mat3 res;
    res.m[0] = m->m[0];
    res.m[1] = m->m[3];
    res.m[2] = m->m[6];

    res.m[3] = m->m[1];
    res.m[4] = m->m[4];
    res.m[5] = m->m[7];

    res.m[6] = m->m[2];
    res.m[7] = m->m[5];
    res.m[8] = m->m[8];
    return res;
}

/**
 * Returns the identity quaternion representing zero rotation.
 *
 * Returns:
 *   Quaternion (0, 0, 0, 1).
 */
static inline aph_quat aph_quat_identity(void) {
    aph_quat q;
    q.x = 0.0f;
    q.y = 0.0f;
    q.z = 0.0f;
    q.w = 1.0f;
    return q;
}

/**
 * Returns the complex conjugate of a quaternion.
 *
 * For a unit quaternion, the conjugate equals the inverse rotation (q* = q^-1).
 *
 * Inputs:
 *   q: Source quaternion.
 *
 * Returns:
 *   Conjugate (-q.x, -q.y, -q.z, q.w).
 */
static inline aph_quat aph_quat_conjugate(aph_quat q) {
    aph_quat res;
    res.x = -q.x;
    res.y = -q.y;
    res.z = -q.z;
    res.w = q.w;
    return res;
}

/**
 * Multiplies two quaternions using the Hamilton product.
 *
 * With rotation defined as v' = q * v * q*, multiplying a * b produces the composite
 * rotation that applies b first, then a: v' = a(b(v)).
 *
 * Preconditions:
 *   Does not re-normalize the result. If repeated multiplications accumulate
 *   numerical drift, the caller must call aph_quat_normalize().
 *
 * Inputs:
 *   a: Left-hand rotation applied second.
 *   b: Right-hand rotation applied first.
 *
 * Returns:
 *   Hamilton product a * b.
 */
static inline aph_quat aph_quat_mul(aph_quat a, aph_quat b) {
    aph_quat q;
    q.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
    q.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
    q.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
    q.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
    return q;
}

/**
 * Normalizes a quaternion to unit length.
 *
 * Checks if length squared <= APH_QUAT_NORMALIZE_EPSILON_SQ.
 * Using the <= comparison ensures that NaN inputs do not evaluate as <= epsilon
 * and propagate as NaN rather than being silently suppressed to identity.
 *
 * Normalization divides each component directly by length rather than multiplying
 * by an approximate reciprocal, preserving reproducible golden state hashes.
 *
 * Inputs:
 *   q: Quaternion to normalize.
 *
 * Returns:
 *   Unit quaternion q / |q|, or identity (0, 0, 0, 1) if |q|^2 <= APH_QUAT_NORMALIZE_EPSILON_SQ.
 *   If q contains NaN, returns NaN.
 */
static inline aph_quat aph_quat_normalize(aph_quat q) {
    float len_sq = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
    if (len_sq <= APH_QUAT_NORMALIZE_EPSILON_SQ) {
        return aph_quat_identity();
    }
    float len = sqrtf(len_sq);
    aph_quat res;
    res.x = q.x / len;
    res.y = q.y / len;
    res.z = q.z / len;
    res.w = q.w / len;
    return res;
}

/**
 * Constructs a unit quaternion from an axis of rotation and an angle in radians.
 *
 * Preconditions:
 *   sinf() and cosf() are for setup only (e.g. initial conditions or keyframe import).
 *   They must not be called inside the hot simulation step loop. Note that standard
 *   trigonometric functions are not available in freestanding WebAssembly builds until
 *   provided by the engine (Step 3.2).
 *
 * Inputs:
 *   axis: Axis of rotation. If axis length squared <= APH_VEC3_NORMALIZE_EPSILON_SQ,
 *         returns the identity quaternion.
 *   radians: Rotation angle in radians. If radians is NaN, propagates NaN.
 *
 * Returns:
 *   Unit rotation quaternion, or identity if axis length squared <= epsilon.
 */
static inline aph_quat aph_quat_from_axis_angle(aph_vec3 axis, float radians) {
    float axis_len_sq = aph_vec3_length_sq(axis);
    if (axis_len_sq <= APH_VEC3_NORMALIZE_EPSILON_SQ) {
        return aph_quat_identity();
    }
    float axis_len = sqrtf(axis_len_sq);
    float half_angle = 0.5f * radians;
    float s = sinf(half_angle);
    float c = cosf(half_angle);

    aph_quat q;
    q.x = (axis.x / axis_len) * s;
    q.y = (axis.y / axis_len) * s;
    q.z = (axis.z / axis_len) * s;
    q.w = c;
    return q;
}

/**
 * Rotates a 3D vector by a unit quaternion.
 *
 * Computes v' = q * v * q* using the Rodrigues vector formula:
 *   v' = v + 2*w*(u x v) + 2*(u x (u x v))
 * where u = (q.x, q.y, q.z) and w = q.w.
 *
 * Preconditions:
 *   q must be a unit quaternion. This is not checked at runtime.
 *
 * Inputs:
 *   q: Unit rotation quaternion.
 *   v: Vector to rotate.
 *
 * Returns:
 *   Rotated vector v'.
 */
static inline aph_vec3 aph_quat_rotate(aph_quat q, aph_vec3 v) {
    aph_vec3 u = aph_vec3_make(q.x, q.y, q.z);
    aph_vec3 u_cross_v = aph_vec3_cross(u, v);
    aph_vec3 t = aph_vec3_scale(u_cross_v, 2.0f);
    aph_vec3 w_t = aph_vec3_scale(t, q.w);
    aph_vec3 u_cross_t = aph_vec3_cross(u, t);
    return aph_vec3_add(v, aph_vec3_add(w_t, u_cross_t));
}

/**
 * Constructs a 3x3 rotation matrix from a unit quaternion in row-major order.
 *
 * Preconditions:
 *   q must be a unit quaternion. This is not checked at runtime.
 *
 * Inputs:
 *   q: Unit rotation quaternion.
 *
 * Returns:
 *   Row-major 3x3 rotation matrix representing the same orientation as q.
 */
static inline aph_mat3 aph_mat3_from_quat(aph_quat q) {
    float x2 = q.x + q.x;
    float y2 = q.y + q.y;
    float z2 = q.z + q.z;

    float xx = q.x * x2;
    float xy = q.x * y2;
    float xz = q.x * z2;
    float yy = q.y * y2;
    float yz = q.y * z2;
    float zz = q.z * z2;
    float wx = q.w * x2;
    float wy = q.w * y2;
    float wz = q.w * z2;

    aph_mat3 res;
    res.m[0] = 1.0f - (yy + zz);
    res.m[1] = xy - wz;
    res.m[2] = xz + wy;

    res.m[3] = xy + wz;
    res.m[4] = 1.0f - (xx + zz);
    res.m[5] = yz - wx;

    res.m[6] = xz - wy;
    res.m[7] = yz + wx;
    res.m[8] = 1.0f - (xx + yy);
    return res;
}

/**
 * Multiplies a row-major 3x3 matrix by a 3D vector (M * v).
 *
 * Preconditions:
 *   m must not be NULL.
 *
 * Inputs:
 *   m: Pointer to row-major 3x3 matrix.
 *   v: Vector to transform.
 *
 * Returns:
 *   Transformed vector M * v.
 */
static inline aph_vec3 aph_mat3_mul_vec3(const aph_mat3 *m, aph_vec3 v) {
    return aph_vec3_make(m->m[0] * v.x + m->m[1] * v.y + m->m[2] * v.z,
                         m->m[3] * v.x + m->m[4] * v.y + m->m[5] * v.z,
                         m->m[6] * v.x + m->m[7] * v.y + m->m[8] * v.z);
}

/**
 * Transposes a 3x3 matrix (swaps rows and columns).
 *
 * Preconditions:
 *   m must not be NULL.
 *
 * Inputs:
 *   m: Pointer to 3x3 matrix.
 *
 * Returns:
 *   Transposed matrix M^T.
 */
static inline aph_mat3 aph_mat3_transpose(const aph_mat3 *m) {
    aph_mat3 res;
    res.m[0] = m->m[0];
    res.m[1] = m->m[3];
    res.m[2] = m->m[6];

    res.m[3] = m->m[1];
    res.m[4] = m->m[4];
    res.m[5] = m->m[7];

    res.m[6] = m->m[2];
    res.m[7] = m->m[5];
    res.m[8] = m->m[8];
    return res;
}

#endif /* APH_MATH_H */
