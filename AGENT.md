# AGENT.md - Ship Battle Simulator

This repository is the incremental teaching build. The current source of truth is [README.md](README.md), the phase plan is [docs/PHASE_PLAN.md](docs/PHASE_PLAN.md), and the current completed checkpoint is documented in [docs/PHASE_33_EXPLANATION.md](docs/PHASE_33_EXPLANATION.md).

## Current phase boundary

Phase 37 is the latest implemented phase. Stages A, B and C are complete, and **Stage D has finished its first part, the hierarchy method (Phases 34 to 37)**. A small prototype ship stands at the back of the sea, built from `src/Ship.h` (which contains no OpenGL): a root, a hull and a deck; two masts, each with a yard and a sail drawn from both sides; a red flag at the main masthead; and a cannon with a mount, a yoke (azimuth) and a barrel (elevation) whose `MUZZLE_Z` tip is readable as a position (`w = 1`) and a direction (`w = 0`). The scene is 28 draws / 2208 triangles / 1471 vertices. Keys added in Stage D: `H` only.

**Treasure, wildlife and the ship's interior (TAB walks below deck; F uses ladders, hatches and chests): see addendum 5 of docs/ENVIRONMENT_BUILD.md.** **Camera views (F6 free, F7 first person, F8 cinematic, F9 captain) and a steadier enemy: see addendum 3 of docs/ENVIRONMENT_BUILD.md.** **Living crew, waves, reflections (after the environment build):** see the addendum in [docs/ENVIRONMENT_BUILD.md](docs/ENVIRONMENT_BUILD.md). **Environment build (after the playable build).** The showcase has five selectable locations (open ocean, jungle island, mountain coast, rocky islands, foggy coast), five weathers (sunny, cloudy, rainy, misty, storm) and three times of day (day, sunset, night), chosen from an options menu or with `I`, `C`, `T`; weather and time can also be DYNAMIC (the sun moves, the weather changes by itself, both closed forms of the game clock). There is no menu while playing: **ESC pauses and opens the menu** (Resume / Quit), and the game clock stops with it. Ships have 20 hit points, and ships and land are solid (a chain of four circles per hull, `src/Scenery.h`): they are pushed apart and never overlap. See [docs/ENVIRONMENT_BUILD.md](docs/ENVIRONMENT_BUILD.md). The default (open ocean, sunny, sunset) is the previous build unchanged. Rules that still hold: exactly two lights (the moon uses the sun slot; lanterns are emissive geometry), no textures, no shadows, no new mesh generator (scenery is the existing sphere, cylinder and cube), no stored animation (rain, lightning, clouds, sway and rocking are closed forms of the clock), and the gallery (`G`) is drawn exactly as before. `shaders/effects.vert` and `effects.frag` draw only the panel, rain, stars and glows; the lit scene still has one program and one shading branch.

The ship starts rolled 45 degrees ON PURPOSE (`ShipConfig::PROOF_ROLL`): that is the Phase 37 proof, and `H` clears the root's roll and pitch so it has something to clear. Position and heading belong to the player and `H` never touches them.

**Stage D has been widened.** The student supplied a brief for a detailed pirate galleon in the style of the Jackdaw (Assassin's Creed IV: Black Flag) and the Pirates of the Caribbean ships, and the plan now carries it as **Phases 38 to 78**. The prototype proved the method; those phases build the ship with it. The brief was checked against the project's rules first, and the plan's "brief, checked against the project's rules" table records every conflict and its resolution. The decisions that matter when coding:

- **The "five meshes, never a sixth" rule is superseded, in exactly three places:** `makeHull` (Phase 47), a plank strip (51) and `makeSail` (60). Everything else stays on the existing meshes. Do not add a generator the plan does not name.
- **Shadows are not done**, deliberately. The report's position is that this pipeline is local illumination only (L8 s10-11).
- **Exactly two lights, always.** Lanterns are emissive geometry (the emission term arrives in Phase 44), never light sources.
- **No textures and no skybox.** Wood comes from per-vertex colour (Phases 48-51); the sky is a gradient dome from the existing sphere (Phase 45).
- **The test gallery is hidden by `G` (Phase 38) and Demo A and Demo B stay reachable with it on.** Showcase lighting and MSAA are profiles; the regression runs in the old profile so it stays byte-identical to Phase 33.
- **Camera keys avoid the ship and cannon keys:** `F1`-`F5` presets, `R` reset, `,` `.` yaw, `PageUp`/`PageDown` pitch, `Q`/`E` zoom.
- **"Looks like the movie or game" is defined, not promised.** The plan gives a proportion table and a feature checklist, both measurable, and Phase 78 ends with the student reviewing screenshots against reference images. Those proportions are design targets, not measurements of any film or game model - say so if asked.

The next phase is **Phase 38, the showcase scene**, in [docs/PHASE_PLAN.md](docs/PHASE_PLAN.md): `scaleShipDimensions(d, k)` (scaling the dimension DATA, never a matrix) and a `G` key that hides or restores the test gallery, hidden by default. It passes when, with the gallery on and the ship hidden, ten renders are byte-identical to Phase 33. Nothing else. Stage E (player control, now Phases 79 and 80) comes after Stage D and was renumbered; the old Phase 38 is now Phase 79. Do not start a phase unless the student asks.

**Nothing after Stage D may break Demo A, Demo B, or the hierarchy rules.** Demo A and Demo B are described under "Stage C rules" below. For the ship: every frame is a rigid motion; no part inherits another's size; the `H` proof (a child's local transform never changes when the root's rotation is cleared) must still hold.

## Stage D rules that must keep holding

- **A frame stores translation and rotation only. Scale is applied once, in the matrix given to `drawMesh`** - in the model functions in `src/Ship.h`. A part built from another part's *scaled* matrix comes out crushed or stretched: measured 3% of its intended height for a mast on the deck's matrix, 2% for the barrel on the mount's.
- **`src/Ship.h` must stay free of OpenGL**, so every claim about the frames can be tested exactly on the CPU.
- **Dimensions are passed in as a struct**, not read from constants inside the builder, so a different ship can be built and compared in the same run.
- **Each part is a child of the one above.** The flag hangs from the main YARD, not the sail, so that the sail can swing about its yard (Phase 84) without taking the flag with it. The flag hoist (the pivot at the masthead) is a separate frame from the flag (the cloth's centre).
- **Anything drawn from a quad is drawn twice** - the second time with a frame turned half a revolution IN PLACE - because back-face culling is on. The turn must be about the cloth's own centre, or the copy lands on the wrong side of its pivot (this produced two flags once).
- **The elevation is applied as `R_x(-elevation)`.** A rotation about `+x` by a positive angle lowers a barrel pointing along `+z`; the reference guide's snippet omits the minus sign.
- **The muzzle position takes `w = 1` and the muzzle direction `w = 0`.** Getting it backwards aims the shot at the world origin.
- **`H` clears roll and pitch and nothing else.** Position and heading belong to the player; roll and pitch belong to the sea. They live in separate fields of `ShipPose` for exactly this reason.
- **A regression test must compare against the real previous phase, byte for byte.** With the ship hidden, the render must be identical to Phase 33's across ten views. That is far stronger than "the old tests still pass".

## Stage C rules that must keep holding

**The two demonstrations.** Demo A (L9 s28): the sea at `n_s` 160 - Gouraud's highlight peaks at 186 and Phong's at 255 at CELLS 8, and Phong's image is identical across tessellations (0 pixels differ between 8 and 16 cells) while Gouraud's changes by about 17,000 pixels. Demo B (L9 s27): the 8-segment brass tube - Flat uses 42 brightness levels against Phong's 216. If a change stops either working, the change is wrong.

- **One shader program, one branch.** Never split Flat/Gouraud/Phong into separate programs.
- **The lighting code exists once** (`src/Lighting.h`). Never paste a second copy into a `.vert` or `.frag`; a copy that drifts makes the Gouraud/Phong comparison a lie.
- **Material values are verbatim.** Brass, polished silver and black plastic are L8 slide 60; do not tune them. The `n_s` spread of 4 to 160 is deliberate.
- **Two lights, no more.** The ambient term is added once, outside the per-light work.
- **A normal is transformed by `(M^-1)^T` and renormalised**, in the vertex shader; `normalize()` is also needed in the fragment shader after interpolation.
- **`uViewPos` must be refreshed every frame**, from the same variable the view matrix is built from.
- **A test that compares an image to a prediction must predict the WHOLE model**, not one term of it. Fitting only attenuation on a surface with a strong specular term failed for exactly this reason.

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

**Detail is not more objects.** The draw count is the same at every detail level (10 when Phase 25 was written, 28 since the ship arrived) while the triangle count changes by orders of magnitude (92 to 36,116 in Phase 25; the ship adds a further 368, a few of which - the masts, yards and barrel - follow the detail level too because they share the cylinder mesh). Those two numbers are independent and are improved by different means, which is the argument Phase 103 makes; keep both in the title.

Rebuild in the main loop between `processInput()` and `updateScene()`, never inside the key handler - the input code must not own GPU objects - and set a `needsRebuild` flag to carry the request across.

A counter must be **measured, not predicted**. The `W` key's grid line (Phase 19) is a prediction; the title is a measurement. Keep both - a prediction and a measurement that disagree have found a bug. The title reports vertices **submitted** per frame (437 at level 0), not vertices resident on the GPU (386); the gap is mesh reuse. Keep `glfwSetWindowTitle` behind a change check: it is an operating-system call and re-setting an identical string every frame is waste that can flicker, while the counting itself still happens every frame. The loop's local for the returned stats is named `frameCost`, not `stats`, because `main()` already has a `FrameStats stats` for timing.

**A leak test must have a negative control.** "Nothing leaked" is unfalsifiable on its own: prove the test detects a leak by disabling `destroy()` and showing the lowest free GL object name climb (15 to 615 for 300 uploads), then re-enable it.

Six rules hold permanently. The unit-mesh rule: a generator produces a shape 1 unit across centred on its own origin and takes no size parameter, a stored frame holds translation and rotation only, and `glm::scale` is applied solely inside the `drawMesh` call. Division (`CELLS`, `SEGMENTS`, `STACKS`, `SLICES`) and size (`SIZE`, `DIAMETER`, `HEIGHT`) are separate ideas. A closed ring shares its seam vertices with a modulo, where an open row needs `N + 1`. **A vertex carries one value** - one normal and one colour - so two surfaces meeting at a point need separate vertices only when they genuinely disagree. A pole or cap band is a triangle FAN, never a collapsed lattice row, so no degenerate triangles are produced. And normals are ANALYTIC wherever the shape allows it, with `computeSmoothNormals()` reserved for shapes that have no formula to write down.

`computeSmoothNormals()` is a direct implementation of L9 slide 20 and must keep its citation; it belongs in the report. It only changes anything on a mesh whose vertices are SHARED - running it on the flat 24-vertex cube provably changes nothing, which is the Phase 23 viva answer.

Verify the strict-warning build from PowerShell, not Git Bash: MSYS rewrites `/W4` into a file path, and a failed configure can look identical to a clean build. Check three things - the `CMakeCache.txt` flag value, the generated project's `WarningLevel`, and `/W4` on the real `cl.exe` command line (which needs `cmake --build ... -- /v:detailed`, because the default verbosity never echoes it) - plus that the executable was produced. When overriding `CMAKE_CXX_FLAGS` for that build, pass `/DWIN32 /D_WINDOWS /EHsc /W4` and not `/W4` alone: replacing the flags wholesale drops CMake's default `/EHsc` and produces a `C4530` from inside a system header that has nothing to do with the project.

Scan build logs for `warning` and `error` **anywhere**, not for compiler warnings only. A `LINK :` line carries no `C####` code, and filtering for compiler diagnostics hid a real `LNK4098` for 24 phases. The vendored `glfw3.lib` is built against the release CRT, so Debug builds exclude the release runtime via `LINK_FLAGS_DEBUG` in [CMakeLists.txt](CMakeLists.txt) - do not remove that line, and do not use `target_link_options` to replace it, which needs CMake 3.13 while this project declares 3.10.

There are no known open defects. Phase 25 found and fixed two of its own before shipping: non-reversible detail (now prevented by `static_assert`) and the `W` key predicting from the starting grid division instead of the current one. Phase 24 found and fixed one pre-existing defect: a Debug-only `LNK4098` caused by the vendored GLFW's release CRT, present since Phase 1. Every "the scene, counted" table from Phase 18 onward was recomputed in Phase 24 - fifteen figures across five documents - and all fifteen verified correct, with the current frame confirmed against a live measurement. One accepted cosmetic observation is recorded at the end of [docs/PHASE_20_EXPLANATION.md](docs/PHASE_20_EXPLANATION.md): the test triangle passes through the largest cube about 0.8% of the time, which renders correctly and was left alone deliberately. One checkpoint is superseded and annotated: Phase 20's cylinder counts (`32 / 96 / 32`), which Phase 21 changed to `66 / 192 / 64` by adding the end caps.
