## Description

Please include a summary of the change and the motivation behind it. If this addresses an existing issue, please link it below.

Fixes #(issue)

---

## Type of Change

Please delete options that are not relevant:

- [ ] **Bug fix** (non-breaking change which fixes an issue)
- [ ] **New feature** (non-breaking change which adds functionality)
- [ ] **Breaking change** (fix or feature that would cause existing functionality to not work as expected)
- [ ] **Performance improvement** / Optimization
- [ ] **Documentation update**
- [ ] **Refactoring / Code quality**
- [ ] **CI / Build system changes**

---

## How Has This Been Tested?

Please describe the tests that you ran to verify your changes:

- [ ] Built with Clang 23 and Bazel 9 (`bazel build //...`)
- [ ] Executed tests with Bazel (`bazel test //...`)
- [ ] Verified application run (`bazel run //tools:aphysics_info`)
- [ ] Verified clean target (`bazel clean`)
- [ ] Verified formatting (`clang-format --dry-run --Werror ...`)
- [ ] Verified Apache 2.0 license headers (`addlicense -check ...`)

---

## Checklist

- [ ] My code adheres to the project rules in `AGENTS.md`.
- [ ] Code compiles with Clang 23 without warnings (`-Wall -Wextra -Werror`).
- [ ] Code passes formatting check against `.clang-format`.
- [ ] Apache 2.0 license headers are present via `addlicense`.
- [ ] Tests pass locally via `bazel test //...`.
- [ ] Corresponding updates to documentation are included.
