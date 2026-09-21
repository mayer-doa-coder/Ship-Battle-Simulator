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

**Renumbered on 21 September 2026.** Stage A and Stage B were re-examined and
found to contain rows with three or four concepts in them. They are now split
into one concept each, which moved Stage A from 4 phases to 11 and Stage B from
5 phases to 12. Everything from Stage C onward kept its order and its content
but moved up by 14 numbers: the old Phase 12 is now Phase 26, and the old final
Phase 52 is now Phase 66.

## Completed

| Phase | Outcome | Document |
|---:|---|---|
| 0 | OpenGL window, GLAD loading, clear colour, clean exit | [PHASE_0_EXPLANATION.md](PHASE_0_EXPLANATION.md) |
| 1 | Render loop split into clock/input/update/render/report, `dt` clamp, FPS report | [PHASE_1_EXPLANATION.md](PHASE_1_EXPLANATION.md) |
| 2 | Checked shader loader, VAO/VBO, one temporary coloured triangle | [PHASE_2_EXPLANATION.md](PHASE_2_EXPLANATION.md) |
| 3 | Uniform setters on `ShaderProgram`, one `uTint` colour filter, warn-once on a missing uniform | [PHASE_3_EXPLANATION.md](PHASE_3_EXPLANATION.md) |
| 4 | `uniform mat4 uModel` built by `glm::translate` from `sinf(now)`, first use of `setMat4` | [PHASE_4_EXPLANATION.md](PHASE_4_EXPLANATION.md) |
| 5 | `glm::rotate` about Z from `now`, joined to the slide as `slide * spin`; the turn is about the origin | [PHASE_5_EXPLANATION.md](PHASE_5_EXPLANATION.md) |
| 6 | `glm::scale` joined as `slide * spin * scaleMat`; an edge-detected `O` key rebuilds it backwards (`scaleMat * spin * slide`) to compare | [PHASE_6_EXPLANATION.md](PHASE_6_EXPLANATION.md) |
| 7 | `uView` from `glm::lookAt` and `uProjection` from `glm::perspective`, added together; the triangle gains real world-space depth motion | [PHASE_7_EXPLANATION.md](PHASE_7_EXPLANATION.md) |
| 8 | The same VAO drawn twice at different depths; an edge-detected `D` key switches `GL_DEPTH_TEST` off and on to prove which one wins and why | [PHASE_8_EXPLANATION.md](PHASE_8_EXPLANATION.md) |
| 9 | A static quad with an EBO: 4 unique vertices, 6 `glDrawElements` indices, no duplicated corners | [PHASE_9_EXPLANATION.md](PHASE_9_EXPLANATION.md) |
| 10 * | A spinning cube: 24 vertices (4 per face, so each face keeps its own flat colour), 36 indices, a tilted spin axis so every face is eventually seen | [PHASE_10_EXPLANATION.md](PHASE_10_EXPLANATION.md) |
| 11 | Winding order and `GL_CULL_FACE` named explicitly; an edge-detected `W` key shows wireframe with culling off, so a deliberately reversed face's outline can be proven still there | [PHASE_11_EXPLANATION.md](PHASE_11_EXPLANATION.md) |

Current source: `src/main.cpp`, `src/Shader.h`, `shaders/basic.vert`,
`shaders/basic.frag`. There are no meshes, lights, ships, or gameplay yet. The
scene draws two triangles, one indexed quad, and one indexed, spinning cube -
the project's first solid 3D object - and both `GL_DEPTH_TEST` and
`GL_CULL_FACE` can now be switched off live to see what each one was doing.

## How to read the tables

- **Adds** - the only new idea in that phase.
- **Checkpoint** - what must be visible on screen before the next phase starts.
- **Viva change** - the small edit a teacher can ask for during that phase.
- **Ref** - the matching phase in the `Broadside` reference guide,
  `../../docs/IMPLEMENTATION_GUIDE_Broadside.md`. Read it, then write the code
  here yourself. Never copy `Broadside/src` into this repository.
- ***** - a graded milestone. Do not merge these with a neighbour.
- **(o)** - small enough to merge with the phase above it if the deadline is tight.

## How far to work right now

**Phase 12 to Phase 25** - the rest of Stage A, then all of Stage B. Phases 3 to 11 are
done. This range is the answer to "I want to work until I am creating 3D
objects".

Three landmarks inside that range:

| At | You have |
|---|---|
| Phase 10 | Your first real 3D object - a solid, spinning, indexed cube |
| Phase 13 | A camera that can inspect that object from any angle |
| Phase 25 | The complete reusable mesh library the whole project is built from |

After Phase 25 the project stops inventing geometry. The ship, the cannon, the
cannonball, the sea, the crew, the rain, and the snow are all the five meshes
from Stage B, scaled and placed by matrices. Only two more shapes are ever
added, both cosmetic and both optional: a cone and a splash ring. That is why
Stage B is worth doing slowly and properly.

## Why the later stages are still stage-sized

Stage C onward is deliberately left at the coarser size until you reach it. A
phase split is a set of guesses about code that does not exist yet, and those
guesses get less accurate the further ahead they are. When you finish Stage B,
Stage C gets the same treatment Stage A and B just received, against the code
you actually wrote. The stage boundaries and the order will not change.

---

## Stage A - From flat triangle to 3D space (Phases 3-13)

Was 4 phases. Phase 5 alone used to contain indexed drawing, depth testing,
back-face culling, and the cube - four separate ideas, each able to fail on its
own and each with its own debugging method. They are now four phases.

| Phase | Adds | Checkpoint | Viva change | Ref |
|---:|---|---|---|---|
| 3 **done** | Uniform setters on `ShaderProgram` (`setInt`, `setFloat`, `setVec3`, `setMat4`), used to drive one `uniform vec3 uTint` in the fragment shader | A value in C++ changes the triangle's colour, with no change to the shader file | Change the tint | B3 |
| 4 **done** | The first matrix: `uniform mat4 uModel` built by `glm::translate`, animated with `sinf(now)` | The triangle slides left and right | Change the travel distance | B3 |
| 5 **done** | `glm::rotate` about the Z axis with `angle = now` | The triangle spins. Moving it off centre first makes it **orbit** instead, which is the lesson: rotation happens about the origin | Change the spin speed or the axis | B3 |
| 6 **done** | `glm::scale`, and the `T * R * S` order, with a key that builds the product backwards on purpose | A sliding, spinning, pulsing triangle; the wrong order visibly smears or orbits | Swap two matrices and explain the result | B3 |
| 7 * **done** | `uView` from `glm::lookAt` and `uProjection` from `glm::perspective`, introduced **together** | Pushing the triangle along `-Z` now makes it shrink instead of vanishing | Change the field of view or the near/far planes | B3 |
| 8 (o) **done** | `GL_DEPTH_TEST` proved with two overlapping triangles at different `z`, and a key to switch it off | With depth off the later draw wins; with depth on the nearer one wins | Swap the draw order | B3 |
| 9 **done** | Indexed drawing: an EBO and `glDrawElements`, used to build a quad from 4 vertices and 6 indices | A quad made of two triangles that share an edge, with no duplicated corner data | Change one index and see the tear | B4 |
| 10 * **done** | The cube: 24 vertices, 36 indices, one face colour each | A solid 3D cube turning in space | Change a cube dimension or a face colour | B3 |
| 11 **done** | Winding order and `glEnable(GL_CULL_FACE)`, plus a wireframe key | Reversing one face's indices leaves a visible hole in the cube; the wireframe view proves the face is still there | Reverse one face's winding and find the hole | B4 |
| 12 | `src/Camera.h`: `radius`, `yaw`, `pitch`, and the spherical-to-Cartesian position, driven by the arrow keys first | The cube can be inspected from any angle | Change the orbit speed | B3 |
| 13 * | GLFW cursor and scroll callbacks: drag to orbit, wheel to zoom, pitch clamped to 89 degrees, radius clamped | Dragging orbits smoothly and the view never flips over at the poles | Change the zoom limits | B3 |

**Why Phase 7 is one phase and not two.** The view and projection matrices
cannot be introduced separately and still leave something on screen. A view
matrix alone pushes the triangle to `z = -3`, which is outside the clip cube
`[-1, 1]`, so it disappears. A projection matrix alone, with the camera sitting
on top of the object, divides by a near-zero `w`. The smallest step that ends in
a visible, correct picture contains both.

**Note on Phase 3.** If `glGetUniformLocation` returns `-1`, the uniform is not
being used in the shader and the compiler removed it. That is the most common
silent uniform failure, and it is worth reproducing on purpose once.

---

## Stage B - Reusable geometry (Phases 14-25)

Was 5 phases. The old Phase 7 contained the vertex struct, the mesh class, the
unit-mesh rule, and the draw helper at once, and the old Phase 9 built the
cylinder wall and its caps in one step even though they use different
techniques and different normals.

| Phase | Adds | Checkpoint | Viva change | Ref |
|---:|---|---|---|---|
| 14 * | `src/Mesh.h`: `struct Vertex { glm::vec3 position, normal; }` and a `Mesh` that owns its VAO/VBO/EBO and draws itself. The cube moves into `makeCube()` | The picture is identical to Phase 13, `main.cpp` is shorter, and shutdown still frees every GPU object | Explain what the struct owns and when it is freed | B4 |
| 15 | A normals-as-colour debug view: the shader writes `N * 0.5 + 0.5`, toggled by a key | Each cube face shows one flat, predictable colour for its axis | Predict the colour of the `+X` face before pressing the key | B8 |
| 16 * | The unit-mesh rule and `drawMesh(mesh, model)`: scale is applied in the model matrix, never in the vertex data | Three differently sized cubes drawn from **one** mesh and one VAO | Change one cube's scale and show the mesh data did not change | B4 |
| 17 | `makeQuad()` - the simplest generator, 4 vertices, normal `(0, 0, 1)` | A quad that the debug view confirms is facing `+Z` | Change the quad's size | B4 |
| 18 | `makeGrid(N)` - the first **parameterised** generator, with nested loops building vertices and indices | A flat grid; `N` is a named value | Change `N` | B4 |
| 19 (o) | Wireframe toggle with `glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)` | `N = 4` and `N = 32` are obviously different in wireframe | Count the triangles at `N = 4` by eye and check it against `2 * N * N` | B15.1 |
| 20 | `makeCylinder(segments)`, side wall only: a ring of vertices from `sinf`/`cosf`, with the analytic normal `normalize(vec3(x, 0, z))` | An open tube whose debug view sweeps smoothly through the colours around its circumference | Change the segment count | B4 |
| 21 (o) | The cylinder caps: a centre vertex and a triangle fan at each end, normals `(0, +1, 0)` and `(0, -1, 0)` | A closed solid; the caps are two flat colours against the sweeping side wall | Change the cylinder height | B4 |
| 22 | `makeSphere(stacks, slices)` - a two-parameter surface with `normal = normalize(position)` | A sphere that the debug view renders as the classic RGB ball | Change stacks and slices independently | B4 |
| 23 * | `computeSmoothNormals()` - the L9 slide 20 averaging formula - demonstrated on a shared-vertex cube against the flat 24-vertex cube | The two cubes are obviously different in the debug view: hard face colours against blended corners | Explain why the 24-vertex cube cannot be smoothed | B4.2 |
| 24 (o) | Live counters in the window title: draw calls, vertices, triangles | The reported triangle count matches what you can work out by hand for the gallery | Add the vertex count to the title | B15.1 |
| 25 | `rebuildMeshes()` driven by `+` and `-`, with clamped minimum and maximum detail | The polygon count changes live in wireframe, and 100 rebuilds produce no OpenGL error and no leak | Change the detail limits | B15.1 |

By Phase 23 every mesh in the project exists and every one of them is
verifiable by eye, before a single light is switched on. That is the point of
introducing the debug view at Phase 15 rather than with the lighting stage: a
wrong normal is nearly invisible once it is hidden inside a lighting equation.

---

## Stage C - Illumination (Phases 26-33)

Replaces the single old "lighting and named materials" phase. This is where the
marks are. Each phase adds exactly one term or one comparison.

| Phase | Adds | Checkpoint | Viva change | Ref |
|---:|---|---|---|---|
| 26 | The normal matrix `uNormalMatrix = (M^-1)^T`, computed on the CPU once per object per frame | The Phase 15 debug view stays correct on a non-uniformly scaled cube, which it does not without the normal matrix | Stretch a cube and show the correction | B5.3 |
| 27 | Ambient plus diffuse from one directional sun; `uGlobalAmbient`, `uKa`, `uKd` | A sphere with a clear lit side and dark side | Change the sun direction | B5 |
| 28 * | Specular `(R.V)^n`, `uKs`, `uShininess`, and `uViewPos` | The highlight **moves** when the camera orbits | Change shininess and watch the highlight tighten | B5 |
| 29 | `src/Material.h` with the L8 slide 60 table: brass, polished silver, black plastic, plus tuned ocean, hull wood, and sailcloth | Three spheres side by side that are obviously different materials - the report screenshot for the `n_s` comparison | Swap one object's material | B6 |
| 30 | The second light: a point light with attenuation `1/(a0 + a1*d + a2*d^2)`, and the `L` key to isolate each light | Moving the point light near an object visibly brightens it; `L` proves each light's contribution separately | Change the attenuation constants | B6 |
| 31 * | `uniform int uShadingMode` with one branch in **one** program: Flat (`dFdx`/`dFdy`), Gouraud (lit in the vertex shader), Phong (lit in the fragment shader); keys `1`/`2`/`3`. `computeLighting` lives in one C++ string prepended to both stages | Keys `1`/`2`/`3` give three visibly different images of identical geometry | Explain why three shader programs were not used | B5 |
| 32 | `K` term mask (ambient / +diffuse / +specular / full - L8 slide 54) and the `B` Blinn-Phong `(N.H)^n` toggle | `K` walks through the lecture slide live in your own scene | Change the `K` cycle order | B15.2 |
| 33 * | Verification only, no new feature: the two demonstrations are set up and photographed | **Demo A** - grid at `N` = 8-16 with `n_s` = 160: the highlight vanishes in Gouraud and returns in Phong. **Demo B** - cylinder at 6-8 segments: Flat shows Mach bands, Gouraud smears the highlight, Phong keeps it round | Lower the tessellation until Gouraud fails | B15.3 |

Nothing after Stage C may break Phase 33. If a later change stops either
demonstration working, the change is wrong.

---

## Stage D - The ship hierarchy (Phases 34-37)

Replaces the old "one simple hierarchical ship" phase. Still no animation.

| Phase | Adds | Checkpoint | Viva change | Ref |
|---:|---|---|---|---|
| 34 * | `src/Ship.h`: the root and the hull and deck **frames**. The rule that a frame stores translation and rotation only, and `glm::scale` appears only inside `drawMesh` | A hull and a deck at the right size, with the deck not inheriting the hull's stretch | Change the hull dimensions and show the deck is unaffected | B7 |
| 35 | The rigging chain: mast, yard, sail, and flag, each a child of the one above | A recognisable ship silhouette | Move the mast and watch the sail and flag follow | B7 |
| 36 | The cannon chain: mount to yoke (azimuth) to barrel (elevation), with a named `MUZZLE_Z` tip | A cannon on the deck; editing `azimuth`/`elevation` by hand swings it correctly | Change the mount position on the deck | B7 |
| 37 * | Proof: rotating the ship root 45 degrees carries every child, and the `H` key clears the **root's** rotation only | With `H`, children keep their own local transforms while the root is identity | Change the root rotation angle | B7 |

---

## Stage E - Player control (Phases 38-39)

Teacher requirement: keyboard ship control.

| Phase | Adds | Checkpoint | Viva change | Ref |
|---:|---|---|---|---|
| 38 * | `PlayerMotion { position, heading, speed }`; `W`/`S` change speed, `A`/`D` change heading, both scaled by `dt`. Keyboard camera zoom moves to `Q`/`E` | The ship sails and turns; hull, rigging, and cannon all follow the root | Change `PLAYER_TURN_SPEED` | B16 |
| 39 (o) | Movement bounds, and a camera target that follows the player root | The ship cannot sail out of the world and stays framed | Change the sea bounds | B16 |

Storing position and heading is allowed. It is interactive state, not a
pre-computed animation table.

---

## Stage F - The living sea (Phases 40-43)

| Phase | Adds | Checkpoint | Viva change | Ref |
|---:|---|---|---|---|
| 40 * | `src/Wave.h`: the seven wave literals written **once**, generating both the C++ constants and the GLSL text. GPU displacement in the vertex shader behind `uIsOcean` | The grid becomes moving water | Change one amplitude and see the CPU and GPU still agree | B8.1 |
| 41 | Analytic wave normals from the partial derivatives | The sun streak glitters and rolls across the water; the Phase 15 debug view shows smooth moving bands, not one flat colour | Change a wave frequency | B8.1 |
| 42 * | CPU `waveHeight`, `waveSlopeX`, `waveSlopeZ`, and `shipMatrix()` giving heave, roll, and pitch | The ship rides the water it is standing on; `H` now also freezes the rocking | Change the `0.8` rocking factor | B8.2 |
| 43 | Idle rigging animation: `R_z(A sin(wt + phi))` on the sails and `R_y` on the flag, each with a different phase. `P` pauses time | Sails and flag move independently even with nothing else happening; with `H` on, they keep moving on a still hull | Change one sail's phase offset | B9 |

---

## Stage G - Aiming the cannon (Phases 44-46)

| Phase | Adds | Checkpoint | Viva change | Ref |
|---:|---|---|---|---|
| 44 | Arrow-key azimuth and elevation, clamped to -5 to 45 degrees | The barrel traverses and elevates; the brass highlight sweeps along it | Change the elevation limits | B11 |
| 45 * | Reading the muzzle out of the hierarchy: position with `w = 1`, forward direction with `w = 0`, plus a debug line drawn along the barrel axis | The debug line leaves the barrel mouth and points where the gun aims, including while the hull rocks | Change `MUZZLE_Z` | B12.1 |
| 46 | Auto-track: the target converted into the ship's local space, `wrapAngle()` for the shortest signed angle, a rate-limited slew, and `TAB` to switch modes | The gun tracks a moving point smoothly and never spins the long way round | Change the slew rate | B11 |

`wrapAngle` is not optional. Without it the turret swings 357 degrees the wrong
way every time the bearing crosses the `atan2` branch cut.

---

## Stage H - Ballistics (Phases 47-49)

| Phase | Adds | Checkpoint | Viva change | Ref |
|---:|---|---|---|---|
| 47 * | `fire()` and the closed form `p(tau) = p0 + v0*tau + 0.5*g*tau^2`, evaluated fresh from the launch time and never accumulated | `SPACE` launches a ball that arcs; firing at the top and at the bottom of a roll gives different arcs | Change gravity | B12.2 |
| 48 * | Trajectory control: a named `g_muzzleSpeed` on `[` / `]` between clamped limits, and the low-arc lift `theta = 0.5*asin(g*R/v^2)` | A deliberately short flat shot and a deliberately high long shot, on demand | Change the muzzle speed limits | B18 |
| 49 | Reload state: `SPACE` fires only when ready, `g_reloadReadyAt = now + RELOAD_SECONDS`, plus the 0.15 s muzzle-flash point light | Holding `SPACE` cannot bypass the reload; each shot flashes the barrel | Change `RELOAD_SECONDS` | B18 |

---

## Stage I - The fleet (Phases 50-52)

Teacher requirement: more ships in the sea.

| Phase | Adds | Checkpoint | Viva change | Ref |
|---:|---|---|---|---|
| 50 | One extra ship: **the same** `drawShip()` with a different root, a reduced detail level, and a different hull material, on a sine patrol | Two ships, rocking differently because each samples the wave under itself; zero new meshes | Change the patrol radius | B10 |
| 51 * | A fixed ship array holding per-instance data (position, heading, speed, patrol centre, phase, material, flags); default three non-player ships | Four ships from one ship function, with the count as a named constant | Change the ship count | B17 |
| 52 | Target selection: a selected-target index and `T` to cycle; aiming and collision read the index instead of a single global | The gun can be pointed at any ship in the fleet | Change the starting target | B17 |

---

## Stage J - Impact and effects (Phases 53-55)

| Phase | Adds | Checkpoint | Viva change | Ref |
|---:|---|---|---|---|
| 53 * | A hull hit test that **sweeps** the segment travelled this frame, a brief emission flash on the struck ship, and a `HIT` result string | Aiming well reports `HIT` and the struck hull glows; the verdict is the same at 30 and at 144 FPS | Change `HIT_RADIUS` | B13 |
| 54 (o) | The sea test, run **after** the hull test, reporting `SPLASH` | A short shot reports `SPLASH` where it meets the water | Explain why the hull is tested first | B13 |
| 55 | Fixed particle pools - smoke, spray, sparks - with position derived from age, and blending enabled for particles only and then switched off again | Firing repeatedly produces smoke and splashes and never allocates in the render loop | Change a particle's life or growth | B14 |

---

## Stage K - Crew (Phases 56-60)

Teacher requirement: visible people with roles, one role per phase.

| Phase | Adds | Checkpoint | Viva change | Ref |
|---:|---|---|---|---|
| 56 * | `src/Crew.h`: a body built from reused primitives, and a crew root parented to the deck frame | One crew member stands on the deck and is carried correctly by steering and by the waves | Move the crew member's deck post | B19 |
| 57 | Lookout: a fixed elevated post plus `R_y(scanAngle)` | The lookout scans from the mast | Change the scan amplitude | B19 |
| 58 | Helmsman: fixed at the wheel, with body or arm angle following the player's steering input | Turning with `A`/`D` visibly turns the helmsman | Change the arm travel | B19 |
| 59 | Cannon crew: interpolation between named ready and reload poses, driven by the Phase 49 reload progress | The crew works the gun in time with the reload and is idle when ready | Change a pose | B19 |
| 60 | Helpers: a small fixed set moving along named deck-local waypoints | Several crew move about the deck without leaving it | Move one waypoint | B19 |

Waypoints are deck locations, not a frame-by-frame animation table. The crew
must never need skeletal animation or an imported model.

---

## Stage L - HUD and optimization (Phases 61-62)

| Phase | Adds | Checkpoint | Viva change | Ref |
|---:|---|---|---|---|
| 61 | The full window-title HUD: shading mode, environment, speed, heading, target, aim mode, azimuth, elevation, muzzle speed, reload, flight time, last result, ship and crew counts, draw calls, and FPS | A grader can read every piece of state without opening the code | Add or remove one displayed field | B21 |
| 62 * | Measurement pass: unique meshes and their reuse, idle and effect draw/triangle counts, uniforms hoisted out of the per-object loop, no allocation in the loop, and measured FPS | The report's optimization section contains real measured numbers, not the old 20-draw ceiling | Explain one measurement | B22 |

---

## Stage M - Environment modes (Phases 63-65)

These stay **last**, exactly as the teacher required, and are never started
while an earlier checkpoint fails.

| Phase | Adds | Checkpoint | Viva change | Ref |
|---:|---|---|---|---|
| 63 * | `enum class EnvironmentMode`, one settings function supplying clear colour, global ambient, and sun direction/colour/intensity; `4` sun and `5` moonlight | Two clearly different times of day; ship, cannon, and crew keep working in both | Change the moonlight colour | B20 |
| 64 | Rain mode on `6`: a darker palette plus a fixed downward rain pool | Rain falls and the scene is visibly overcast | Change the rain speed or count | B20 |
| 65 | Winter mode on `7`: a cold palette plus a slow drifting snow pool | Snow drifts and the palette is cold | Change the snow drift | B20 |

One shader system receives different mode values. Four shader programs would be
a scope violation.

---

## Stage N - Submission (Phase 66)

| Phase | Adds | Checkpoint | Ref |
|---:|---|---|---|
| 66 | Report and viva rehearsal: traceability matrix, illumination equation and material table with slide citations, the Flat/Gouraud/Phong and ocean-highlight evidence, the player-root transform explanation, fleet and ship hierarchy diagrams, a crew screenshot naming every role, two trajectories, one screenshot per environment mode, final measurements, and the local-versus-global illumination paragraph | The 3-minute demonstration can be performed twice without touching code | B23 |

---

## If the deadline gets close

Merge in this order, and never merge a `*` phase:

1. The `(o)` phases: 8 into 7, 19 into 18, 21 into 20, 24 into 25, 39 into 38,
   54 into 53.
2. Stage A phases 4, 5, and 6 into one model-matrix phase.
3. Stage B phases 20 and 22 into one curved-surface phase.
4. Stage K helpers (60) into the cannon crew phase (59).
5. Stage M rain and winter (64, 65) into one weather phase.

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
