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

#include "aphysics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int should_use_color(void) {
    if (getenv("NO_COLOR") != NULL) {
        return 0;
    }
    return isatty(STDOUT_FILENO);
}

int main(void) {
    const char *version = aphysics_version();
    int color = should_use_color();

    if (color) {
        int ver_pad = 35 - (int)strlen(version);
        if (ver_pad < 0) {
            ver_pad = 0;
        }

        /* Styled ANSI terminal output with box border (64 columns total) */
        printf("\033[1;38;5;"
               "99m╭──────────────────────────────────────────────────────────────╮\033[0m\n");
        printf(
            "\033[1;38;5;99m│\033[0m  \033[1;38;5;213m⚡ aPhysics\033[0m — \033[38;5;250mPhysics "
            "Engine for Remotion & Three.js\033[0m        \033[1;38;5;99m│\033[0m\n");
        printf("\033[1;38;5;"
               "99m├──────────────────────────────────────────────────────────────┤\033[0m\n");
        printf(
            "\033[1;38;5;99m│\033[0m  \033[38;5;244mVersion    :\033[0m \033[1;38;5;45m%s\033[0m "
            "\033[38;5;214m(pre-alpha)\033[0m%*s\033[1;38;5;99m│\033[0m\n",
            version, ver_pad, "");
        printf("\033[1;38;5;99m│\033[0m  \033[38;5;244mCreator    :\033[0m \033[1;38;5;255mAarav "
               "Ravindra Kharade\033[0m \033[38;5;245m(@Aarav90-cpu)\033[0m          "
               "\033[1;38;5;99m│\033[0m\n");
        printf("\033[1;38;5;99m│\033[0m  \033[38;5;244mRepository :\033[0m "
               "\033[4;38;5;39mhttps://github.com/Aarav90-cpu/aPhysics\033[0m        "
               "\033[1;38;5;99m│\033[0m\n");
        printf("\033[1;38;5;99m│\033[0m  \033[38;5;244mLicense    :\033[0m "
               "\033[38;5;48mApache-2.0\033[0m                                     "
               "\033[1;38;5;99m│\033[0m\n");
        printf("\033[1;38;5;99m│\033[0m  \033[38;5;244mPlatforms  :\033[0m \033[38;5;48mLinux "
               "[Verified]\033[0m \033[38;5;240m•\033[0m \033[38;5;214mmacOS [Experimental]\033[0m "
               "       \033[1;38;5;99m│\033[0m\n");
        printf("\033[1;38;5;"
               "99m╰──────────────────────────────────────────────────────────────╯\033[0m\n");
    } else {
        /* Clean plain-text fallback (NO_COLOR or non-TTY) */
        printf("----------------------------------------------------------------\n");
        printf("  aPhysics - Physics Engine for Remotion & Three.js\n");
        printf("----------------------------------------------------------------\n");
        printf("  Version    : %s (pre-alpha)\n", version);
        printf("  Creator    : Aarav Ravindra Kharade (@Aarav90-cpu)\n");
        printf("  Repository : https://github.com/Aarav90-cpu/aPhysics\n");
        printf("  License    : Apache-2.0\n");
        printf("  Platforms  : Linux [Verified] - macOS [Experimental]\n");
        printf("----------------------------------------------------------------\n");
    }

    return 0;
}
