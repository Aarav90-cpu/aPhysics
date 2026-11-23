# Getting Started with aPhysics

This guide walks you through setting up, building, and running aPhysics on your machine.

---

## Prerequisites

1. **Clang 23+**:
   - aPhysics is strictly built with Clang (minimum version 23).
   - GCC is not supported.
   - Verify your compiler version:
     ```bash
     clang --version
     ```

2. **Bazel 9+**:
   - Bazel 9 is the required build tool (version 9.2.0 is pinned in `.bazelversion`).
   - We recommend installing `bazelisk` so that Bazel 9 is invoked automatically.
   - Verify your Bazel version:
     ```bash
     bazel --version
     ```

---

## Cloning the Repository

Clone the repository using SSH or HTTPS:

```bash
# Using SSH (recommended)
git clone git@github.com:Aarav90-cpu/aPhysics.git
cd aPhysics

# Or using HTTPS
git clone https://github.com/Aarav90-cpu/aPhysics.git
cd aPhysics
```

---

## Building and Running

### Build All Targets

```bash
bazel build //...
```

### Run the Physics Engine Executable

```bash
bazel run //:app
```

### Execute Tests

```bash
bazel test //...
```

### Clean Workspace

```bash
bazel clean
```

---

## Next Steps

- Explore the [Architecture & Zero-Copy Memory Model](architecture.md).
- If you are running on Apple Silicon or Intel macOS, read [Looking for macOS Testers!](macos_testers.md).
- Learn about how AI coding agents interface with the codebase in [Agent Skills](agent_skills.md).
