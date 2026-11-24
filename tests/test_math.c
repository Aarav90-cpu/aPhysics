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

#include "aph_test.h"
#include "core/aph_math.h"
#include <math.h>

int aph_test_failures = 0;

static void test_vec3_make(void) {
    aph_vec3 v = aph_vec3_make(1.5f, -2.5f, 3.75f);
    APH_CHECK(v.x == 1.5f);
    APH_CHECK(v.y == -2.5f);
    APH_CHECK(v.z == 3.75f);
}

static void test_vec3_add_sub(void) {
    aph_vec3 a = aph_vec3_make(1.0f, 2.0f, 3.0f);
    aph_vec3 b = aph_vec3_make(4.0f, 5.0f, 6.0f);

    aph_vec3 sum = aph_vec3_add(a, b);
    APH_CHECK(sum.x == 5.0f);
    APH_CHECK(sum.y == 7.0f);
    APH_CHECK(sum.z == 9.0f);

    aph_vec3 diff = aph_vec3_sub(sum, b);
    APH_CHECK(diff.x == 1.0f);
    APH_CHECK(diff.y == 2.0f);
    APH_CHECK(diff.z == 3.0f);
}

static void test_vec3_scale(void) {
    aph_vec3 v = aph_vec3_make(2.0f, -3.0f, 4.0f);

    aph_vec3 scaled = aph_vec3_scale(v, 2.5f);
    APH_CHECK(scaled.x == 5.0f);
    APH_CHECK(scaled.y == -7.5f);
    APH_CHECK(scaled.z == 10.0f);

    aph_vec3 zero = aph_vec3_scale(v, 0.0f);
    APH_CHECK(zero.x == 0.0f);
    APH_CHECK(zero.y == 0.0f);
    APH_CHECK(zero.z == 0.0f);
}

static void test_vec3_dot(void) {
    aph_vec3 a = aph_vec3_make(1.0f, 2.0f, 3.0f);
    aph_vec3 b = aph_vec3_make(4.0f, -5.0f, 6.0f);

    /* 1*4 + 2*(-5) + 3*6 = 4 - 10 + 18 = 12 */
    float dot = aph_vec3_dot(a, b);
    APH_CHECK(dot == 12.0f);

    /* Orthogonal axes dot product is zero */
    aph_vec3 x_axis = aph_vec3_make(1.0f, 0.0f, 0.0f);
    aph_vec3 y_axis = aph_vec3_make(0.0f, 1.0f, 0.0f);
    APH_CHECK(aph_vec3_dot(x_axis, y_axis) == 0.0f);

    /* Self-dot product equals squared length */
    aph_vec3 v = aph_vec3_make(3.0f, 4.0f, 0.0f);
    APH_CHECK(aph_vec3_dot(v, v) == 25.0f);
}

static void test_vec3_cross_axes(void) {
    aph_vec3 x = aph_vec3_make(1.0f, 0.0f, 0.0f);
    aph_vec3 y = aph_vec3_make(0.0f, 1.0f, 0.0f);
    aph_vec3 z = aph_vec3_make(0.0f, 0.0f, 1.0f);

    /* Right-handed coordinate system basis crosses */
    aph_vec3 x_cross_y = aph_vec3_cross(x, y);
    APH_CHECK(x_cross_y.x == z.x && x_cross_y.y == z.y && x_cross_y.z == z.z);

    aph_vec3 y_cross_z = aph_vec3_cross(y, z);
    APH_CHECK(y_cross_z.x == x.x && y_cross_z.y == x.y && y_cross_z.z == x.z);

    aph_vec3 z_cross_x = aph_vec3_cross(z, x);
    APH_CHECK(z_cross_x.x == y.x && z_cross_x.y == y.y && z_cross_x.z == y.z);

    /* Anti-commutativity: a x b == -(b x a) */
    aph_vec3 y_cross_x = aph_vec3_cross(y, x);
    APH_CHECK(y_cross_x.x == -z.x && y_cross_x.y == -z.y && y_cross_x.z == -z.z);

    /* Collinear cross product is exactly zero */
    aph_vec3 x_cross_x = aph_vec3_cross(x, x);
    APH_CHECK(x_cross_x.x == 0.0f && x_cross_x.y == 0.0f && x_cross_x.z == 0.0f);

    /* Arbitrary known values: (1, 2, 3) x (4, 5, 6) = (-3, 6, -3) */
    aph_vec3 a = aph_vec3_make(1.0f, 2.0f, 3.0f);
    aph_vec3 b = aph_vec3_make(4.0f, 5.0f, 6.0f);
    aph_vec3 c = aph_vec3_cross(a, b);
    APH_CHECK(c.x == -3.0f && c.y == 6.0f && c.z == -3.0f);
}

static void test_vec3_cross_fma_guard(void) {
    /*
     * Cross product of a vector with itself using values not exactly representable in base 2.
     * Evaluates a.y * a.z - a.z * a.y.
     * Under IEEE 754 with -ffp-contract=off, this is guaranteed to equal exactly zero.
     * If FMA contraction is active, asymmetric fusion can cause non-zero residuals.
     */
    aph_vec3 a = aph_vec3_make(0.1f, 0.2f, 0.3f);
    aph_vec3 c = aph_vec3_cross(a, a);
    APH_CHECK(c.x == 0.0f);
    APH_CHECK(c.y == 0.0f);
    APH_CHECK(c.z == 0.0f);
}

static void test_vec3_length(void) {
    aph_vec3 v = aph_vec3_make(3.0f, 4.0f, 0.0f);
    APH_CHECK(aph_vec3_length_sq(v) == 25.0f);
    APH_CHECK(aph_vec3_length(v) == 5.0f);

    aph_vec3 zero = aph_vec3_make(0.0f, 0.0f, 0.0f);
    APH_CHECK(aph_vec3_length_sq(zero) == 0.0f);
    APH_CHECK(aph_vec3_length(zero) == 0.0f);

    aph_vec3 v3d = aph_vec3_make(1.0f, 2.0f, 2.0f);
    APH_CHECK(aph_vec3_length_sq(v3d) == 9.0f);
    APH_CHECK(aph_vec3_length(v3d) == 3.0f);
}

static void test_vec3_normalize_or_zero(void) {
    /* Known vector (3, 4, 0) -> (0.6, 0.8, 0.0) */
    aph_vec3 v = aph_vec3_make(3.0f, 4.0f, 0.0f);
    aph_vec3 n = aph_vec3_normalize_or_zero(v);
    APH_CHECK_NEAR(n.x, 0.6f, 1e-6f);
    APH_CHECK_NEAR(n.y, 0.8f, 1e-6f);
    APH_CHECK(n.z == 0.0f);
    APH_CHECK_NEAR(aph_vec3_length(n), 1.0f, 1e-6f);

    /* Unit vectors return unchanged */
    aph_vec3 x = aph_vec3_make(1.0f, 0.0f, 0.0f);
    aph_vec3 nx = aph_vec3_normalize_or_zero(x);
    APH_CHECK(nx.x == 1.0f && nx.y == 0.0f && nx.z == 0.0f);

    /* Zero vector returns exactly (0, 0, 0) */
    aph_vec3 zero = aph_vec3_make(0.0f, 0.0f, 0.0f);
    aph_vec3 n_zero = aph_vec3_normalize_or_zero(zero);
    APH_CHECK(n_zero.x == 0.0f && n_zero.y == 0.0f && n_zero.z == 0.0f);

    /*
     * Boundary check: vector just below epsilon (threshold 1e-12 m^2).
     * 0.5e-6 m has squared length 0.25e-12 m^2 <= 1e-12 m^2 -> returns zero vector.
     */
    aph_vec3 v_below = aph_vec3_make(0.5e-6f, 0.0f, 0.0f);
    aph_vec3 n_below = aph_vec3_normalize_or_zero(v_below);
    APH_CHECK(n_below.x == 0.0f && n_below.y == 0.0f && n_below.z == 0.0f);

    /* Exact epsilon boundary: 1e-6 m squared is 1e-12 m^2 <= 1e-12 m^2 -> returns zero vector */
    aph_vec3 v_exact = aph_vec3_make(1.0e-6f, 0.0f, 0.0f);
    aph_vec3 n_exact = aph_vec3_normalize_or_zero(v_exact);
    APH_CHECK(n_exact.x == 0.0f && n_exact.y == 0.0f && n_exact.z == 0.0f);

    /*
     * Boundary check: vector just above epsilon.
     * 2.0e-6 m has squared length 4.0e-12 m^2 > 1e-12 m^2 -> normalized to (1, 0, 0).
     */
    aph_vec3 v_above = aph_vec3_make(2.0e-6f, 0.0f, 0.0f);
    aph_vec3 n_above = aph_vec3_normalize_or_zero(v_above);
    APH_CHECK_NEAR(n_above.x, 1.0f, 1e-6f);
    APH_CHECK(n_above.y == 0.0f && n_above.z == 0.0f);

    /* NaN input returns NaN components (not silently converted to zero) */
    float nan_val = 0.0f / 0.0f;
    aph_vec3 v_nan = aph_vec3_make(nan_val, 1.0f, 2.0f);
    aph_vec3 n_nan = aph_vec3_normalize_or_zero(v_nan);
    APH_CHECK(isnan(n_nan.x));
    APH_CHECK(isnan(n_nan.y));
    APH_CHECK(isnan(n_nan.z));
}

static void test_vec3_lerp(void) {
    aph_vec3 a = aph_vec3_make(1.234567f, -8.910111f, 42.0f);
    aph_vec3 b = aph_vec3_make(9.876543f, 3.141592f, -17.0f);

    /* t = 0.0f returns exactly a */
    aph_vec3 l0 = aph_vec3_lerp(a, b, 0.0f);
    APH_CHECK(l0.x == a.x);
    APH_CHECK(l0.y == a.y);
    APH_CHECK(l0.z == a.z);

    /* t = 1.0f returns exactly b */
    aph_vec3 l1 = aph_vec3_lerp(a, b, 1.0f);
    APH_CHECK(l1.x == b.x);
    APH_CHECK(l1.y == b.y);
    APH_CHECK(l1.z == b.z);

    /* t = 0.5f returns midpoint */
    aph_vec3 l_half = aph_vec3_lerp(a, b, 0.5f);
    APH_CHECK_NEAR(l_half.x, (a.x + b.x) * 0.5f, 1e-6f);
    APH_CHECK_NEAR(l_half.y, (a.y + b.y) * 0.5f, 1e-6f);
    APH_CHECK_NEAR(l_half.z, (a.z + b.z) * 0.5f, 1e-6f);
}

static void test_quat_identity(void) {
    aph_quat id = aph_quat_identity();
    APH_CHECK(id.x == 0.0f);
    APH_CHECK(id.y == 0.0f);
    APH_CHECK(id.z == 0.0f);
    APH_CHECK(id.w == 1.0f);

    aph_vec3 v = aph_vec3_make(1.5f, -2.5f, 3.75f);
    aph_vec3 rot = aph_quat_rotate(id, v);
    APH_CHECK(rot.x == v.x && rot.y == v.y && rot.z == v.z);
}

static void test_quat_conjugate_and_inverse(void) {
    aph_vec3 axis = aph_vec3_make(1.0f, 2.0f, 3.0f);
    aph_quat q = aph_quat_from_axis_angle(axis, 0.65f);

    aph_quat conj = aph_quat_conjugate(q);
    APH_CHECK(conj.x == -q.x);
    APH_CHECK(conj.y == -q.y);
    APH_CHECK(conj.z == -q.z);
    APH_CHECK(conj.w == q.w);

    /* Quaternion multiplied by its conjugate equals identity: q * q* = (0, 0, 0, 1) */
    aph_quat prod = aph_quat_mul(q, conj);
    APH_CHECK_NEAR(prod.x, 0.0f, 1e-6f);
    APH_CHECK_NEAR(prod.y, 0.0f, 1e-6f);
    APH_CHECK_NEAR(prod.z, 0.0f, 1e-6f);
    APH_CHECK_NEAR(prod.w, 1.0f, 1e-6f);
}

static void test_quat_mul_order(void) {
    /*
     * aph_quat_mul(a, b) applies rotation b first, then a: v' = a(b(v)).
     * Test case:
     *   a = 90 deg about Z
     *   b = 90 deg about X
     * When rotating X = (1, 0, 0) by aph_quat_mul(a, b):
     *   b (90 deg about X) leaves X unchanged: b(X) = X.
     *   a (90 deg about Z) turns X into Y: a(X) = Y.
     * Therefore, aph_quat_mul(a, b) on X must yield Y = (0, 1, 0).
     */
    float pi_over_2 = 1.57079632679f;
    aph_quat a = aph_quat_from_axis_angle(aph_vec3_make(0.0f, 0.0f, 1.0f), pi_over_2);
    aph_quat b = aph_quat_from_axis_angle(aph_vec3_make(1.0f, 0.0f, 0.0f), pi_over_2);

    aph_quat ab = aph_quat_mul(a, b);
    aph_vec3 x_axis = aph_vec3_make(1.0f, 0.0f, 0.0f);
    aph_vec3 res_ab = aph_quat_rotate(ab, x_axis);

    APH_CHECK_NEAR(res_ab.x, 0.0f, 1e-6f);
    APH_CHECK_NEAR(res_ab.y, 1.0f, 1e-6f);
    APH_CHECK_NEAR(res_ab.z, 0.0f, 1e-6f);

    /*
     * The reverse order aph_quat_mul(b, a) would rotate X by a first (turning X to Y),
     * then by b (turning Y to Z), resulting in Z = (0, 0, 1).
     */
    aph_quat ba = aph_quat_mul(b, a);
    aph_vec3 res_ba = aph_quat_rotate(ba, x_axis);

    APH_CHECK_NEAR(res_ba.x, 0.0f, 1e-6f);
    APH_CHECK_NEAR(res_ba.y, 0.0f, 1e-6f);
    APH_CHECK_NEAR(res_ba.z, 1.0f, 1e-6f);
}

static void test_quat_normalize(void) {
    aph_quat q = {0.0f, 0.0f, 3.0f, 4.0f};
    aph_quat n = aph_quat_normalize(q);
    APH_CHECK_NEAR(n.x, 0.0f, 1e-6f);
    APH_CHECK_NEAR(n.y, 0.0f, 1e-6f);
    APH_CHECK_NEAR(n.z, 0.6f, 1e-6f);
    APH_CHECK_NEAR(n.w, 0.8f, 1e-6f);

    /* Zero quaternion normalizes to identity */
    aph_quat zero = {0.0f, 0.0f, 0.0f, 0.0f};
    aph_quat n_zero = aph_quat_normalize(zero);
    APH_CHECK(n_zero.x == 0.0f && n_zero.y == 0.0f && n_zero.z == 0.0f && n_zero.w == 1.0f);

    /* Sub-epsilon quaternion normalizes to identity */
    aph_quat tiny = {0.5e-6f, 0.0f, 0.0f, 0.0f};
    aph_quat n_tiny = aph_quat_normalize(tiny);
    APH_CHECK(n_tiny.x == 0.0f && n_tiny.y == 0.0f && n_tiny.z == 0.0f && n_tiny.w == 1.0f);

    /* NaN input propagates NaN */
    float nan_val = 0.0f / 0.0f;
    aph_quat q_nan = {nan_val, 0.0f, 0.0f, 1.0f};
    aph_quat n_nan = aph_quat_normalize(q_nan);
    APH_CHECK(isnan(n_nan.x));
    APH_CHECK(isnan(n_nan.w));
}

static void test_quat_from_axis_angle(void) {
    /* Zero axis returns identity */
    aph_vec3 zero_axis = aph_vec3_make(0.0f, 0.0f, 0.0f);
    aph_quat q_zero_axis = aph_quat_from_axis_angle(zero_axis, 1.57f);
    APH_CHECK(q_zero_axis.x == 0.0f && q_zero_axis.y == 0.0f && q_zero_axis.z == 0.0f &&
              q_zero_axis.w == 1.0f);

    /* Sub-epsilon axis returns identity */
    aph_vec3 tiny_axis = aph_vec3_make(0.5e-6f, 0.0f, 0.0f);
    aph_quat q_tiny = aph_quat_from_axis_angle(tiny_axis, 1.57f);
    APH_CHECK(q_tiny.x == 0.0f && q_tiny.y == 0.0f && q_tiny.z == 0.0f && q_tiny.w == 1.0f);

    /* Unnormalized axis (0, 0, 2) matches unit axis (0, 0, 1) */
    aph_vec3 unit_axis = aph_vec3_make(0.0f, 0.0f, 1.0f);
    aph_vec3 scaled_axis = aph_vec3_make(0.0f, 0.0f, 2.0f);
    aph_quat q_unit = aph_quat_from_axis_angle(unit_axis, 0.8f);
    aph_quat q_scaled = aph_quat_from_axis_angle(scaled_axis, 0.8f);
    APH_CHECK_NEAR(q_scaled.x, q_unit.x, 1e-6f);
    APH_CHECK_NEAR(q_scaled.y, q_unit.y, 1e-6f);
    APH_CHECK_NEAR(q_scaled.z, q_unit.z, 1e-6f);
    APH_CHECK_NEAR(q_scaled.w, q_unit.w, 1e-6f);

    /* Angle 0 returns identity */
    aph_quat q_angle_zero = aph_quat_from_axis_angle(unit_axis, 0.0f);
    APH_CHECK_NEAR(q_angle_zero.x, 0.0f, 1e-6f);
    APH_CHECK_NEAR(q_angle_zero.y, 0.0f, 1e-6f);
    APH_CHECK_NEAR(q_angle_zero.z, 0.0f, 1e-6f);
    APH_CHECK_NEAR(q_angle_zero.w, 1.0f, 1e-6f);

    /* NaN angle propagates NaN */
    float nan_val = 0.0f / 0.0f;
    aph_quat q_nan_angle = aph_quat_from_axis_angle(unit_axis, nan_val);
    APH_CHECK(isnan(q_nan_angle.x) || isnan(q_nan_angle.z));
    APH_CHECK(isnan(q_nan_angle.w));
}

static void test_rotations_strong(void) {
    float pi_over_2 = 1.57079632679f;
    aph_vec3 x = aph_vec3_make(1.0f, 0.0f, 0.0f);
    aph_vec3 y = aph_vec3_make(0.0f, 1.0f, 0.0f);
    aph_vec3 z = aph_vec3_make(0.0f, 0.0f, 1.0f);

    /* 1. Rotate X by +90 deg about Z gives +Y */
    aph_quat q_z90 = aph_quat_from_axis_angle(z, pi_over_2);
    aph_mat3 m_z90 = aph_mat3_from_quat(q_z90);
    aph_vec3 rot_q_z = aph_quat_rotate(q_z90, x);
    aph_vec3 rot_m_z = aph_mat3_mul_vec3(&m_z90, x);
    APH_CHECK_NEAR(rot_q_z.x, 0.0f, 1e-6f);
    APH_CHECK_NEAR(rot_q_z.y, 1.0f, 1e-6f);
    APH_CHECK_NEAR(rot_q_z.z, 0.0f, 1e-6f);
    APH_CHECK_NEAR(rot_m_z.x, 0.0f, 1e-6f);
    APH_CHECK_NEAR(rot_m_z.y, 1.0f, 1e-6f);
    APH_CHECK_NEAR(rot_m_z.z, 0.0f, 1e-6f);

    /* 2. Rotate Y by +90 deg about X gives +Z */
    aph_quat q_x90 = aph_quat_from_axis_angle(x, pi_over_2);
    aph_mat3 m_x90 = aph_mat3_from_quat(q_x90);
    aph_vec3 rot_q_x = aph_quat_rotate(q_x90, y);
    aph_vec3 rot_m_x = aph_mat3_mul_vec3(&m_x90, y);
    APH_CHECK_NEAR(rot_q_x.x, 0.0f, 1e-6f);
    APH_CHECK_NEAR(rot_q_x.y, 0.0f, 1e-6f);
    APH_CHECK_NEAR(rot_q_x.z, 1.0f, 1e-6f);
    APH_CHECK_NEAR(rot_m_x.x, 0.0f, 1e-6f);
    APH_CHECK_NEAR(rot_m_x.y, 0.0f, 1e-6f);
    APH_CHECK_NEAR(rot_m_x.z, 1.0f, 1e-6f);

    /* 3. Rotate Z by +90 deg about Y gives +X */
    aph_quat q_y90 = aph_quat_from_axis_angle(y, pi_over_2);
    aph_mat3 m_y90 = aph_mat3_from_quat(q_y90);
    aph_vec3 rot_q_y = aph_quat_rotate(q_y90, z);
    aph_vec3 rot_m_y = aph_mat3_mul_vec3(&m_y90, z);
    APH_CHECK_NEAR(rot_q_y.x, 1.0f, 1e-6f);
    APH_CHECK_NEAR(rot_q_y.y, 0.0f, 1e-6f);
    APH_CHECK_NEAR(rot_q_y.z, 0.0f, 1e-6f);
    APH_CHECK_NEAR(rot_m_y.x, 1.0f, 1e-6f);
    APH_CHECK_NEAR(rot_m_y.y, 0.0f, 1e-6f);
    APH_CHECK_NEAR(rot_m_y.z, 0.0f, 1e-6f);

    /* 4. Rotate X by -90 deg about Z gives -Y (checks right-hand rule) */
    aph_quat q_zn90 = aph_quat_from_axis_angle(z, -pi_over_2);
    aph_mat3 m_zn90 = aph_mat3_from_quat(q_zn90);
    aph_vec3 rot_q_zn = aph_quat_rotate(q_zn90, x);
    aph_vec3 rot_m_zn = aph_mat3_mul_vec3(&m_zn90, x);
    APH_CHECK_NEAR(rot_q_zn.x, 0.0f, 1e-6f);
    APH_CHECK_NEAR(rot_q_zn.y, -1.0f, 1e-6f);
    APH_CHECK_NEAR(rot_q_zn.z, 0.0f, 1e-6f);
    APH_CHECK_NEAR(rot_m_zn.x, 0.0f, 1e-6f);
    APH_CHECK_NEAR(rot_m_zn.y, -1.0f, 1e-6f);
    APH_CHECK_NEAR(rot_m_zn.z, 0.0f, 1e-6f);

    /*
     * 5. Rotate 120 deg (2*pi/3) about (1, 1, 1).
     * Permutes the axes: X -> Y, Y -> Z, Z -> X.
     * Uses every single entry of the 3x3 rotation matrix.
     */
    float angle_120 = 2.09439510239f; /* 2 * pi / 3 */
    aph_vec3 axis_111 = aph_vec3_make(1.0f, 1.0f, 1.0f);
    aph_quat q_120 = aph_quat_from_axis_angle(axis_111, angle_120);
    aph_mat3 m_120 = aph_mat3_from_quat(q_120);

    /* X -> Y */
    aph_vec3 rot_q_120_x = aph_quat_rotate(q_120, x);
    aph_vec3 rot_m_120_x = aph_mat3_mul_vec3(&m_120, x);
    APH_CHECK_NEAR(rot_q_120_x.x, 0.0f, 1e-6f);
    APH_CHECK_NEAR(rot_q_120_x.y, 1.0f, 1e-6f);
    APH_CHECK_NEAR(rot_q_120_x.z, 0.0f, 1e-6f);
    APH_CHECK_NEAR(rot_m_120_x.x, 0.0f, 1e-6f);
    APH_CHECK_NEAR(rot_m_120_x.y, 1.0f, 1e-6f);
    APH_CHECK_NEAR(rot_m_120_x.z, 0.0f, 1e-6f);

    /* Y -> Z */
    aph_vec3 rot_q_120_y = aph_quat_rotate(q_120, y);
    aph_vec3 rot_m_120_y = aph_mat3_mul_vec3(&m_120, y);
    APH_CHECK_NEAR(rot_q_120_y.x, 0.0f, 1e-6f);
    APH_CHECK_NEAR(rot_q_120_y.y, 0.0f, 1e-6f);
    APH_CHECK_NEAR(rot_q_120_y.z, 1.0f, 1e-6f);
    APH_CHECK_NEAR(rot_m_120_y.x, 0.0f, 1e-6f);
    APH_CHECK_NEAR(rot_m_120_y.y, 0.0f, 1e-6f);
    APH_CHECK_NEAR(rot_m_120_y.z, 1.0f, 1e-6f);

    /* Z -> X */
    aph_vec3 rot_q_120_z = aph_quat_rotate(q_120, z);
    aph_vec3 rot_m_120_z = aph_mat3_mul_vec3(&m_120, z);
    APH_CHECK_NEAR(rot_q_120_z.x, 1.0f, 1e-6f);
    APH_CHECK_NEAR(rot_q_120_z.y, 0.0f, 1e-6f);
    APH_CHECK_NEAR(rot_q_120_z.z, 0.0f, 1e-6f);
    APH_CHECK_NEAR(rot_m_120_z.x, 1.0f, 1e-6f);
    APH_CHECK_NEAR(rot_m_120_z.y, 0.0f, 1e-6f);
    APH_CHECK_NEAR(rot_m_120_z.z, 0.0f, 1e-6f);
}

static void test_mat3_transpose_properties(void) {
    /* Use the 120 deg rotation about (1, 1, 1) to test a dense matrix */
    aph_vec3 axis = aph_vec3_make(1.0f, 2.0f, 3.0f);
    aph_quat q = aph_quat_from_axis_angle(axis, 1.15f);
    aph_mat3 m = aph_mat3_from_quat(q);

    aph_mat3 m_t = aph_mat3_transpose(&m);
    aph_mat3 m_tt = aph_mat3_transpose(&m_t);

    /* (M^T)^T == M */
    for (int i = 0; i < 9; ++i) {
        APH_CHECK(m.m[i] == m_tt.m[i]);
    }

    /*
     * For an orthogonal matrix, M^T is the inverse: M^T(M * v) == v.
     * Tested on basis vectors and a generic vector.
     */
    aph_vec3 test_vectors[4] = {
        aph_vec3_make(1.0f, 0.0f, 0.0f),
        aph_vec3_make(0.0f, 1.0f, 0.0f),
        aph_vec3_make(0.0f, 0.0f, 1.0f),
        aph_vec3_make(1.5f, -2.7f, 3.8f),
    };

    for (int i = 0; i < 4; ++i) {
        aph_vec3 v = test_vectors[i];
        aph_vec3 v_fwd = aph_mat3_mul_vec3(&m, v);
        aph_vec3 v_back = aph_mat3_mul_vec3(&m_t, v_fwd);
        APH_CHECK_NEAR(v_back.x, v.x, 1e-6f);
        APH_CHECK_NEAR(v_back.y, v.y, 1e-6f);
        APH_CHECK_NEAR(v_back.z, v.z, 1e-6f);

        /* Matrix multiplication matches direct quaternion rotation */
        aph_vec3 v_quat = aph_quat_rotate(q, v);
        APH_CHECK_NEAR(v_fwd.x, v_quat.x, 1e-6f);
        APH_CHECK_NEAR(v_fwd.y, v_quat.y, 1e-6f);
        APH_CHECK_NEAR(v_fwd.z, v_quat.z, 1e-6f);
    }
}

int main(void) {
    test_vec3_make();
    test_vec3_add_sub();
    test_vec3_scale();
    test_vec3_dot();
    test_vec3_cross_axes();
    test_vec3_cross_fma_guard();
    test_vec3_length();
    test_vec3_normalize_or_zero();
    test_vec3_lerp();

    test_quat_identity();
    test_quat_conjugate_and_inverse();
    test_quat_mul_order();
    test_quat_normalize();
    test_quat_from_axis_angle();
    test_rotations_strong();
    test_mat3_transpose_properties();

    if (aph_test_failures == 0) {
        printf("math_test: all checks passed.\n");
    } else {
        fprintf(stderr, "math_test: %d check(s) failed.\n", aph_test_failures);
    }

    return aph_test_failures != 0;
}
