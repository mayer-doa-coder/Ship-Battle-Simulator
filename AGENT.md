# AGENT.md - Ship Battle Simulator

This repository is the incremental teaching build. The current source of truth is [README.md](README.md), the phase plan is [docs/PHASE_PLAN.md](docs/PHASE_PLAN.md), and the current completed checkpoint is documented in [docs/PHASE_7_EXPLANATION.md](docs/PHASE_7_EXPLANATION.md).

## Current phase boundary

Phase 7 is the latest implemented phase. The next phase is Phase 8 in [docs/PHASE_PLAN.md](docs/PHASE_PLAN.md): `GL_DEPTH_TEST` proved with two overlapping triangles at different `z`, plus a key to switch it off, and nothing else. This one is marked `(o)` in the plan, small enough to merge into a neighbouring phase if the deadline is tight. Indexed drawing and the first cube are Phase 9-10, the orbit camera is Phase 12-13, and reusable meshes begin at Phase 14. Do not add any of them unless the student explicitly asks to begin the phase that owns it.

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

## Phase 7 checkpoint

Phase 7 is complete only when the project builds in Debug and Release with no compiler warnings, both shaders link with no missing-uniform warning, the triangle is visible with correct proportions in three dimensions, moving it along `-Z` makes it shrink rather than vanish or distort, resizing the window keeps its proportions correct instead of stretching it, changing the field of view or the near/far planes visibly and correctly changes what is drawn, and the window still reports frame timing and closes normally. The triangle remains a temporary test, not a reusable project mesh.
