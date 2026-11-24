#!/usr/bin/env bash
#
# Copyright 2026 Aarav Ravindra Kharade
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

set -euo pipefail

# Determine compiler binary (respects argument $1, environment $CC, or defaults to clang)
COMPILER="${1:-${CC:-clang}}"

if ! command -v "$COMPILER" >/dev/null 2>&1; then
    echo "Error: compiler '$COMPILER' not found in PATH." >&2
    exit 1
fi

# Verify the compiler is Clang
IS_CLANG=$(echo "__clang__" | "$COMPILER" -E -P - 2>/dev/null | tr -d '[:space:]')
if [ "$IS_CLANG" != "1" ]; then
    echo "Error: Compiler '$COMPILER' is not Clang. aPhysics strictly requires Clang (AGENTS.md)." >&2
    exit 1
fi

# Extract Clang major version directly from compiler preprocessor macro
CLANG_MAJOR=$(echo "__clang_major__" | "$COMPILER" -E -P - 2>/dev/null | tr -d '[:space:]')
if [ -z "$CLANG_MAJOR" ] || ! [ "$CLANG_MAJOR" -eq "$CLANG_MAJOR" ] 2>/dev/null; then
    echo "Error: Failed to extract __clang_major__ from '$COMPILER'." >&2
    exit 1
fi

OS_NAME=$(uname -s)

if [ "$OS_NAME" = "Darwin" ]; then
    # On macOS, Xcode 26.0 and up is required
    if command -v xcodebuild >/dev/null 2>&1; then
        XCODE_RAW=$(xcodebuild -version 2>/dev/null | head -n 1)
        XCODE_VER=$(echo "$XCODE_RAW" | sed -E 's/^Xcode[[:space:]]+//')
        XCODE_MAJOR=$(echo "$XCODE_VER" | cut -d. -f1)
        if [ -n "$XCODE_MAJOR" ] && [ "$XCODE_MAJOR" -eq "$XCODE_MAJOR" ] 2>/dev/null; then
            if [ "$XCODE_MAJOR" -lt 26 ]; then
                echo "Error: macOS requires Xcode 26.0 or newer (found $XCODE_RAW)." >&2
                exit 1
            fi
        fi
    fi

    # Check Apple Clang build macro vs upstream Clang
    APPLE_BUILD=$(echo "__apple_build_version__" | "$COMPILER" -E -P - 2>/dev/null | tr -d '[:space:]')
    if [ "$APPLE_BUILD" != "__apple_build_version__" ]; then
        if [ "$CLANG_MAJOR" -lt 26 ]; then
            echo "Error: Apple Clang version 26 or higher is required for Xcode 26.0+ (found Apple Clang $CLANG_MAJOR)." >&2
            exit 1
        fi
        echo "Verified: Apple Clang $CLANG_MAJOR (Xcode 26+ compatible) on macOS."
    else
        # Upstream Clang on macOS requires version 23+
        if [ "$CLANG_MAJOR" -lt 23 ]; then
            echo "Error: Clang version 23 or higher is required (found version $CLANG_MAJOR)." >&2
            exit 1
        fi
        echo "Verified: Upstream Clang $CLANG_MAJOR on macOS."
    fi
else
    # Linux and other platforms require Clang 23+
    if [ "$CLANG_MAJOR" -lt 23 ]; then
        echo "Error: Clang version 23 or higher is required (found version $CLANG_MAJOR)." >&2
        exit 1
    fi
    echo "Verified: Clang version $CLANG_MAJOR on $OS_NAME."
fi

exit 0
