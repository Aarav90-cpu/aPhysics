# Terminal UI & Design Guidelines

> **Notice**: This document outlines the terminal design rules, color palettes, typography, and accessibility fallback standards for CLI tools within `aPhysics`.

---

## 1. Design Philosophy

CLI tools in aPhysics (such as `//tools:aphysics_info`, bake utilities, and dump tools) follow the **Expressive-Glass / Modern Terminal** aesthetic:
- **Clean Structure**: Use unicode box-drawing frames (`╭───╮`, `│`, `╰───╯`) to separate information clearly.
- **Visual Hierarchy**: Highlight primary brand and status with bright accents, secondary information in muted tones, and data values in crisp contrasting colors.
- **Restraint**: Avoid excessive flashing or rainbow effects; keep colors purposeful and functional.

---

## 2. Palette & Color Standards

When ANSI color is supported, use the following 256-color palette standards:

| Role | Color / Code | Usage |
| :--- | :--- | :--- |
| **Border / Frame** | Indigo / Violet (`\033[1;38;5;99m`) | Window frames, dividers, and boxes |
| **Brand Accent** | Electric Fuchsia (`\033[1;38;5;213m`) | `⚡ aPhysics` brand highlight |
| **Version Accent** | Bright Cyan (`\033[1;38;5;45m`) | Version numbers, core metrics |
| **Labels / Keys** | Muted Slate (`\033[38;5;244m`) | Key names (`Version :`, `Creator :`) |
| **Values / Text** | Bright White / Pearl (`\033[1;38;5;255m`) | Creator name, primary content |
| **Hyperlinks / URLs** | Sky Blue Underline (`\033[4;38;5;39m`) | Repository and documentation URLs |
| **Success / Verified** | Emerald Green (`\033[38;5;48m`) | Verified tests, Linux supported |
| **Warning / Status** | Amber Gold (`\033[38;5;214m`) | `(pre-alpha)`, macOS experimental |
| **Reset** | Plain Reset (`\033[0m`) | Must follow every colored sequence |

---

## 3. Mandatory Fallback Rules (`NO_COLOR` & `isatty`)

Terminal tools **must never emit broken escape codes** into pipes, log files, or non-ANSI terminals:

1. **`NO_COLOR` Compliance**:
   - If the `NO_COLOR` environment variable is set (regardless of its value), disable all ANSI color escape sequences (per [no-color.org](https://no-color.org)).
2. **TTY Detection (`isatty`)**:
   - Check if standard output is an interactive terminal using `isatty(STDOUT_FILENO)`.
   - If output is piped to a file, another program, or a test runner (`!isatty(1)`), automatically disable ANSI colors and Unicode box art in favor of clean ASCII delimiters (`----`).
3. **Implementation Pattern in C**:
   ```c
   static int should_use_color(void) {
       if (getenv("NO_COLOR") != NULL) {
           return 0;
       }
       return isatty(STDOUT_FILENO);
   }
   ```
