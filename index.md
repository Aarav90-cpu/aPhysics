# aPhysics Web & Documentation Hub

Welcome to the documentation portal for **aPhysics**, a physics engine in C for **Remotion** and **Three.js** animations.

> 🌐 **Interactive Web Experience**: Open [`website/index.html`](website/index.html) in your browser for the full fluid, animated dark/light UI with integrated search and documentation browser.

---

## Quick Navigation

- [Documentation: Getting Started](docs/getting_started.md)
- [Architecture: Engine Design & Core Subsystems](docs/architecture.md)
- [Platform Notice: Looking for macOS Testers!](docs/macos_testers.md)
- [Agent Standards: Agent Skills & Tools](docs/agent_skills.md)
- [Interactive Web App](website/index.html)
- [GitHub Repository](https://github.com/Aarav90-cpu/aPhysics)

---

## Current Status & Roadmap

The build/CI foundation is in place, Phase 1 C core utilities are complete, and Phase 2 native cloth is in progress. See [docs/architecture.md](docs/architecture.md) for subsystem details.

- [x] Bazel 9 build setup (`9.2.0`)
- [x] Clang 23+ compiler toolchain configuration
- [x] C17 standard enforcement via `.bazelrc`
- [x] CI checks (`clang-format`, `cppcheck`, sanitizers on Linux; experimental macOS job)
- [x] Phase 1 Core & Phase 2 Steps 2.1–2.3 — see [docs/architecture.md](docs/architecture.md)
- [ ] *[Planned]* Milestone 1: Particle & spring-mass integrator
- [ ] *[Planned]* Zero-Copy Three.js linear memory bridge
- [ ] *[Planned]* Deterministic 64-bit seed reproduction
- [ ] *[Planned]* Cloth simulation & soft-body movement
- [ ] *[Planned]* Crash dynamics & deformation mechanics

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

# Run the binary
bazel run //tools:aphysics_info
```

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
