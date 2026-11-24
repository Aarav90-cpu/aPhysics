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

#include "core/aph_status.h"
#include "tests/aph_test.h"
#include <string.h>

int aph_test_failures = 0;

static void test_status_strings(void) {
    APH_CHECK(strcmp(aph_status_string(APH_OK), "APH_OK") == 0);
    APH_CHECK(strcmp(aph_status_string(APH_ERR_INVALID_ARGUMENT), "APH_ERR_INVALID_ARGUMENT") == 0);
    APH_CHECK(strcmp(aph_status_string(APH_ERR_OUT_OF_MEMORY), "APH_ERR_OUT_OF_MEMORY") == 0);
    APH_CHECK(strcmp(aph_status_string(APH_ERR_BUFFER_TOO_SMALL), "APH_ERR_BUFFER_TOO_SMALL") == 0);
    APH_CHECK(strcmp(aph_status_string(APH_ERR_NOT_INITIALIZED), "APH_ERR_NOT_INITIALIZED") == 0);
    APH_CHECK(strcmp(aph_status_string((aph_status)999), "APH_ERR_UNKNOWN") == 0);
}

int main(void) {
    test_status_strings();

    if (aph_test_failures == 0) {
        printf("status_test: all checks passed.\n");
    } else {
        fprintf(stderr, "status_test: %d check(s) failed.\n", aph_test_failures);
    }

    return aph_test_failures == 0 ? 0 : 1;
}
