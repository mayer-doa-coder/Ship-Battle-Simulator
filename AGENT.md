# AGENT.md - Ship Battle Simulator

This repository is the incremental teaching build. The current source of truth is [README.md](README.md), the phase plan is [docs/PHASE_PLAN.md](docs/PHASE_PLAN.md), and the current completed checkpoint is documented in [docs/PHASE_14_EXPLANATION.md](docs/PHASE_14_EXPLANATION.md).

## Current phase boundary

Phase 14 is the latest implemented phase: `src/Mesh.h` now holds a `Vertex{position, color}` struct and a move-only `Mesh` class owning its own VAO/VBO/EBO, replacing the old `QuadGpu`/`CubeGpu` structs and their four create/destroy functions; the cube's geometry moved into `makeCube()`. The next phase is Phase 15 in [docs/PHASE_PLAN.md](docs/PHASE_PLAN.md): add a `normal` field to `Vertex`, give `makeCube()` real per-face normals, and add a normals-as-colour debug view (`N * 0.5 + 0.5`) toggled by a key. Do not add lighting, ships, or anything past reusable meshes unless the student explicitly asks to begin the phase that owns it.

**Stage order changed on 22 September 2026.** After Phase 25 finishes Stage B, the next stage is the new Stage D (Phases 26-30: ship, cannon, crew - object construction only, no lighting or motion), not the old Stage C. Lighting now starts at Phase 31. See [docs/PHASE_PLAN.md](docs/PHASE_PLAN.md)'s numbering note for why. Do not jump to old phase numbers from memory - re-read the plan when Phase 25 is reached.

## Permanent project requirements

- Use C++17, OpenGL 3.3 Core, GLSL 330, GLFW, GLAD, GLM, and CMake.
- Never use Python anywhere in the project or its setup instructions.
- Do not use a game engine, physics engine, scene-graph library, model loader, or GUI toolkit.
- Work in small phases with one visual checkpoint each.
- After a change, explain **what**, **why**, and **how** in simple language.
- Keep transform and tuning values named and centralized; a teacher must be able to request a translation, rotation, scale, speed, colour, or count change with only a small edit.
- Prefer plain structs, small free functions, and direct GLM matrix operations over advanced architecture.
- Keep parent matrices unscaled. Apply object scale only when drawing so child objects are not distorted.
- Use `w = 1` for positions and `w = 0` for directions when homogeneous coordinates are introduced.

## Teacher-required final capabilities

The finished project will contain:

- a keyboard-controlled player ship;
- multiple ships at sea;
- user-controlled cannon firing and trajectory;
- visible crew with lookout, cannon/reload, helm, and helper roles;
- sun, moonlight, rainy, and winter modes;
- the environment modes implemented after the core simulator is working.

These requirements are future phases, not permission to implement them all at once.

## Phase 14 checkpoint

Phase 14 is complete only when the project builds in Debug and Release with no compiler warnings, the default view with no input looks identical to Phase 13's, `W`/`D`/`O`/left-drag orbit/scroll-wheel zoom all still behave exactly as before now that the quad and cube are `Mesh` objects, shutdown frees every GPU object while the context still exists with no leaks, and `main.cpp` no longer contains `QuadGpu`, `CubeGpu`, or their four create/destroy functions. `src/Mesh.h`'s `Vertex` intentionally has no `normal` field yet - it stores `position` and `color`, the same pair every object has used since Phase 2, because nothing reads a normal until Phase 15's debug view. Do not add `normal` early just because the reference guide's wording for this phase mentions it.
