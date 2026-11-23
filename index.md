# aPhysics Web & Documentation Hub

Welcome to the documentation portal for **aPhysics**, a modern, high-performance physics engine built in C for **Remotion** and **Three.js** animations.

> 🌐 **Interactive Web Experience**: Open [`website/index.html`](website/index.html) in your browser for the full fluid, animated dark/light UI with integrated search and documentation browser.

---

## Quick Navigation

- [Documentation: Getting Started](docs/getting_started.md)
- [Documentation: Zero-Copy Architecture & Seeds](docs/architecture.md)
- [Platform Notice: Looking for macOS Testers!](docs/macos_testers.md)
- [Agent Standards: Agent Skills & Tools](docs/agent_skills.md)
- [Interactive Web App](website/index.html)
- [GitHub Repository](https://github.com/Aarav90-cpu/aPhysics)

---

## About the Project

aPhysics bakes realistic cloth dynamics, collision reactions, tearing, and deformation directly into 3D scenes so that Remotion video renders and Three.js canvas animations look vibrant, lifelike, and physically grounded.

### Core Architecture Pillars

1. **Share Memory Directly with Three.js (Zero-Copy)**:
   Vertex positions are modified directly inside WebAssembly linear memory and bound to `THREE.BufferAttribute(positions, 3)`. WebGL streams changes straight to the GPU VBO without intermediate JavaScript array copies.
2. **Deterministic Simulation via 64-bit Seeds**:
   Pass a fixed 64-bit seed (e.g., `1337`) for frame-exact, bit-reproducible video rendering across distributed Remotion workers, or use `0` for interactive randomness.
3. **Clang 23 & Bazel 9 Only**:
   Engineered for maximum optimization and modern build ergonomics. GCC and Makefiles are not supported.

---

## Platform Availability

| Platform | Status | Details |
| :--- | :--- | :--- |
| **Linux (x86_64)** | 🟢 **Supported & Tested** | Primary development and CI platform. |
| **macOS (Apple Silicon & Intel)** | 🟡 **Experimental & Untested** | Builds are configured for macOS, but physical hardware testing is ongoing. **[We are actively seeking macOS testers!](docs/macos_testers.md)** |

---

## Cloning & Quickstart

```bash
# Clone the repository
git clone git@github.com:Aarav90-cpu/aPhysics.git
cd aPhysics

# Build all targets with Bazel 9 and Clang 23
bazel build //...

# Run the test suite
bazel test //...

# Run the physics engine binary
bazel run //:app
```

---

## Agent Skills & Guidelines

This repository follows strict autonomous agent guidelines in [`AGENTS.md`](AGENTS.md). All AI coding agents, background workflows, and crawlers adhere to:
- Documented Clang 23 and Bazel 9 commands.
- Verification through `bazel build //...` and `bazel test //...`.
- Apache 2.0 license headers applied to all source files via `addlicense`.
- Dedicated integration with **[fetchIt](https://github.com/Aarav90-cpu/fetchIt)** using [`sitemap.xml`](sitemap.xml).

---

## About the Creator

**Aarav (Arphd4 / Aarav90-cpu)**  
*Pronouns: he/him • Maharashtra, India (GMT+05:30)*

A hobbyist developer passionate about system programming, low-level OS design, physics engines, UI design languages, and creative music production.

### Notable Projects:
- **ARK OS**: Operating system built on the Linux kernel powered by Swift (`libark`, `sash`).
- **fetchIt**: Fast documentation crawler and clean Markdown converter (on PyPI as `fetchit-md` & Arch AUR as `fetchit-git`).
- **MusicSkills**: AI-assisted music generation via Python's `mido` library, complete with a DAW and audio tools.
- **Expressive-Glass UI**: A hybrid design experiment synthesizing Apple's Liquid Glass fluidity with Google Material 3 Expressive shapes and motion.
- **Music & YouTube**: Produces songs and animations on YouTube channel [**@HandleITP**](https://www.youtube.com/@HandleITP).

### Connect:
- GitHub: [Aarav90-cpu](https://github.com/Aarav90-cpu)
- Reddit: [u/Ok_Sky3062](https://www.reddit.com/user/Ok_Sky3062/) • [r/ARKEverything](https://www.reddit.com/r/ARKEverything/)
- YouTube: [@HandleITP](https://www.youtube.com/@HandleITP)
- Instagram: [@arkhandlesitp](https://www.instagram.com/arkhandlesitp/)
- Email: `aarav.kharade1234@gmail.com`
