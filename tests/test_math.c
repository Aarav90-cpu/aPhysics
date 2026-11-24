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

    if (aph_test_failures == 0) {
        printf("math_test: all checks passed.\n");
    } else {
        fprintf(stderr, "math_test: %d check(s) failed.\n", aph_test_failures);
    }

    return aph_test_failures != 0;
}
