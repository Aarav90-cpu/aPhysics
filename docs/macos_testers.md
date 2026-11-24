# Looking for macOS Testers!

> **Platform Status**: Linux (Tested & Supported) | macOS (Experimental & Untested)

aPhysics is currently developed and tested on Linux (x86_64). While Bazel 9 and Clang 23 support macOS (both Apple Silicon `arm64` and Intel `x86_64`), macOS builds are currently **experimental and untested on physical hardware**.

We are actively seeking contributors and testers on macOS!

---

## Why We Need Your Help

- **Apple Silicon (M1/M2/M3/M4) Architecture Verification**: Validating SIMD instructions, floating-point determinism, and Clang 23 compilation on Darwin.
- **Bazel 9 Toolchain Support**: Ensuring that `@rules_cc` and local Xcode/Command Line Tools integrate smoothly without build warnings.
- **Performance Profiling**: Benchmarking simulation speed and WebAssembly compilation targets on macOS.

---

## How to Test on macOS

If you have a Mac running macOS 13+:

1. **Install Prerequisites**:
   - Xcode 26.0+ Command Line Tools (with Apple Clang 21+) or upstream Clang 23+
   - Bazelisk (`brew install bazelisk`)

2. **Clone and Build**:
   ```bash
   git clone git@github.com:Aarav90-cpu/aPhysics.git
   cd aPhysics
   bazel build //...
   ```

3. **Run the Binary & Tests**:
   ```bash
   bazel run //tools:aphysics_info
   bazel test //...
   ```

4. **Report Your Results**:
   - If you encounter build issues, compiler warnings, or test failures, please open an issue using the [Bug Report Template](https://github.com/Aarav90-cpu/aPhysics/issues/new?template=bug_report.yml).
   - Include your macOS version, chip architecture (`uname -m`), Clang version (`clang --version`), and terminal output.

Thank you for helping make aPhysics cross-platform!
