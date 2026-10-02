# AGENT.md - Ship Battle Simulator

This repository is the incremental teaching build. The current source of truth is [README.md](README.md), the phase plan is [docs/PHASE_PLAN.md](docs/PHASE_PLAN.md), and the current completed checkpoint is documented in [docs/PHASE_15_EXPLANATION.md](docs/PHASE_15_EXPLANATION.md).

## Current phase boundary

Phase 15 is the latest implemented phase. The next phase is Phase 16 in [docs/PHASE_PLAN.md](docs/PHASE_PLAN.md): the unit-mesh rule and `drawMesh(mesh, model)` - scale applied in the model matrix and never baked into vertex data - demonstrated by three differently sized cubes drawn from one mesh and one VAO. This is a milestone phase (marked `*` in the plan). Nothing else. Do not add lighting, materials, further mesh generators, ships, or anything past that unless the student explicitly asks to begin the phase that owns it.

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

## Phase 15 checkpoint

Phase 15 is complete only when the project builds in Debug and Release with no compiler warnings, all shaders link with no missing-uniform warning for `uDebugNormals`, the picture with `N` not pressed is exactly Phase 14's, pressing `N` paints every pixel from its normal as `N * 0.5 + 0.5` and pressing it again returns, holding `N` toggles exactly once per press, each cube face shows one flat colour that stays steady as the cube turns, the `+X` face's colour can be predicted before the key is pressed, the `D`, `W`, and `O` keys and the mouse orbit and zoom all still behave exactly as before, and the window still reports frame timing and closes normally. The normal is shown untransformed and un-normalized on purpose: the normal matrix is Phase 26 and `normalize()` belongs with the curved surfaces of Phases 20-22.
