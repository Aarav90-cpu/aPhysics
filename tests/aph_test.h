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

#ifndef APH_TEST_H
#define APH_TEST_H

#include <math.h>
#include <stdio.h>

/** Global count of test assertion failures across the test executable. */
extern int aph_test_failures;

/**
 * Checks that a boolean condition is true.
 *
 * If false, prints the failure with file, line, and expression to stderr,
 * and increments aph_test_failures.
 */
#define APH_CHECK(cond)                                                                            \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #cond);               \
            aph_test_failures++;                                                                   \
        }                                                                                          \
    } while (0)

/**
 * Checks that two single-precision floating point values are within eps.
 */
#define APH_CHECK_NEAR(a, b, eps) APH_CHECK(fabsf((a) - (b)) <= (eps))

#endif /* APH_TEST_H */
