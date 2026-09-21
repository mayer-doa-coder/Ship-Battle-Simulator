# AGENT.md - Ship Battle Simulator

This repository is the incremental teaching build. The current source of truth is [README.md](README.md), the phase plan is [docs/PHASE_PLAN.md](docs/PHASE_PLAN.md), and the current completed checkpoint is documented in [docs/PHASE_10_EXPLANATION.md](docs/PHASE_10_EXPLANATION.md).

## Current phase boundary

Phase 10 is the latest implemented phase. The next phase is Phase 11 in [docs/PHASE_PLAN.md](docs/PHASE_PLAN.md): winding order and `glEnable(GL_CULL_FACE)` explained explicitly, plus a wireframe key, and nothing else. The orbit camera is Phase 12-13, and reusable meshes begin at Phase 14. Do not add any of them unless the student explicitly asks to begin the phase that owns it.

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

## Phase 10 checkpoint

Phase 10 is complete only when the project builds in Debug and Release with no compiler warnings, all shaders link with no missing-uniform warning, a solid and correctly coloured cube is visible and turning, watching it over a few seconds shows genuinely different faces rather than the same ones repeating, changing the cube's size or one face's colour produces the expected visible change, the earlier triangles, quad, and the `D` and `O` keys still behave exactly as before, and the window still reports frame timing and closes normally. The cube remains a temporary test, not a reusable project mesh.
