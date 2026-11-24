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

#include "core/aph_log.h"
#include "core/aph_world.h"
#include "tests/aph_test.h"
#include <math.h>
#include <stdint.h>
#include <string.h>

#if defined(__SANITIZE_ADDRESS__) || (defined(__has_feature) && __has_feature(address_sanitizer))
const char *__asan_default_options(void) {
    return "allocator_may_return_null=1";
}
#endif

int aph_test_failures = 0;

static char g_last_log_msg[256];

static void test_log_capture(aph_log_level level, const char *module, const char *msg, void *user) {
    (void)level;
    (void)module;
    (void)user;
    strncpy(g_last_log_msg, msg, sizeof(g_last_log_msg) - 1);
    g_last_log_msg[sizeof(g_last_log_msg) - 1] = '\0';
}

static aph_world_config default_test_config(void) {
    aph_world_config cfg;
    cfg.seed = 1337ULL;
    cfg.fixed_dt = 1.0f / 60.0f;
    cfg.substeps = 8U;
    cfg.gravity = aph_vec3_make(0.0f, -9.81f, 0.0f);
    cfg.memory_bytes = 64 * 1024;
    return cfg;
}

static void test_world_create_destroy_clean(void) {
    aph_world_config cfg = default_test_config();
    aph_world *w = NULL;

    aph_status status = aph_world_create(&cfg, &w);
    APH_CHECK(status == APH_OK);
    APH_CHECK(w != NULL);
    APH_CHECK(aph_world_step_count(w) == 0);

    aph_world_destroy(w);

    /* aph_world_destroy(NULL) must safely do nothing */
    aph_world_destroy(NULL);
}

static void test_world_invalid_configs_and_failure_paths(void) {
    aph_world *w = (aph_world *)0x1234;
    aph_world_config cfg = default_test_config();

    /* 1. out_world is NULL */
    APH_CHECK(aph_world_create(&cfg, NULL) == APH_ERR_INVALID_ARGUMENT);

    /* 2. cfg is NULL */
    w = (aph_world *)0x1234;
    APH_CHECK(aph_world_create(NULL, &w) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(w == NULL);

    /* 3. Invalid fixed_dt: <= 0, NaN, Inf, > 1.0 */
    cfg.fixed_dt = 0.0f;
    APH_CHECK(aph_world_create(&cfg, &w) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(w == NULL);

    cfg.fixed_dt = -0.016f;
    APH_CHECK(aph_world_create(&cfg, &w) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(w == NULL);

    cfg.fixed_dt = NAN;
    APH_CHECK(aph_world_create(&cfg, &w) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(w == NULL);

    cfg.fixed_dt = INFINITY;
    APH_CHECK(aph_world_create(&cfg, &w) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(w == NULL);

    cfg.fixed_dt = 1.01f;
    APH_CHECK(aph_world_create(&cfg, &w) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(w == NULL);
    cfg.fixed_dt = 1.0f / 60.0f; /* restore */

    /* 4. Invalid substeps: 0 and > 1000 */
    cfg.substeps = 0;
    APH_CHECK(aph_world_create(&cfg, &w) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(w == NULL);

    cfg.substeps = 1001;
    APH_CHECK(aph_world_create(&cfg, &w) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(w == NULL);
    cfg.substeps = 8U; /* restore */

    /* 5. Invalid gravity components: NaN and Inf */
    cfg.gravity.x = NAN;
    APH_CHECK(aph_world_create(&cfg, &w) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(w == NULL);
    cfg.gravity.x = 0.0f;

    cfg.gravity.y = INFINITY;
    APH_CHECK(aph_world_create(&cfg, &w) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(w == NULL);
    cfg.gravity.y = -9.81f;

    cfg.gravity.z = -INFINITY;
    APH_CHECK(aph_world_create(&cfg, &w) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(w == NULL);
    cfg.gravity.z = 0.0f;

    /* 6. Invalid memory_bytes: below minimum (1024) */
    cfg.memory_bytes = 1023;
    APH_CHECK(aph_world_create(&cfg, &w) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(w == NULL);

    /* 7. Memory bytes overflow */
    cfg.memory_bytes = SIZE_MAX;
    APH_CHECK(aph_world_create(&cfg, &w) == APH_ERR_INVALID_ARGUMENT);
    APH_CHECK(w == NULL);

    /* 8. Memory bytes so large that host allocation fails */
    aph_set_log_callback(test_log_capture, NULL);
    g_last_log_msg[0] = '\0';
    cfg.memory_bytes = SIZE_MAX / 2;
    APH_CHECK(aph_world_create(&cfg, &w) == APH_ERR_OUT_OF_MEMORY);
    APH_CHECK(w == NULL);
    APH_CHECK(strcmp(g_last_log_msg,
                     "Out of memory: host failed to allocate world memory budget. Reduce "
                     "memory_bytes in world config or free system memory.") == 0);
    aph_set_log_callback(NULL, NULL);
}

static void test_world_step_and_null(void) {
    aph_world_config cfg = default_test_config();
    aph_world *w = NULL;

    APH_CHECK(aph_world_create(&cfg, &w) == APH_OK);
    APH_CHECK(aph_world_step_count(w) == 0);

    /* Step exactly 10 times */
    for (int i = 0; i < 10; ++i) {
        aph_status st = aph_world_step(w);
        APH_CHECK(st == APH_OK);
    }
    APH_CHECK(aph_world_step_count(w) == 10);

    /* NULL world check on step */
    APH_CHECK(aph_world_step(NULL) == APH_ERR_INVALID_ARGUMENT);

    aph_world_destroy(w);
}

/**
 * Tests state hash equivalence and golden values.
 *
 * Expected golden hash values computed independently using Python 64-bit FNV-1a:
 *   # Layout: <QQfIfff (36 bytes total, little-endian)
 *   # seed: 1337ULL, step_count: 0ULL, fixed_dt: 1.0f/60.0f, substeps: 8U, gravity: (0, -9.81, 0)
 *   # Step 0 expected hash: 0x7f591bd7f3766fd5ULL
 *   # Step 5 expected hash: 0x9a20090370d00fd0ULL
 */
static void test_world_hash_determinism_and_golden(void) {
    aph_world_config cfg1 = default_test_config();
    aph_world_config cfg2 = default_test_config();

    aph_world *w1 = NULL;
    aph_world *w2 = NULL;

    APH_CHECK(aph_world_create(&cfg1, &w1) == APH_OK);
    APH_CHECK(aph_world_create(&cfg2, &w2) == APH_OK);

    /* 1. Golden hash check at step 0 */
    uint64_t h1_0 = aph_world_hash(w1);
    uint64_t h2_0 = aph_world_hash(w2);
    APH_CHECK(h1_0 == 0x7f591bd7f3766fd5ULL);
    APH_CHECK(h1_0 == h2_0);

    /* 2. Step both 5 times: hashes advance identically to golden step 5 value */
    for (int i = 0; i < 5; ++i) {
        APH_CHECK(aph_world_step(w1) == APH_OK);
        APH_CHECK(aph_world_step(w2) == APH_OK);
    }
    uint64_t h1_5 = aph_world_hash(w1);
    uint64_t h2_5 = aph_world_hash(w2);
    APH_CHECK(h1_5 == 0x9a20090370d00fd0ULL);
    APH_CHECK(h1_5 == h2_5);

    /* 3. Step w1 one more time: step counts differ (6 vs 5) -> hashes diverge */
    APH_CHECK(aph_world_step(w1) == APH_OK);
    uint64_t h1_6 = aph_world_hash(w1);
    APH_CHECK(h1_6 != h2_5);

    /* 4. Different seed produces different hash */
    aph_world_config cfg3 = default_test_config();
    cfg3.seed = 9999ULL;
    aph_world *w3 = NULL;
    APH_CHECK(aph_world_create(&cfg3, &w3) == APH_OK);
    APH_CHECK(aph_world_hash(w3) != h1_0);

    /* 5. Different gravity produces different hash */
    aph_world_config cfg4 = default_test_config();
    cfg4.gravity = aph_vec3_make(0.0f, -1.62f, 0.0f); /* Moon gravity */
    aph_world *w4 = NULL;
    APH_CHECK(aph_world_create(&cfg4, &w4) == APH_OK);
    APH_CHECK(aph_world_hash(w4) != h1_0);

    aph_world_destroy(w1);
    aph_world_destroy(w2);
    aph_world_destroy(w3);
    aph_world_destroy(w4);
}

int main(void) {
    test_world_create_destroy_clean();
    test_world_invalid_configs_and_failure_paths();
    test_world_step_and_null();
    test_world_hash_determinism_and_golden();

    if (aph_test_failures == 0) {
        printf("world_test: all checks passed.\n");
    } else {
        fprintf(stderr, "world_test: %d check(s) failed.\n", aph_test_failures);
    }

    return aph_test_failures == 0 ? 0 : 1;
}
