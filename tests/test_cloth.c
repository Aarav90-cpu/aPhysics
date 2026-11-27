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

#include "cloth/aph_cloth.h"
#include "core/aph_log.h"
#include "core/aph_world.h"
#include "core/aph_world_internal.h"
#include "tests/aph_test.h"
#include <math.h>
#include <stdint.h>
#include <string.h>

int aph_test_failures = 0;

static aph_world_config make_default_world_cfg(void) {
    aph_world_config cfg;
    cfg.seed = 1337ULL;
    cfg.fixed_dt = 1.0f / 60.0f;
    cfg.substeps = 8U;
    cfg.gravity = aph_vec3_make(0.0f, -9.81f, 0.0f);
    cfg.memory_bytes = 64U * 1024U;
    return cfg;
}

static void silent_log(aph_log_level level, const char *module, const char *msg, void *user) {
    (void)level;
    (void)module;
    (void)msg;
    (void)user;
}

static void test_cloth_invalid_args_and_rollback(void) {
    aph_world_config wcfg = make_default_world_cfg();
    aph_world *w = NULL;
    APH_CHECK(aph_world_create(&wcfg, &w) == APH_OK);

    const float valid_pos[9] = {0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.5f, 0.5f, 0.0f};
    aph_cloth_config ccfg;
    ccfg.vertex_count = 3U;
    ccfg.positions = valid_pos;
    ccfg.mass = 1.5f;
    ccfg.damping = 0.5f;
    ccfg.thickness = 0.01f;

    uint32_t id = 99U;

    /* 1. NULL arguments */
    APH_CHECK(aph_cloth_create(NULL, &ccfg, &id) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(id == 0U);
    APH_CHECK(aph_cloth_create(w, &ccfg, NULL) == APH_ERR_INVALID_ARGUMENT);

    aph_set_log_callback(silent_log, NULL);
    APH_CHECK(aph_cloth_create(w, NULL, &id) == APH_ERR_INVALID_ARGUMENT);

    /* 2. Zero vertices */
    ccfg.vertex_count = 0U;
    APH_CHECK(aph_cloth_create(w, &ccfg, &id) == APH_ERR_INVALID_ARGUMENT);
    ccfg.vertex_count = 3U;

    /* 3. NULL positions */
    ccfg.positions = NULL;
    APH_CHECK(aph_cloth_create(w, &ccfg, &id) == APH_ERR_INVALID_ARGUMENT);

    /* 4. NaN / Inf positions */
    float bad_pos[9] = {0.0f, 1.0f, 0.0f, 1.0f, NAN, 0.0f, 0.5f, 0.5f, 0.0f};
    ccfg.positions = bad_pos;
    APH_CHECK(aph_cloth_create(w, &ccfg, &id) == APH_ERR_INVALID_ARGUMENT);
    bad_pos[4] = INFINITY;
    APH_CHECK(aph_cloth_create(w, &ccfg, &id) == APH_ERR_INVALID_ARGUMENT);
    ccfg.positions = valid_pos;

    /* 5. Non-positive and non-finite mass */
    ccfg.mass = 0.0f;
    APH_CHECK(aph_cloth_create(w, &ccfg, &id) == APH_ERR_INVALID_ARGUMENT);
    ccfg.mass = -2.0f;
    APH_CHECK(aph_cloth_create(w, &ccfg, &id) == APH_ERR_INVALID_ARGUMENT);
    ccfg.mass = NAN;
    APH_CHECK(aph_cloth_create(w, &ccfg, &id) == APH_ERR_INVALID_ARGUMENT);
    ccfg.mass = 1.5f;

    /* 6. Negative and non-finite damping */
    ccfg.damping = -0.1f;
    APH_CHECK(aph_cloth_create(w, &ccfg, &id) == APH_ERR_INVALID_ARGUMENT);
    ccfg.damping = NAN;
    APH_CHECK(aph_cloth_create(w, &ccfg, &id) == APH_ERR_INVALID_ARGUMENT);
    ccfg.damping = 0.5f;

    /* 7. Negative and non-finite thickness */
    ccfg.thickness = -0.001f;
    APH_CHECK(aph_cloth_create(w, &ccfg, &id) == APH_ERR_INVALID_ARGUMENT);
    ccfg.thickness = INFINITY;
    APH_CHECK(aph_cloth_create(w, &ccfg, &id) == APH_ERR_INVALID_ARGUMENT);
    ccfg.thickness = 0.01f;

    /* All failed validations must leave arena.used and cloth_count untouched */
    APH_CHECK(w->arena.used == 0U);
    APH_CHECK(w->cloth_count == 0U);

    /* 8. OOM mid-allocation rolls back arena.used completely */
    aph_world_config tight_wcfg = make_default_world_cfg();
    tight_wcfg.memory_bytes = 1024U;
    aph_world *tight_w = NULL;
    APH_CHECK(aph_world_create(&tight_wcfg, &tight_w) == APH_OK);

    /* 32 vertices require > 1024 bytes across pos/prev/vel/inv_mass, failing partway through */
    float mesh_pos[32 * 3];
    for (size_t i = 0; i < 32U * 3U; ++i) {
        mesh_pos[i] = (float)i * 0.1f;
    }
    ccfg.vertex_count = 32U;
    ccfg.positions = mesh_pos;
    APH_CHECK(aph_cloth_create(tight_w, &ccfg, &id) == APH_ERR_OUT_OF_MEMORY);
    APH_CHECK(tight_w->arena.used == 0U);
    APH_CHECK(tight_w->cloth_count == 0U);
    APH_CHECK(tight_w->cloths_head == NULL);

    /* A smaller cloth that fits cleanly can still be created after rollback */
    ccfg.vertex_count = 3U;
    ccfg.positions = valid_pos;
    APH_CHECK(aph_cloth_create(tight_w, &ccfg, &id) == APH_OK);
    APH_CHECK(id == 0U);
    APH_CHECK(tight_w->cloth_count == 1U);

    /* 9. Accessor and pin/unpin error paths */
    const float *out_pos = (const float *)0x1234;
    uint32_t out_count = 77U;
    APH_CHECK(aph_cloth_positions(NULL, 0U, &out_pos, &out_count) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(out_pos == NULL);
    APH_CHECK(out_count == 0U);

    APH_CHECK(aph_cloth_positions(tight_w, 0U, NULL, &out_count) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(aph_cloth_positions(tight_w, 1U, &out_pos, &out_count) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(out_pos == NULL);
    APH_CHECK(out_count == 0U);

    APH_CHECK(aph_cloth_pin(NULL, 0U, 0U) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(aph_cloth_pin(tight_w, 1U, 0U) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(aph_cloth_pin(tight_w, 0U, 3U) == APH_ERR_INVALID_ARGUMENT);

    APH_CHECK(aph_cloth_unpin(NULL, 0U, 0U) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(aph_cloth_unpin(tight_w, 1U, 0U) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(aph_cloth_unpin(tight_w, 0U, 3U) == APH_ERR_INVALID_ARGUMENT);

    aph_set_log_callback(NULL, NULL);
    aph_world_destroy(tight_w);
    aph_world_destroy(w);
}

static void test_cloth_layout_alignment_and_ids(void) {
    aph_world_config wcfg = make_default_world_cfg();
    aph_world *w = NULL;
    APH_CHECK(aph_world_create(&wcfg, &w) == APH_OK);

    const float p0[6] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
    const float p1[3] = {-1.0f, 0.0f, 2.0f};

    aph_cloth_config ccfg;
    ccfg.vertex_count = 2U;
    ccfg.positions = p0;
    ccfg.mass = 1.0f;
    ccfg.damping = 0.0f;
    ccfg.thickness = 0.0f;

    uint32_t id0 = 99U;
    uint32_t id1 = 99U;
    APH_CHECK(aph_cloth_create(w, &ccfg, &id0) == APH_OK);
    APH_CHECK(id0 == 0U);

    ccfg.vertex_count = 1U;
    ccfg.positions = p1;
    APH_CHECK(aph_cloth_create(w, &ccfg, &id1) == APH_OK);
    APH_CHECK(id1 == 1U);

    const float *pos0 = NULL;
    uint32_t count0 = 0U;
    APH_CHECK(aph_cloth_positions(w, id0, &pos0, &count0) == APH_OK);
    APH_CHECK(pos0 != NULL);
    APH_CHECK(count0 == 2U);
    APH_CHECK(((uintptr_t)pos0 % 16U) == 0U);
    APH_CHECK(pos0[0] == 1.0f && pos0[1] == 2.0f && pos0[2] == 3.0f);
    APH_CHECK(pos0[3] == 4.0f && pos0[4] == 5.0f && pos0[5] == 6.0f);

    const float *pos1 = NULL;
    APH_CHECK(aph_cloth_positions(w, id1, &pos1, NULL) == APH_OK);
    APH_CHECK(pos1 != NULL);
    APH_CHECK(((uintptr_t)pos1 % 16U) == 0U);
    APH_CHECK(pos1[0] == -1.0f && pos1[1] == 0.0f && pos1[2] == 2.0f);

    aph_world_destroy(w);
}

static void test_cloth_free_fall_discrete_and_continuous(void) {
    const float g = -9.81f;
    const float dt = 1.0f / 60.0f;
    const uint32_t substeps = 8U;
    const uint32_t step_count = 60U; /* 1.0 second */

    aph_world_config wcfg;
    wcfg.seed = 42ULL;
    wcfg.fixed_dt = dt;
    wcfg.substeps = substeps;
    wcfg.gravity = aph_vec3_make(0.0f, g, 0.0f);
    wcfg.memory_bytes = 64U * 1024U;

    aph_world *w = NULL;
    APH_CHECK(aph_world_create(&wcfg, &w) == APH_OK);

    const float y0 = 0.0f;
    const float initial[3] = {1.0f, y0, -2.0f};
    aph_cloth_config ccfg;
    ccfg.vertex_count = 1U;
    ccfg.positions = initial;
    ccfg.mass = 1.0f;
    ccfg.damping = 0.0f;
    ccfg.thickness = 0.0f;

    uint32_t cid = 0U;
    APH_CHECK(aph_cloth_create(w, &ccfg, &cid) == APH_OK);

    for (uint32_t i = 0; i < step_count; ++i) {
        APH_CHECK(aph_world_step(w) == APH_OK);
    }

    const float *pos = NULL;
    APH_CHECK(aph_cloth_positions(w, cid, &pos, NULL) == APH_OK);
    APH_CHECK(pos[0] == 1.0f);
    APH_CHECK(pos[2] == -2.0f);

    /*
     * Exact discrete free-fall height after n substeps of size h under semi-implicit Euler:
     *   y_n = y0 + g * h * h * n * (n + 1) / 2
     * Starting at y0 = 0 avoids float32 catastrophic cancellation in (x - prev) / h; residual
     * float32 rounding across 480 substeps stays within ~5.4e-4 m (< 1e-3f).
     */
    const double h = (double)(dt / (float)substeps);
    const double n = (double)(step_count * substeps);
    const double t = n * h;
    const double expected_discrete = (double)y0 + (double)g * h * h * n * (n + 1.0) * 0.5;
    APH_CHECK_NEAR(pos[1], (float)expected_discrete, 1e-3f);

    /*
     * Continuous free-fall formula y0 - 0.5 * |g| * t^2 (equivalently y0 + 0.5 * g * t * t with
     * signed gravity g = -9.81) differs from symplectic Euler by
     *   y_discrete - y_continuous = 0.5 * g * h * t
     * because velocity is updated before position in each substep (first-order error).
     * For g = -9.81, h = 1/480, t = 1.0 s, that offset plus float32 drift is ~0.0108 m.
     */
    const double expected_continuous = (double)y0 + 0.5 * (double)g * t * t;
    APH_CHECK_NEAR(pos[1], (float)expected_continuous, 0.02f);

    aph_world_destroy(w);
}

static void test_cloth_pin_and_unpin(void) {
    aph_world_config wcfg = make_default_world_cfg();
    aph_world *w = NULL;
    APH_CHECK(aph_world_create(&wcfg, &w) == APH_OK);

    const float initial[6] = {1.25f, 3.5f, -0.75f, -1.0f, 3.5f, 0.0f};
    aph_cloth_config ccfg;
    ccfg.vertex_count = 2U;
    ccfg.positions = initial;
    ccfg.mass = 2.0f;
    ccfg.damping = 0.0f;
    ccfg.thickness = 0.0f;

    uint32_t cid = 0U;
    APH_CHECK(aph_cloth_create(w, &ccfg, &cid) == APH_OK);
    APH_CHECK(aph_cloth_pin(w, cid, 0U) == APH_OK);

    const float *pos = NULL;
    APH_CHECK(aph_cloth_positions(w, cid, &pos, NULL) == APH_OK);

    /* Pinned vertex 0 must remain bitwise identical while vertex 1 falls */
    for (int i = 0; i < 20; ++i) {
        APH_CHECK(aph_world_step(w) == APH_OK);
    }
    APH_CHECK(memcmp(&pos[0], &initial[0], 3U * sizeof(float)) == 0);
    APH_CHECK(pos[4] < initial[4]);

    /* Unpinning vertex 0 restores its positive inverse mass so it starts falling */
    APH_CHECK(aph_cloth_unpin(w, cid, 0U) == APH_OK);
    for (int i = 0; i < 10; ++i) {
        APH_CHECK(aph_world_step(w) == APH_OK);
    }
    APH_CHECK(pos[1] < initial[1]);

    aph_world_destroy(w);
}

static void test_cloth_damping_decay_and_clamp(void) {
    /*
     * Starting from rest under constant gravity g with linear damping c:
     *   dv/dt = g - c * v   =>   v(t) = (g / c) * (1 - exp(-c * t))
     * Thus the velocity gap to terminal velocity (v_inf = g / c) decays exponentially:
     *   (v_inf - v(t)) / v_inf = exp(-c * t)
     * Comparing finite-difference velocity dy / dt over step dt against the continuous ODE
     * exhibits O(c * h) substep discretization error plus O(c * dt) finite-difference lag.
     * With c = 4.0 s^-1 and substeps = 16 (h = 1/960 s), both fit cleanly within 0.03.
     */
    const float g = -10.0f;
    const float c = 4.0f;
    const float dt = 1.0f / 60.0f;
    const uint32_t substeps = 16U;

    aph_world_config wcfg;
    wcfg.seed = 7ULL;
    wcfg.fixed_dt = dt;
    wcfg.substeps = substeps;
    wcfg.gravity = aph_vec3_make(0.0f, g, 0.0f);
    wcfg.memory_bytes = 64U * 1024U;

    aph_world *w = NULL;
    APH_CHECK(aph_world_create(&wcfg, &w) == APH_OK);

    const float initial[3] = {0.0f, 0.0f, 0.0f};
    aph_cloth_config ccfg;
    ccfg.vertex_count = 1U;
    ccfg.positions = initial;
    ccfg.mass = 1.0f;
    ccfg.damping = c;
    ccfg.thickness = 0.0f;

    uint32_t cid = 0U;
    APH_CHECK(aph_cloth_create(w, &ccfg, &cid) == APH_OK);

    const float *pos = NULL;
    APH_CHECK(aph_cloth_positions(w, cid, &pos, NULL) == APH_OK);

    const double v_inf = (double)g / (double)c; /* -2.5 m/s */
    const uint32_t checkpoints[3] = {15U, 30U, 60U};
    double prev_residual = 1.0;

    uint32_t step = 0U;
    for (size_t cp = 0; cp < 3U; ++cp) {
        while (step + 1U < checkpoints[cp]) {
            APH_CHECK(aph_world_step(w) == APH_OK);
            step++;
        }
        const float y_before = pos[1];
        APH_CHECK(aph_world_step(w) == APH_OK);
        step++;
        const float y_after = pos[1];

        const double v_num = (double)(y_after - y_before) / (double)dt;
        const double residual = (v_inf - v_num) / v_inf;
        const double t_mid = ((double)step - 0.5) * (double)dt;
        const double expected_decay = exp(-(double)c * t_mid);

        APH_CHECK(residual > 0.0 && residual < prev_residual);
        APH_CHECK_NEAR((float)residual, (float)expected_decay, 0.03f);
        prev_residual = residual;
    }

    aph_world_destroy(w);

    /* Huge damping (damping * h > 1) must clamp damp_factor to 0 without inverting velocity */
    wcfg.substeps = 1U;
    APH_CHECK(aph_world_create(&wcfg, &w) == APH_OK);
    ccfg.damping = 1000.0f;
    APH_CHECK(aph_cloth_create(w, &ccfg, &cid) == APH_OK);
    APH_CHECK(aph_cloth_positions(w, cid, &pos, NULL) == APH_OK);

    APH_CHECK(aph_world_step(w) == APH_OK);
    const float step1_y = pos[1];
    APH_CHECK(step1_y < 0.0f);
    APH_CHECK(aph_world_step(w) == APH_OK);
    const float step2_dy = pos[1] - step1_y;
    /* Because v damped to 0 after step 1, step 2 repeats identical displacement g * h^2 */
    APH_CHECK_NEAR(step2_dy, step1_y, 1e-6f);

    aph_world_destroy(w);
}

/**
 * Tests state hash determinism and golden hash values for a small cloth.
 *
 * Expected golden hashes computed independently by Python IEEE 754 float32 emulator:
 *   tools/verify_cloth_golden.py
 * Layout hashed:
 *   36-byte world header + (id, vertex_count, mass, damping, thickness, pos, vel, inv_masses).
 */
static void test_cloth_hash_determinism_and_golden(void) {
    aph_world_config wcfg = make_default_world_cfg();

    /* 1. Empty world keeps existing Phase 1 golden hash intact */
    aph_world *empty_w = NULL;
    APH_CHECK(aph_world_create(&wcfg, &empty_w) == APH_OK);
    APH_CHECK(aph_world_hash(empty_w) == 0x7f591bd7f3766fd5ULL);
    aph_world_destroy(empty_w);

    /* 2. Small 3-vertex cloth matching tools/verify_cloth_golden.py */
    aph_world *w1 = NULL;
    aph_world *w2 = NULL;
    APH_CHECK(aph_world_create(&wcfg, &w1) == APH_OK);
    APH_CHECK(aph_world_create(&wcfg, &w2) == APH_OK);

    const float initial[9] = {0.0f, 2.0f, 0.0f, 1.0f, 2.0f, 0.0f, 0.5f, 1.5f, 0.25f};
    aph_cloth_config ccfg;
    ccfg.vertex_count = 3U;
    ccfg.positions = initial;
    ccfg.mass = 1.5f;
    ccfg.damping = 2.0f;
    ccfg.thickness = 0.01f;

    uint32_t id1 = 0U;
    uint32_t id2 = 0U;
    APH_CHECK(aph_cloth_create(w1, &ccfg, &id1) == APH_OK);
    APH_CHECK(aph_cloth_create(w2, &ccfg, &id2) == APH_OK);
    APH_CHECK(aph_cloth_pin(w1, id1, 0U) == APH_OK);
    APH_CHECK(aph_cloth_pin(w2, id2, 0U) == APH_OK);

    const uint64_t h0 = aph_world_hash(w1);
    APH_CHECK(h0 == 0x59f4baf7eeab908cULL);
    APH_CHECK(h0 == aph_world_hash(w2));

    for (int i = 0; i < 5; ++i) {
        APH_CHECK(aph_world_step(w1) == APH_OK);
        APH_CHECK(aph_world_step(w2) == APH_OK);
    }
    const uint64_t h5 = aph_world_hash(w1);
    APH_CHECK(h5 == 0x3ffed7ca17f67b6cULL);
    APH_CHECK(h5 == aph_world_hash(w2));

    for (int i = 0; i < 5; ++i) {
        APH_CHECK(aph_world_step(w1) == APH_OK);
        APH_CHECK(aph_world_step(w2) == APH_OK);
    }
    const uint64_t h10 = aph_world_hash(w1);
    APH_CHECK(h10 == 0x68860a560795385bULL);
    APH_CHECK(h10 == aph_world_hash(w2));

    /* Unpinning modifies inv_masses -> changes state hash immediately */
    APH_CHECK(aph_cloth_unpin(w1, id1, 0U) == APH_OK);
    APH_CHECK(aph_world_hash(w1) != h10);

    aph_world_destroy(w1);
    aph_world_destroy(w2);
}

int main(void) {
    test_cloth_invalid_args_and_rollback();
    test_cloth_layout_alignment_and_ids();
    test_cloth_free_fall_discrete_and_continuous();
    test_cloth_pin_and_unpin();
    test_cloth_damping_decay_and_clamp();
    test_cloth_hash_determinism_and_golden();

    if (aph_test_failures == 0) {
        printf("cloth_test: all checks passed.\n");
    } else {
        fprintf(stderr, "cloth_test: %d check(s) failed.\n", aph_test_failures);
    }

    return aph_test_failures == 0 ? 0 : 1;
}
