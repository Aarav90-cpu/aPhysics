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

#ifndef APH_STATUS_H
#define APH_STATUS_H

/**
 * Standard status codes returned by internal engine functions.
 */
typedef enum aph_status {
    APH_OK = 0,
    APH_ERR_INVALID_ARGUMENT = 1,
    APH_ERR_OUT_OF_MEMORY = 2,
    APH_ERR_BUFFER_TOO_SMALL = 3,
    APH_ERR_NOT_INITIALIZED = 4
} aph_status;

/**
 * Returns a human-readable string representation of an aph_status code.
 *
 * Inputs:
 *   status: Error code to describe.
 *
 * Returns:
 *   Null-terminated constant string naming the status code.
 */
static inline const char *aph_status_string(aph_status status) {
    switch (status) {
    case APH_OK:
        return "APH_OK";
    case APH_ERR_INVALID_ARGUMENT:
        return "APH_ERR_INVALID_ARGUMENT";
    case APH_ERR_OUT_OF_MEMORY:
        return "APH_ERR_OUT_OF_MEMORY";
    case APH_ERR_BUFFER_TOO_SMALL:
        return "APH_ERR_BUFFER_TOO_SMALL";
    case APH_ERR_NOT_INITIALIZED:
        return "APH_ERR_NOT_INITIALIZED";
    default:
        return "APH_ERR_UNKNOWN";
    }
}

#endif /* APH_STATUS_H */
