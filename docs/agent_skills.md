# Agent Skills & Automated Tooling

aPhysics is designed from the ground up to be agent-friendly and compatible with modern AI development workflows and documentation crawlers.

---

## Guidelines in `AGENTS.md`

All automated assistants and contributors working on this repository follow the explicit standards defined in [`AGENTS.md`](../AGENTS.md):

- **Strict Tooling**: Only Clang (version 23+) and Bazel (version 9+) are permitted.
- **Verification First**: Changes must be built and tested through Bazel (`bazel build //...` and `bazel test //...`) with exact commands and output reported.
- **License Headers**: All source files must include Apache 2.0 license headers applied via `addlicense`.
- **Zero Hallucination Policy**: No assumptions or claims regarding untested platforms or unverified features.

---

## Compatibility with `fetchIt`

This documentation structure is formatted for **[fetchIt](https://github.com/Aarav90-cpu/fetchIt)**, the documentation crawler and markdown conversion tool created by Aarav (@Aarav90-cpu):

- A root [`sitemap.xml`](../sitemap.xml) explicitly marked `<!-- For FetchIt -->` indexes all documentation pages.
- Clean semantic Markdown files with structured headings, code blocks, and diagrams.
- Clean URLs and relative links enable offline preservation and agent-native retrieval.
