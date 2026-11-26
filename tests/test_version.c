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
#include "aphysics.h"
#include <stdio.h>
#include <string.h>

int aph_test_failures = 0;

static void test_version_non_empty(void) {
    const char *ver = aphysics_version();
    char expected[32];
    int len = snprintf(expected, sizeof(expected), "%d.%d.%d", APHYSICS_VERSION_MAJOR,
                       APHYSICS_VERSION_MINOR, APHYSICS_VERSION_PATCH);

    APH_CHECK(len > 0 && (size_t)len < sizeof(expected));
    APH_CHECK(ver != NULL);
    if (ver != NULL) {
        APH_CHECK(strlen(ver) > 0);
        APH_CHECK(strcmp(ver, APHYSICS_VERSION_STRING) == 0);
        APH_CHECK(strcmp(ver, expected) == 0);
    }
}

static void test_check_near(void) {
    APH_CHECK_NEAR(1.0f, 1.0001f, 0.001f);
}

int main(void) {
    test_version_non_empty();
    test_check_near();

    if (aph_test_failures == 0) {
        printf("test_version: all checks passed.\n");
    } else {
        fprintf(stderr, "test_version: %d check(s) failed.\n", aph_test_failures);
    }

    return aph_test_failures != 0;
}
