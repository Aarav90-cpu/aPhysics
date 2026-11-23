# Contributing to aPhysics

Thank you for your interest in contributing to aPhysics!

## Requirements

- **C Compiler**: Clang (version 23 or newer). GCC is not supported.
- **Build System**: Bazel (version 9 or newer, pinned via `.bazelversion`).
- **Language**: C (C17 standard).
- **License**: Apache 2.0.

## Development Workflow

### Building the Project

```bash
# Build all targets
bazel build //...

# Build specific binary
bazel build //:app
```

### Running the Application

```bash
bazel run //:app
```

### Running Tests

```bash
bazel test //...
```

### Cleaning Build Artifacts

```bash
bazel clean
```

## Coding and Contribution Guidelines

1. **Compiler Warnings**: Code must compile without warnings under `-Wall -Wextra -Werror`.
2. **Formatting**: Ensure your code conforms to `.clang-format` before submitting pull requests:
   ```bash
   clang-format --dry-run --Werror src/*.c include/*.h
   ```
3. **License Headers**: Every new source file must include the Apache 2.0 license header. Apply it using `addlicense`:
   ```bash
   addlicense -l apache -c "The aPhysics Authors" src/*.c include/*.h
   ```
4. **Pull Requests**:
   - Write clear commit messages.
   - Fill out the pull request template completely.
   - Ensure `bazel test //...` passes cleanly.
