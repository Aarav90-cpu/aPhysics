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
#include <stdbool.h>

#ifndef __wasm__
#include <stdio.h>
#endif

static aph_log_fn g_log_fn = NULL;
static void *g_log_user = NULL;
static aph_log_level g_min_level = APH_LOG_INFO;

const char *aph_log_level_string(aph_log_level level) {
    switch (level) {
    case APH_LOG_ERROR:
        return "ERROR";
    case APH_LOG_WARN:
        return "WARN";
    case APH_LOG_INFO:
        return "INFO";
    case APH_LOG_DEBUG:
        return "DEBUG";
    default:
        return "UNKNOWN";
    }
}

void aph_set_log_callback(aph_log_fn fn, void *user) {
    g_log_fn = fn;
    g_log_user = user;
}

void aph_set_log_level(aph_log_level min_level) {
    g_min_level = min_level;
}

static size_t append_str(char *dst, size_t dst_size, size_t offset, const char *src,
                         bool *truncated) {
    if (src == NULL) {
        return offset;
    }
    for (size_t i = 0; src[i] != '\0'; ++i) {
        if (offset + 1 < dst_size) {
            dst[offset] = src[i];
        } else {
            *truncated = true;
        }
        offset++;
    }
    return offset;
}

aph_status aph_log_format(char *out_buf, size_t buf_size, aph_log_level level, const char *module,
                          const char *msg) {
    if (buf_size == 0) {
        return APH_ERR_BUFFER_TOO_SMALL;
    }
    if (out_buf == NULL) {
        return APH_ERR_INVALID_ARGUMENT;
    }

    const char *mod = (module != NULL) ? module : "unknown";
    const char *m = (msg != NULL) ? msg : "(no message)";
    const char *lvl_str = aph_log_level_string(level);

    size_t written = 0;
    bool truncated = false;

    written = append_str(out_buf, buf_size, written, "[aphysics] ", &truncated);
    written = append_str(out_buf, buf_size, written, lvl_str, &truncated);
    written = append_str(out_buf, buf_size, written, " ", &truncated);
    written = append_str(out_buf, buf_size, written, mod, &truncated);
    written = append_str(out_buf, buf_size, written, ": ", &truncated);
    written = append_str(out_buf, buf_size, written, m, &truncated);

    size_t term_idx = (written < buf_size) ? written : (buf_size - 1);
    out_buf[term_idx] = '\0';

    if (truncated) {
        return APH_ERR_BUFFER_TOO_SMALL;
    }
    return APH_OK;
}

void aph_log(aph_log_level level, const char *module, const char *msg) {
    /* Severity is ordered descending: ERROR=0 highest, DEBUG=3 lowest */
    if ((int)level < 0 || level > g_min_level) {
        return;
    }

    const char *mod = (module != NULL) ? module : "unknown";
    const char *m = (msg != NULL) ? msg : "(no message)";

    if (g_log_fn != NULL) {
        g_log_fn(level, mod, m, g_log_user);
    } else {
#ifndef __wasm__
        char buf[512];
        aph_status status = aph_log_format(buf, sizeof(buf), level, mod, m);
        if (status == APH_OK || status == APH_ERR_BUFFER_TOO_SMALL) {
            fprintf(stderr, "%s\n", buf);
        }
#endif
    }
}
