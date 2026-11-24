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
#include "tests/aph_test.h"
#include <string.h>

int aph_test_failures = 0;

typedef struct {
    int call_count;
    aph_log_level last_level;
    char last_module[64];
    char last_msg[128];
    void *last_user;
} log_capture;

static void capture_callback(aph_log_level level, const char *module, const char *msg, void *user) {
    log_capture *cap = (log_capture *)user;
    if (cap == NULL) {
        return;
    }
    cap->call_count++;
    cap->last_level = level;

    strncpy(cap->last_module, module, sizeof(cap->last_module) - 1);
    cap->last_module[sizeof(cap->last_module) - 1] = '\0';

    strncpy(cap->last_msg, msg, sizeof(cap->last_msg) - 1);
    cap->last_msg[sizeof(cap->last_msg) - 1] = '\0';

    cap->last_user = user;
}

static void alt_capture_callback(aph_log_level level, const char *module, const char *msg,
                                 void *user) {
    (void)level;
    (void)module;
    (void)msg;
    int *count = (int *)user;
    if (count != NULL) {
        (*count)++;
    }
}

static void test_log_level_strings(void) {
    APH_CHECK(strcmp(aph_log_level_string(APH_LOG_ERROR), "ERROR") == 0);
    APH_CHECK(strcmp(aph_log_level_string(APH_LOG_WARN), "WARN") == 0);
    APH_CHECK(strcmp(aph_log_level_string(APH_LOG_INFO), "INFO") == 0);
    APH_CHECK(strcmp(aph_log_level_string(APH_LOG_DEBUG), "DEBUG") == 0);
    APH_CHECK(strcmp(aph_log_level_string((aph_log_level)999), "UNKNOWN") == 0);
}

static void test_log_format_valid(void) {
    char buf[128];
    aph_status st = aph_log_format(buf, sizeof(buf), APH_LOG_INFO, "cloth", "Solver initialized.");
    APH_CHECK(st == APH_OK);
    APH_CHECK(strcmp(buf, "[aphysics] INFO cloth: Solver initialized.") == 0);

    st = aph_log_format(buf, sizeof(buf), APH_LOG_ERROR, "arena",
                        "Out of memory: the world's memory budget is too small. Increase "
                        "memory_bytes in the world config.");
    APH_CHECK(st == APH_OK);
    APH_CHECK(strcmp(buf, "[aphysics] ERROR arena: Out of memory: the world's memory budget is too "
                          "small. Increase memory_bytes in the world config.") == 0);
}

static void test_log_format_truncation_and_edge_cases(void) {
    char buf[16];

    /* 1. buf_size == 0 */
    APH_CHECK(aph_log_format(buf, 0, APH_LOG_INFO, "mod", "msg") == APH_ERR_BUFFER_TOO_SMALL);

    /* 2. out_buf == NULL with buf_size > 0 */
    APH_CHECK(aph_log_format(NULL, 16, APH_LOG_INFO, "mod", "msg") == APH_ERR_INVALID_ARGUMENT);

    /* 3. buf_size == 1: truncated, output is empty NUL-terminated string */
    buf[0] = 'X';
    aph_status st = aph_log_format(buf, 1, APH_LOG_INFO, "mod", "msg");
    APH_CHECK(st == APH_ERR_BUFFER_TOO_SMALL);
    APH_CHECK(buf[0] == '\0');

    /* 4. Small buffer causing partial truncation */
    char small_buf[14]; /* "[aphysics] " is 11 chars */
    st = aph_log_format(small_buf, sizeof(small_buf), APH_LOG_INFO, "core", "test message");
    APH_CHECK(st == APH_ERR_BUFFER_TOO_SMALL);
    APH_CHECK(small_buf[sizeof(small_buf) - 1] == '\0');
    APH_CHECK(strncmp(small_buf, "[aphysics] I", 12) == 0);
}

static void test_log_format_null_substitutions(void) {
    char buf[128];
    aph_status st = aph_log_format(buf, sizeof(buf), APH_LOG_ERROR, NULL, "Error occurred.");
    APH_CHECK(st == APH_OK);
    APH_CHECK(strcmp(buf, "[aphysics] ERROR unknown: Error occurred.") == 0);

    st = aph_log_format(buf, sizeof(buf), APH_LOG_WARN, "cloth", NULL);
    APH_CHECK(st == APH_OK);
    APH_CHECK(strcmp(buf, "[aphysics] WARN cloth: (no message)") == 0);

    st = aph_log_format(buf, sizeof(buf), (aph_log_level)999, NULL, NULL);
    APH_CHECK(st == APH_OK);
    APH_CHECK(strcmp(buf, "[aphysics] UNKNOWN unknown: (no message)") == 0);
}

static void test_log_callback_delivery(void) {
    log_capture cap;
    memset(&cap, 0, sizeof(cap));

    aph_set_log_level(APH_LOG_DEBUG);
    aph_set_log_callback(capture_callback, &cap);

    aph_log(APH_LOG_WARN, "cloth", "Constraint stretch exceeded threshold.");
    APH_CHECK(cap.call_count == 1);
    APH_CHECK(cap.last_level == APH_LOG_WARN);
    APH_CHECK(strcmp(cap.last_module, "cloth") == 0);
    APH_CHECK(strcmp(cap.last_msg, "Constraint stretch exceeded threshold.") == 0);
    APH_CHECK(cap.last_user == &cap);

    /* NULL module and NULL message substitution in aph_log */
    aph_log(APH_LOG_ERROR, NULL, NULL);
    APH_CHECK(cap.call_count == 2);
    APH_CHECK(cap.last_level == APH_LOG_ERROR);
    APH_CHECK(strcmp(cap.last_module, "unknown") == 0);
    APH_CHECK(strcmp(cap.last_msg, "(no message)") == 0);

    aph_set_log_callback(NULL, NULL);
}

static void test_log_level_filtering(void) {
    log_capture cap;
    memset(&cap, 0, sizeof(cap));

    aph_set_log_callback(capture_callback, &cap);

    /* 1. Default level is INFO: DEBUG should be dropped */
    aph_set_log_level(APH_LOG_INFO);
    aph_log(APH_LOG_DEBUG, "core", "Debug trace");
    APH_CHECK(cap.call_count == 0);

    aph_log(APH_LOG_INFO, "core", "Info notice");
    APH_CHECK(cap.call_count == 1);

    aph_log(APH_LOG_WARN, "core", "Warning notice");
    APH_CHECK(cap.call_count == 2);

    aph_log(APH_LOG_ERROR, "core", "Error notice");
    APH_CHECK(cap.call_count == 3);

    /* 2. Filter at WARN: INFO and DEBUG dropped */
    aph_set_log_level(APH_LOG_WARN);
    aph_log(APH_LOG_INFO, "core", "Should be dropped");
    APH_CHECK(cap.call_count == 3);

    aph_log(APH_LOG_WARN, "core", "Warn passed");
    APH_CHECK(cap.call_count == 4);

    aph_log(APH_LOG_ERROR, "core", "Error passed");
    APH_CHECK(cap.call_count == 5);

    /* 3. Filter at ERROR: only ERROR passes */
    aph_set_log_level(APH_LOG_ERROR);
    aph_log(APH_LOG_WARN, "core", "Warn dropped");
    APH_CHECK(cap.call_count == 5);

    aph_log(APH_LOG_ERROR, "core", "Error passed");
    APH_CHECK(cap.call_count == 6);

    /* 4. Filter at DEBUG: all levels pass */
    aph_set_log_level(APH_LOG_DEBUG);
    aph_log(APH_LOG_DEBUG, "core", "Debug passed");
    APH_CHECK(cap.call_count == 7);

    /* 5. Message with printf format characters is treated literally */
    aph_log(APH_LOG_INFO, "core", "Literal specifiers %s %d %x %p must not crash.");
    APH_CHECK(cap.call_count == 8);
    APH_CHECK(strcmp(cap.last_msg, "Literal specifiers %s %d %x %p must not crash.") == 0);

    aph_set_log_callback(NULL, NULL);
    aph_set_log_level(APH_LOG_INFO);
}

static void test_log_callback_replacement_and_reset(void) {
    log_capture cap1;
    memset(&cap1, 0, sizeof(cap1));
    int alt_count = 0;

    aph_set_log_level(APH_LOG_INFO);

    /* Register callback 1 */
    aph_set_log_callback(capture_callback, &cap1);
    aph_log(APH_LOG_INFO, "test", "msg 1");
    APH_CHECK(cap1.call_count == 1);

    /* Replace with callback 2 */
    aph_set_log_callback(alt_capture_callback, &alt_count);
    aph_log(APH_LOG_INFO, "test", "msg 2");
    APH_CHECK(cap1.call_count == 1); /* cap1 unchanged */
    APH_CHECK(alt_count == 1);

    /* Reset callback to NULL */
    aph_set_log_callback(NULL, NULL);
    /* In tests, calling aph_log with NULL callback will use default stderr sink without crashing */
    aph_log(APH_LOG_INFO, "test", "msg 3");
    APH_CHECK(cap1.call_count == 1);
    APH_CHECK(alt_count == 1);
}

int main(void) {
    test_log_level_strings();
    test_log_format_valid();
    test_log_format_truncation_and_edge_cases();
    test_log_format_null_substitutions();
    test_log_callback_delivery();
    test_log_level_filtering();
    test_log_callback_replacement_and_reset();

    if (aph_test_failures == 0) {
        printf("log_test: all checks passed.\n");
    } else {
        fprintf(stderr, "log_test: %d check(s) failed.\n", aph_test_failures);
    }

    return aph_test_failures == 0 ? 0 : 1;
}
