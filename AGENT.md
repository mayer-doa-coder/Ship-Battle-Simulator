# AGENT.md - Ship Battle Simulator

This repository is the incremental teaching build. The current source of truth is [README.md](README.md), the phase plan is [docs/PHASE_PLAN.md](docs/PHASE_PLAN.md), and the current completed checkpoint is documented in [docs/PHASE_11_EXPLANATION.md](docs/PHASE_11_EXPLANATION.md).

## Current phase boundary

Phase 11 is the latest implemented phase. The next phase is Phase 12 in [docs/PHASE_PLAN.md](docs/PHASE_PLAN.md): `src/Camera.h`, an orbit camera with `radius`, `yaw`, `pitch`, driven first by the arrow keys, and nothing else. Mouse drag and scroll wait for Phase 13. Reusable meshes begin at Phase 14. Do not add any of them unless the student explicitly asks to begin the phase that owns it.

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

## Phase 11 checkpoint

Phase 11 is complete only when the project builds in Debug and Release with no compiler warnings, all shaders link with no missing-uniform warning, pressing `W` switches every shape to a wireframe view showing every face front and back and prints the `ON` message, pressing `W` again returns to the normal solid view and prints the `OFF` message, holding `W` down produces exactly one toggle per press rather than one per frame, deliberately reversing one cube face's winding leaves a visible hole in solid mode with that face's outline reappearing in wireframe mode, the earlier triangles, quad, cube, and the `D` and `O` keys still behave exactly as before, and the window still reports frame timing and closes normally. The cube remains a temporary test, not a reusable project mesh.
