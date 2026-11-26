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

#ifndef APH_LOG_H
#define APH_LOG_H

#include "core/aph_status.h"
#include <stddef.h>

/**
 * Diagnostic log severity levels.
 *
 * Ordered by decreasing severity (ERROR = 0 highest severity, DEBUG = 3 lowest).
 */
typedef enum aph_log_level {
    APH_LOG_ERROR = 0,
    APH_LOG_WARN = 1,
    APH_LOG_INFO = 2,
    APH_LOG_DEBUG = 3
} aph_log_level;

/**
 * Log message receiver callback signature.
 *
 * Notice and Constraints:
 *   - The callback must return quickly and must NOT call engine functions (re-entrancy hazard).
 *   - Messages are delivered as plain strings without dynamic number formatting.
 *
 * Inputs:
 *   level: Severity level of the emitted message.
 *   module: Subsystem identifier (e.g. "core", "cloth", "collision"). Never NULL.
 *   msg: Diagnostic message text. Never NULL.
 *   user: User-defined context pointer passed to aph_set_log_callback().
 */
typedef void (*aph_log_fn)(aph_log_level level, const char *module, const char *msg, void *user);

/**
 * Sets the global log callback and associated user context pointer.
 *
 * Thread Safety and Environment Notice:
 *   - The global callback configuration is not thread-safe. Register the callback before
 *     creating worlds or initiating multi-worker execution.
 *   - In WebAssembly builds, the default sink is completely silent until the host import
 *     mechanism is introduced (step 3.2). On native platforms, the default sink writes
 *     formatted output to stderr.
 *   - Passing fn == NULL restores the default sink behavior.
 *
 * Inputs:
 *   fn: Callback function to invoke, or NULL to restore default logging.
 *   user: Opaque context pointer passed to fn on every invocation.
 */
void aph_set_log_callback(aph_log_fn fn, void *user);

/**
 * Sets the minimum severity threshold for log emission.
 *
 * Messages with severity strictly below min_level (i.e. numerically greater than min_level)
 * are discarded before invoking any sink or callback.
 * Default minimum level is APH_LOG_INFO.
 *
 * Thread Safety:
 *   Not thread-safe; set during initialization before creating simulation worlds.
 *   Out-of-range values are clamped to [APH_LOG_ERROR, APH_LOG_DEBUG].
 *
 * Inputs:
 *   min_level: Minimum severity level to emit.
 */
void aph_set_log_level(aph_log_level min_level);

/**
 * Emits a diagnostic message to the active log sink if level <= minimum threshold.
 *
 * Notice:
 *   - Low-level internal headers (such as aph_arena.h) must NOT call this function;
 *     they should purely return an aph_status code.
 *   - Logging must only occur where a status becomes a caller-visible failure (e.g.
 *     during world creation in step 1.7).
 *   - Error messages must state what happened and provide actionable remediation advice
 *     (e.g. "Out of memory: the world's memory budget is too small. Increase memory_bytes in the
 * world config.").
 *
 * Inputs:
 *   level: Message severity level.
 *   module: Subsystem name. If NULL, substituted with "unknown".
 *   msg: Message string. If NULL, substituted with "(no message)".
 */
void aph_log(aph_log_level level, const char *module, const char *msg);

/**
 * Formats a diagnostic message into the standard format:
 *   "[aphysics] <LEVEL> <module>: <msg>"
 *
 * Freestanding Safe:
 *   Implemented with hand-written character copying (no snprintf or C library dependencies).
 *   Always NUL-terminates when buf_size > 0.
 *
 * Inputs:
 *   out_buf: Destination character buffer.
 *   buf_size: Capacity of out_buf in bytes.
 *   level: Severity level.
 *   module: Subsystem name (substitutes "unknown" if NULL).
 *   msg: Message string (substitutes "(no message)" if NULL).
 *
 * Returns:
 *   APH_OK if the entire formatted string fit into out_buf.
 *   APH_ERR_BUFFER_TOO_SMALL if buf_size == 0 or if truncation occurred.
 *   APH_ERR_INVALID_ARGUMENT if out_buf is NULL while buf_size > 0.
 */
aph_status aph_log_format(char *out_buf, size_t buf_size, aph_log_level level, const char *module,
                          const char *msg);

/**
 * Returns a human-readable string representation of an aph_log_level.
 *
 * Inputs:
 *   level: Severity level.
 *
 * Returns:
 *   "ERROR", "WARN", "INFO", "DEBUG", or "UNKNOWN" if out of range.
 */
const char *aph_log_level_string(aph_log_level level);

#endif /* APH_LOG_H */
