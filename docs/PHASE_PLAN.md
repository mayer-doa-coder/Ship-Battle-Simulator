# Phase Plan - Ship Battle Simulator

This file replaces the short list in `README.md` and the 16-row table in
`../../docs/CURRENT_STATUS_AND_TEACHER_ROADMAP.md` section 5 as the phase plan
for this repository. Those tables grouped several concepts into one phase. This
plan splits them so that every phase is the same size as Phase 0, Phase 1, and
Phase 2: **one concept, one visual checkpoint, one explanation document.**

## Numbering

Phase documents are zero-based: `PHASE_0_EXPLANATION.md` is the OpenGL window.
The old one-based list in `README.md` ("1. OpenGL window ... complete") was off
by one against the file names. This plan uses the file-name numbering only.

## Completed

| Phase | Outcome | Document |
|---:|---|---|
| 0 | OpenGL window, GLAD loading, clear colour, clean exit | [PHASE_0_EXPLANATION.md](PHASE_0_EXPLANATION.md) |
| 1 | Render loop split into clock/input/update/render/report, `dt` clamp, FPS report | [PHASE_1_EXPLANATION.md](PHASE_1_EXPLANATION.md) |
| 2 | Checked shader loader, VAO/VBO, one temporary coloured triangle | [PHASE_2_EXPLANATION.md](PHASE_2_EXPLANATION.md) |

Current source: `src/main.cpp`, `src/Shader.h`, `shaders/basic.vert`,
`shaders/basic.frag`. There are no matrices, meshes, lights, ships, or gameplay
yet, and `ShaderProgram` has no uniform setters yet.

## How to read the tables

- **Adds** - the only new idea in that phase.
- **Checkpoint** - what must be visible on screen before the next phase starts.
- **Viva change** - the small edit a teacher can ask for during that phase.
- **Ref** - the matching phase in the `Broadside` reference guide,
  `../../docs/IMPLEMENTATION_GUIDE_Broadside.md`. Read it, then write the code
  here yourself. Never copy `Broadside/src` into this repository.
- ***** - a graded milestone. Do not merge these with a neighbour.
- **(o)** - small enough to merge with the phase above it if the deadline is tight.

---

## Stage A - From flat triangle to 3D space (Phases 3-6)

Replaces the single old "model/view/projection" phase.

| Phase | Adds | Checkpoint | Viva change | Ref |
|---:|---|---|---|---|
| 3 | Uniform setters on `ShaderProgram` (`setMat4`, `setVec3`, `setFloat`, `setInt`), then `uProjection` and `uView`: `glm::perspective` and `glm::lookAt` applied to the existing triangle | The triangle sits in a 3D world; pushing it along `-Z` makes it smaller | Change the field of view or the near/far planes | B3 |
| 4 | `uModel`: translation, rotation, and scale, and why the order is `T * R * S` | The triangle translates, spins from `glfwGetTime()`, and scales, each controllable separately | Swap two matrices in the product and explain the different result | B3 |
| 5 | The first real 3D object: an indexed cube (EBO), with depth testing and back-face culling proved | A solid spinning cube with correct near and far faces; turning off `GL_DEPTH_TEST` visibly breaks it | Change one cube dimension | B3 |
| 6 * | `src/Camera.h`: orbit camera with `radius`, `yaw`, `pitch`; mouse drag orbits, wheel zooms, pitch clamped to 89 degrees | The cube can be inspected from any angle with no stretching | Change the zoom limits or the orbit speed | B3 |

Stage A retires the temporary triangle. From Phase 5 onward every object is a mesh.

---

## Stage B - Reusable geometry (Phases 7-11)

Replaces the single old "reusable primitive meshes" phase.

| Phase | Adds | Checkpoint | Viva change | Ref |
|---:|---|---|---|---|
| 7 * | `src/Mesh.h`: `struct Vertex { position; normal; }`, a `Mesh` that owns VAO/VBO/EBO and draws itself, `makeCube()` as a **unit** cube, and a `drawMesh(mesh, model)` helper | Three cubes of different sizes drawn from **one** mesh, scaled only at draw time | Change a cube's scale and show that the mesh data did not change | B4 |
| 8 (o) | `makeQuad()` and `makeGrid(N)` - the first *parameterised* generator - plus a wireframe toggle | A flat grid whose cell count visibly changes with `N` in wireframe | Change the grid resolution | B4 |
| 9 | `makeCylinder(segments)` with analytic side normals `normalize(vec3(x,0,z))` and cap normals `(0,+-1,0)` | A smooth-silhouetted cylinder whose segment count is visible in wireframe | Change the segment count | B4 |
| 10 | `makeSphere(stacks, slices)` with `normal = normalize(position)`, and `computeSmoothNormals()` (L9 slide 20) for meshes with no analytic normal | A sphere, plus a report-ready copy of the averaging formula | Change stacks and slices independently | B4 |
| 11 | `rebuildMeshes()` driven by `+` / `-`, and live draw-call and triangle counters | A mesh gallery showing every primitive; `+`/`-` changes the polygon count at runtime | Change the minimum or maximum tessellation limit | B15.1 |

Everything later - ship, cannon, crew, rain - is one of these five meshes scaled
and placed. That reuse is the project's main optimization argument.

---

## Stage C - Illumination (Phases 12-19)

Replaces the single old "lighting and named materials" phase. This is the
largest expansion in the plan, because this stage is where the marks are. Each
phase adds exactly one term or one comparison.

| Phase | Adds | Checkpoint | Viva change | Ref |
|---:|---|---|---|---|
| 12 | Normals reaching the shader: the second vertex attribute, `uNormalMatrix = (M^-1)^T` computed on the CPU, and a `FragColor = vec4(N*0.5+0.5, 1.0)` debug view | Normals-as-RGB shows correct, stable colours on a non-uniformly scaled cube | Stretch a cube and show the normal matrix correcting it | B5.3 |
| 13 | Ambient plus diffuse from one directional sun, evaluated per fragment; `uGlobalAmbient`, `uKa`, `uKd` | A sphere with a clear lit side and dark side | Change the sun direction | B5 |
| 14 * | Specular `(R.V)^n`, `uKs`, `uShininess`, and `uViewPos` | The highlight **moves** when the camera orbits | Change shininess and watch the highlight tighten | B5 |
| 15 | `src/Material.h` with the L8 slide 60 table: brass, polished silver, black plastic, plus tuned ocean, hull wood, and sailcloth | Three spheres side by side that are obviously different materials - the report screenshot for the `n_s` comparison | Swap one object's material | B6 |
| 16 | The second light: a point light with attenuation `1/(a0 + a1*d + a2*d^2)`, and the `L` key to isolate each light | Moving the point light near an object visibly brightens it; `L` proves each light's contribution separately | Change the attenuation constants | B6 |
| 17 * | `uniform int uShadingMode` with one branch in **one** program: Flat (`dFdx`/`dFdy`), Gouraud (lit in the vertex shader), Phong (lit in the fragment shader); keys `1`/`2`/`3`. `computeLighting` lives in one C++ string prepended to both stages | Keys `1`/`2`/`3` give three visibly different images of identical geometry | Explain why three shader programs were not used | B5 |
| 18 | `K` term mask (ambient / +diffuse / +specular / full - L8 slide 54) and the `B` Blinn-Phong `(N.H)^n` toggle | `K` walks through the lecture slide live in your own scene | Change the `K` cycle order | B15.2 |
| 19 * | Verification only, no new feature: the two demonstrations are set up and photographed | **Demo A** - grid at `N` = 8-16 with `n_s` = 160: the highlight vanishes in Gouraud and returns in Phong. **Demo B** - cylinder at 6-8 segments: Flat shows Mach bands, Gouraud smears the highlight, Phong keeps it round | Lower the tessellation until Gouraud fails | B15.3 |

Nothing after Stage C may break Phase 19. If a later change stops either
demonstration working, the change is wrong.

---

## Stage D - The ship hierarchy (Phases 20-23)

Replaces the old "one simple hierarchical ship" phase. Still no animation.

| Phase | Adds | Checkpoint | Viva change | Ref |
|---:|---|---|---|---|
| 20 * | `src/Ship.h`: the root and the hull and deck **frames**. The rule that a frame stores translation and rotation only, and `glm::scale` appears only inside `drawMesh` | A hull and a deck at the right size, with the deck not inheriting the hull's stretch | Change the hull dimensions and show the deck is unaffected | B7 |
| 21 | The rigging chain: mast, yard, sail, and flag, each a child of the one above | A recognisable ship silhouette | Move the mast and watch the sail and flag follow | B7 |
| 22 | The cannon chain: mount to yoke (azimuth) to barrel (elevation), with a named `MUZZLE_Z` tip | A cannon on the deck; editing `azimuth`/`elevation` by hand swings it correctly | Change the mount position on the deck | B7 |
| 23 * | Proof: rotating the ship root 45 degrees carries every child, and the `H` key clears the **root's** rotation only | With `H`, children keep their own local transforms while the root is identity | Change the root rotation angle | B7 |

---

## Stage E - Player control (Phases 24-25)

Replaces the old "player ship movement and steering" phase. Teacher
requirement: keyboard ship control.

| Phase | Adds | Checkpoint | Viva change | Ref |
|---:|---|---|---|---|
| 24 * | `PlayerMotion { position, heading, speed }`; `W`/`S` change speed, `A`/`D` change heading, both scaled by `dt`. Keyboard camera zoom moves to `Q`/`E` | The ship sails and turns; hull, rigging, and cannon all follow the root | Change `PLAYER_TURN_SPEED` | B16 |
| 25 (o) | Movement bounds, and a camera target that follows the player root | The ship cannot sail out of the world and stays framed | Change the sea bounds | B16 |

Storing position and heading is allowed. It is interactive state, not a
pre-computed animation table.

---

## Stage F - The living sea (Phases 26-29)

Replaces the old "animated sea and ship rocking" phase.

| Phase | Adds | Checkpoint | Viva change | Ref |
|---:|---|---|---|---|
| 26 * | `src/Wave.h`: the seven wave literals written **once**, generating both the C++ constants and the GLSL text. GPU displacement in the vertex shader behind `uIsOcean` | The grid becomes moving water | Change one amplitude and see the CPU and GPU still agree | B8.1 |
| 27 | Analytic wave normals from the partial derivatives | The sun streak glitters and rolls across the water; the normals-as-RGB view shows smooth moving bands, not one flat colour | Change a wave frequency | B8.1 |
| 28 * | CPU `waveHeight`, `waveSlopeX`, `waveSlopeZ`, and `shipMatrix()` giving heave, roll, and pitch | The ship rides the water it is standing on; `H` now also freezes the rocking | Change the `0.8` rocking factor | B8.2 |
| 29 | Idle rigging animation: `R_z(A sin(wt + phi))` on the sails and `R_y` on the flag, each with a different phase. `P` pauses time | Sails and flag move independently even with nothing else happening; with `H` on, they keep moving on a still hull | Change one sail's phase offset | B9 |

---

## Stage G - Aiming the cannon (Phases 30-32)

Replaces the old "one manually aimable cannon" phase.

| Phase | Adds | Checkpoint | Viva change | Ref |
|---:|---|---|---|---|
| 30 | Arrow-key azimuth and elevation, clamped to -5 to 45 degrees | The barrel traverses and elevates; the brass highlight sweeps along it | Change the elevation limits | B11 |
| 31 * | Reading the muzzle out of the hierarchy: position with `w = 1`, forward direction with `w = 0`, plus a debug line drawn along the barrel axis | The debug line leaves the barrel mouth and points where the gun aims, including while the hull rocks | Change `MUZZLE_Z` | B12.1 |
| 32 | Auto-track: the target converted into the ship's local space, `wrapAngle()` for the shortest signed angle, a rate-limited slew, and `TAB` to switch modes | The gun tracks a moving point smoothly and never spins the long way round | Change the slew rate | B11 |

`wrapAngle` is not optional. Without it the turret swings 357 degrees the wrong
way every time the bearing crosses the `atan2` branch cut.

---

## Stage H - Ballistics (Phases 33-35)

Replaces the old "cannonball and adjustable trajectory" phase.

| Phase | Adds | Checkpoint | Viva change | Ref |
|---:|---|---|---|---|
| 33 * | `fire()` and the closed form `p(tau) = p0 + v0*tau + 0.5*g*tau^2`, evaluated fresh from the launch time and never accumulated | `SPACE` launches a ball that arcs; firing at the top and at the bottom of a roll gives different arcs | Change gravity | B12.2 |
| 34 * | Trajectory control: a named `g_muzzleSpeed` on `[` / `]` between clamped limits, and the low-arc lift `theta = 0.5*asin(g*R/v^2)` | A deliberately short flat shot and a deliberately high long shot, on demand | Change the muzzle speed limits | B18 |
| 35 | Reload state: `SPACE` fires only when ready, `g_reloadReadyAt = now + RELOAD_SECONDS`, plus the 0.15 s muzzle-flash point light | Holding `SPACE` cannot bypass the reload; each shot flashes the barrel | Change `RELOAD_SECONDS` | B18 |

---

## Stage I - The fleet (Phases 36-38)

Replaces the old "one additional ship, then a small fleet" phase. Teacher
requirement: more ships in the sea.

| Phase | Adds | Checkpoint | Viva change | Ref |
|---:|---|---|---|---|
| 36 | One extra ship: **the same** `drawShip()` with a different root, a reduced detail level, and a different hull material, on a sine patrol | Two ships, rocking differently because each samples the wave under itself; zero new meshes | Change the patrol radius | B10 |
| 37 * | A fixed ship array holding per-instance data (position, heading, speed, patrol centre, phase, material, flags); default three non-player ships | Four ships from one ship function, with the count as a named constant | Change the ship count | B17 |
| 38 | Target selection: a selected-target index and `T` to cycle; aiming and collision read the index instead of a single global | The gun can be pointed at any ship in the fleet | Change the starting target | B17 |

---

## Stage J - Impact and effects (Phases 39-41)

Replaces the old "cannonball collision, reload state, and clear feedback" phase.

| Phase | Adds | Checkpoint | Viva change | Ref |
|---:|---|---|---|---|
| 39 * | A hull hit test that **sweeps** the segment travelled this frame, a brief emission flash on the struck ship, and a `HIT` result string | Aiming well reports `HIT` and the struck hull glows; the verdict is the same at 30 and at 144 FPS | Change `HIT_RADIUS` | B13 |
| 40 (o) | The sea test, run **after** the hull test, reporting `SPLASH` | A short shot reports `SPLASH` where it meets the water | Explain why the hull is tested first | B13 |
| 41 | Fixed particle pools - smoke, spray, sparks - with position derived from age, and blending enabled for particles only and then switched off again | Firing repeatedly produces smoke and splashes and never allocates in the render loop | Change a particle's life or growth | B14 |

---

## Stage K - Crew (Phases 42-46)

Replaces the old "simple transform-based crew roles" phase. Teacher
requirement: visible people with roles, one role per phase.

| Phase | Adds | Checkpoint | Viva change | Ref |
|---:|---|---|---|---|
| 42 * | `src/Crew.h`: a body built from reused primitives, and a crew root parented to the deck frame | One crew member stands on the deck and is carried correctly by steering and by the waves | Move the crew member's deck post | B19 |
| 43 | Lookout: a fixed elevated post plus `R_y(scanAngle)` | The lookout scans from the mast | Change the scan amplitude | B19 |
| 44 | Helmsman: fixed at the wheel, with body or arm angle following the player's steering input | Turning with `A`/`D` visibly turns the helmsman | Change the arm travel | B19 |
| 45 | Cannon crew: interpolation between named ready and reload poses, driven by the Phase 35 reload progress | The crew works the gun in time with the reload and is idle when ready | Change a pose | B19 |
| 46 | Helpers: a small fixed set moving along named deck-local waypoints | Several crew move about the deck without leaving it | Move one waypoint | B19 |

Waypoints are deck locations, not a frame-by-frame animation table. The crew
must never need skeletal animation or an imported model.

---

## Stage L - HUD and optimization (Phases 47-48)

Replaces the old "teaching HUD and optimization pass" phase.

| Phase | Adds | Checkpoint | Viva change | Ref |
|---:|---|---|---|---|
| 47 | The full window-title HUD: shading mode, environment, speed, heading, target, aim mode, azimuth, elevation, muzzle speed, reload, flight time, last result, ship and crew counts, draw calls, and FPS | A grader can read every piece of state without opening the code | Add or remove one displayed field | B21 |
| 48 * | Measurement pass: unique meshes and their reuse, idle and effect draw/triangle counts, uniforms hoisted out of the per-object loop, no allocation in the loop, and measured FPS | The report's optimization section contains real measured numbers, not the old 20-draw ceiling | Explain one measurement | B22 |

---

## Stage M - Environment modes (Phases 49-51)

Replaces the old "environment modes" phase. These stay **last**, exactly as the
teacher required, and are never started while an earlier checkpoint fails.

| Phase | Adds | Checkpoint | Viva change | Ref |
|---:|---|---|---|---|
| 49 * | `enum class EnvironmentMode`, one settings function supplying clear colour, global ambient, and sun direction/colour/intensity; `4` sun and `5` moonlight | Two clearly different times of day; ship, cannon, and crew keep working in both | Change the moonlight colour | B20 |
| 50 | Rain mode on `6`: a darker palette plus a fixed downward rain pool | Rain falls and the scene is visibly overcast | Change the rain speed or count | B20 |
| 51 | Winter mode on `7`: a cold palette plus a slow drifting snow pool | Snow drifts and the palette is cold | Change the snow drift | B20 |

One shader system receives different mode values. Four shader programs would be
a scope violation.

---

## Stage N - Submission (Phase 52)

| Phase | Adds | Checkpoint | Ref |
|---:|---|---|---|
| 52 | Report and viva rehearsal: traceability matrix, illumination equation and material table with slide citations, the Flat/Gouraud/Phong and ocean-highlight evidence, the player-root transform explanation, fleet and ship hierarchy diagrams, a crew screenshot naming every role, two trajectories, one screenshot per environment mode, final measurements, and the local-versus-global illumination paragraph | The 3-minute demonstration can be performed twice without touching code | B23 |

---

## If the deadline gets close

Merge in this order, and never merge a `*` phase:

1. The `(o)` phases: 8 into 7, 25 into 24, 40 into 39.
2. Stage B phases 9 and 10 into one geometry phase.
3. Stage K helpers (46) into the cannon crew phase (45).
4. Stage M rain and winter (50, 51) into one weather phase.

Never cut, because each is an explicit teacher requirement: keyboard ship
control, multiple ships, manual firing with trajectory adjustment, the four crew
roles, all four environment modes, the three shading modes, the two lights with
attenuation, the hierarchy, the ballistic projectile, and material variety.

## Rules that do not change between phases

- One phase at a time. Do not start the next until the current checkpoint passes
  and you can explain and modify it.
- Every phase ends with a `docs/PHASE_n_EXPLANATION.md` written in the same
  what / why / how style as Phases 0-2.
- Keep tuning values named and grouped so a teacher can ask for a change and get
  it with one small edit.
- Parent frames stay unscaled; scale is applied only when drawing.
- `w = 1` for positions, `w = 0` for directions.
- No Python anywhere, and no game engine, physics engine, model loader, or
  skeletal-animation framework.
- Read `Broadside` to understand a technique, then write it here yourself.
