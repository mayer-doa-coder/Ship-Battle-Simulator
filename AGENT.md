# AGENT.md - Ship Battle Simulator

This repository is the incremental teaching build. The current source of truth is [README.md](README.md), the phase plan is [docs/PHASE_PLAN.md](docs/PHASE_PLAN.md), and the current completed checkpoint is documented in [docs/PHASE_25_EXPLANATION.md](docs/PHASE_25_EXPLANATION.md).

## Current phase boundary

Phase 25 is the latest implemented phase, and **Stage B is complete**. All five meshes exist (cube, quad, grid, cylinder, sphere) and no sixth will be added. The frame's cost is measured in `drawMesh()` and reported in the window title, and `+`/`-` rebuild the three parameterised meshes live across six detail levels (92 to 36,116 triangles) while the draw count stays at 10.

The next phase is Phase 26 in [docs/PHASE_PLAN.md](docs/PHASE_PLAN.md), which **begins Stage C - Illumination, where the marks are**: `uNormalMatrix = (M^-1)^T`, computed on the CPU once per object per frame, one `glm::mat3(glm::transpose(glm::inverse(model)))` per object. It passes when the Phase 15 debug view stays correct on a NON-UNIFORMLY scaled cube, which it provably does not without the normal matrix - so the phase needs a stretched cube to demonstrate on, and the before/after has to be shown. Nothing else. Do not add ambient, diffuse, specular, materials, ships, or anything past that unless the student explicitly asks to begin the phase that owns it. Each Stage C phase adds exactly one term or one comparison.

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

## Phase 25 checkpoint

Phase 25 is complete only when the project builds in Debug and Release with no compiler warnings, all shaders link with no missing-uniform warning, `+` and `-` rebuild the grid, cylinder and sphere at a new detail level, the polygon count changes live (visibly in wireframe and numerically in the title) while the draw count stays at 10, 100 rebuilds produce no OpenGL error and no leak, `-` then `+` returns to exactly the starting detail including the sphere's 12 x 18, a held key steps the level once rather than once per frame, the `W` key predicts from the CURRENT grid division, level 0 is still exactly the Phase 24 scene (10 draws / 640 triangles / 437 vertices), and the window still reports frame timing and closes normally with every GPU object freed.

Three rules now hold permanently.

**A rebuild must not accumulate GPU objects.** `Mesh::upload()` calls `destroy()` first; that single line, written in Phase 14, is the whole reason this works. Never remove it, and never add a second upload path that skips it.

**Detail must be derived, never accumulated.** Store one level and compute each mesh's divisions from its level-0 constant with `detailAtLevel()`. Repeatedly halving a running value loses the remainder, which made `-` `-` `+` `+` leave the sphere at 12 x 12 instead of 12 x 18 - and Phase 22 chose 12 and 18 BECAUSE they differ, so a latitude/longitude mix-up cannot hide. The `static_assert` on `sphereDivisionsDifferAt()` now enforces this at compile time; if a change to a level-0 value, a `_MIN` floor or the level range breaks it, the project will not compile.

**Detail is not more objects.** The draw count is 10 at every level while triangles span 92 to 36,116. Those two numbers are independent and are improved by different means, which is the argument Phase 62 makes; keep both in the title.

Rebuild in the main loop between `processInput()` and `updateScene()`, never inside the key handler - the input code must not own GPU objects - and set a `needsRebuild` flag to carry the request across.

A counter must be **measured, not predicted**. The `W` key's grid line (Phase 19) is a prediction; the title is a measurement. Keep both - a prediction and a measurement that disagree have found a bug. The title reports vertices **submitted** per frame (437 at level 0), not vertices resident on the GPU (386); the gap is mesh reuse. Keep `glfwSetWindowTitle` behind a change check: it is an operating-system call and re-setting an identical string every frame is waste that can flicker, while the counting itself still happens every frame. The loop's local for the returned stats is named `frameCost`, not `stats`, because `main()` already has a `FrameStats stats` for timing.

**A leak test must have a negative control.** "Nothing leaked" is unfalsifiable on its own: prove the test detects a leak by disabling `destroy()` and showing the lowest free GL object name climb (15 to 615 for 300 uploads), then re-enable it.

Six rules hold permanently. The unit-mesh rule: a generator produces a shape 1 unit across centred on its own origin and takes no size parameter, a stored frame holds translation and rotation only, and `glm::scale` is applied solely inside the `drawMesh` call. Division (`CELLS`, `SEGMENTS`, `STACKS`, `SLICES`) and size (`SIZE`, `DIAMETER`, `HEIGHT`) are separate ideas. A closed ring shares its seam vertices with a modulo, where an open row needs `N + 1`. **A vertex carries one value** - one normal and one colour - so two surfaces meeting at a point need separate vertices only when they genuinely disagree. A pole or cap band is a triangle FAN, never a collapsed lattice row, so no degenerate triangles are produced. And normals are ANALYTIC wherever the shape allows it, with `computeSmoothNormals()` reserved for shapes that have no formula to write down.

`computeSmoothNormals()` is a direct implementation of L9 slide 20 and must keep its citation; it belongs in the report. It only changes anything on a mesh whose vertices are SHARED - running it on the flat 24-vertex cube provably changes nothing, which is the Phase 23 viva answer.

Verify the strict-warning build from PowerShell, not Git Bash: MSYS rewrites `/W4` into a file path, and a failed configure can look identical to a clean build. Check three things - the `CMakeCache.txt` flag value, the generated project's `WarningLevel`, and `/W4` on the real `cl.exe` command line (which needs `cmake --build ... -- /v:detailed`, because the default verbosity never echoes it) - plus that the executable was produced. When overriding `CMAKE_CXX_FLAGS` for that build, pass `/DWIN32 /D_WINDOWS /EHsc /W4` and not `/W4` alone: replacing the flags wholesale drops CMake's default `/EHsc` and produces a `C4530` from inside a system header that has nothing to do with the project.

Scan build logs for `warning` and `error` **anywhere**, not for compiler warnings only. A `LINK :` line carries no `C####` code, and filtering for compiler diagnostics hid a real `LNK4098` for 24 phases. The vendored `glfw3.lib` is built against the release CRT, so Debug builds exclude the release runtime via `LINK_FLAGS_DEBUG` in [CMakeLists.txt](CMakeLists.txt) - do not remove that line, and do not use `target_link_options` to replace it, which needs CMake 3.13 while this project declares 3.10.

There are no known open defects. Phase 25 found and fixed two of its own before shipping: non-reversible detail (now prevented by `static_assert`) and the `W` key predicting from the starting grid division instead of the current one. Phase 24 found and fixed one pre-existing defect: a Debug-only `LNK4098` caused by the vendored GLFW's release CRT, present since Phase 1. Every "the scene, counted" table from Phase 18 onward was recomputed in Phase 24 - fifteen figures across five documents - and all fifteen verified correct, with the current frame confirmed against a live measurement. One accepted cosmetic observation is recorded at the end of [docs/PHASE_20_EXPLANATION.md](docs/PHASE_20_EXPLANATION.md): the test triangle passes through the largest cube about 0.8% of the time, which renders correctly and was left alone deliberately. One checkpoint is superseded and annotated: Phase 20's cylinder counts (`32 / 96 / 32`), which Phase 21 changed to `66 / 192 / 64` by adding the end caps.
