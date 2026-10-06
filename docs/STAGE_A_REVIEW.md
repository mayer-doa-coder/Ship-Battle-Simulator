# Stage A Review - From Flat Triangle to 3D Space (Phases 3-13)

Independent verification of everything built up to and including Phase 13
(Phases 0-2 are covered as the foundation Stage A stands on).

| | |
|---|---|
| Reviewed | 30 September 2026 |
| Repository state | `Ship-Battle-Simulator` at `b971678` (`feat: create first orbit camera and mouse based setup`), working tree clean |
| Scope | `src/main.cpp`, `src/Camera.h`, `src/Shader.h`, `shaders/basic.vert`, `shaders/basic.frag`, `CMakeLists.txt`, `README.md`, `AGENT.md`, `docs/PHASE_PLAN.md`, `docs/PHASE_0` to `PHASE_13_EXPLANATION.md` |
| Source files changed by this review | **None.** All builds and test programs were made outside the project folder |

---

## 1. Verdict

**Stage A is complete and sound, with one real regression that should be fixed
before Phase 14.**

| Question | Answer | Evidence level |
|---|---|---|
| Does it build in Debug and Release with no compiler warnings? | **Yes** - also clean at `/W4 /permissive-` | Executed |
| Does it start, create an OpenGL 3.3 Core context, link its shader, and run without any GL or uniform warning? | **Yes** | Executed |
| Is the cube geometry correct (24 vertices, 36 indices, every face wound outward, one flat colour per face)? | **Yes**, all 12 triangles | Computed |
| Is the quad geometry correct and correctly wound? | **Yes**, both triangles | Computed |
| Is the orbit-camera maths correct, and does the 89 degree clamp really prevent the flip? | **Yes** | Computed |
| Does every phase's checkpoint in `PHASE_PLAN.md` hold? | **10 of 11 fully. Phase 6 only partly** - see finding F1 (fixed in Phase 19) | Code + build + run |
| Do the docs match the code? | **Mostly.** Nine drift items, all cosmetic; listed in F3 | Read |
| Is the "no Python, no pre-computed animation, no engine" rule respected? | **Yes** | Read + search |

The single defect that matters:

> **F1 - Phase 6's pulse range is wrong.** `src/main.cpp:541` multiplies by
> `0.1f` where it should multiply by `0.5f`. The triangle was meant to pulse
> between `0.5x` and `1.3x`; it actually pulses between `-0.22x` and `+0.58x`.
> It was introduced in the Phase 12+13 commit and is not mentioned in any
> document. One-character fix; details in section 5.

Recommended next action: fix F1, rebuild, watch the triangle for one pulse
period (about 5 seconds), then start Phase 14. Nothing else blocks it.

---

## 2. How this was verified, and what was not

| # | Check | How | Result |
|---|---|---|---|
| 1 | Read every Stage A file end to end | Full read of all source, shaders, CMake, README, AGENT, PHASE_PLAN, and Phase 0-13 documents | Done |
| 2 | Git history | Traced every constant and suspicious line through all 12 commits | Done |
| 3 | Configure + build Debug and Release | Fresh out-of-tree CMake build, MSVC 19.44, VS 2022 Build Tools, x64 | 0 compiler warnings. Only `LNK4098` (Debug only, already documented in Phase 0) |
| 4 | Strict warnings | Second fresh build with `/W4 /permissive- /EHsc` | 0 warnings in project source |
| 5 | Live run | Launched the Release exe for 7 seconds, captured stdout and stderr | See below |
| 6 | Cube and quad geometry | A small C++ program compiled against the **actual arrays extracted from `main.cpp`** (not retyped) | See section 4.10 |
| 7 | Camera maths | Same program: default position, 89 and 90 degree pitch | See section 4.13 |
| 8 | Pulse maths | Same program | Confirms F1 numerically |

Live run output (Release, Intel UHD 770):

```text
OpenGL   : 3.3.0 - Build 32.0.101.7088
GLSL     : 3.30 - Build 32.0.101.7088
[shader] linked program 3 (shaders/basic.vert + shaders/basic.frag)
Phase 13 ready. Drag with the left mouse button to orbit, ...
[frame] time= 1.32 s ... fps= 108.7     (six report lines, one per second)
stderr: empty
```

An empty stderr means no `uniform not found` warning, no `OpenGL setup error`,
and no shader failure. The exit code `124` in the raw log is only my `timeout`
command ending the run.

**What I did not do, so it is not claimed:**

- I did not drive the mouse or keyboard, and I did not inspect rendered
  pixels. The pixel-measured results in the Phase 4-13 documents (for example
  "5.2 px error") were **not reproduced**. What I checked is that the code
  implements what those documents say it does.
- The Linux branch of `CMakeLists.txt` was not built (Windows machine).
- The visual side of F1 was established by arithmetic, not by watching the
  screen.

---

## 3. What Stage A is, as one picture

### 3.1 The frame

```text
main()  -  one-time setup                       per-frame loop (main.cpp:1098)
-------------------------                       ---------------------------------
glfwInit, hints (GL 3.3 core)                   updateClock        now, dt (clamped to 0.10 s)
glfwCreateWindow                                processInput       ESC, D, W, O  (edge-detected)
gladLoadGLLoader                                glfwGetFramebufferSize
load shader  (Shader.h)                         updateScene        build every matrix from `now`
create triangle / quad / cube (VAO+VBO[+EBO])   renderScene        set state, upload uniforms, draw x4
register camera + mouse callbacks               glfwSwapBuffers
                                                glfwPollEvents     <- fires the mouse callbacks
                                                reportFrame        once per second
shutdown: destroy cube, quad, triangle, shader, window, GLFW
```

Update calculates, render displays: `renderScene()` only ever *reads*
`SceneState`. That separation (Phase 1) is respected everywhere.

### 3.2 The vertex path

```text
gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0)      basic.vert:43
                   |            |        |
                   |            |        +-- Phases 4-6, 10: place / turn / resize the object
                   |            +----------- Phase 12-13: orbit camera via glm::lookAt
                   +------------------------ Phase 7: glm::perspective(45 deg, aspect, 0.1, 100)
FragColor = vec4(vColor * uTint, 1.0)                                  basic.frag:21
```

One shader program, four uniforms (`uModel`, `uView`, `uProjection`,
`uTint`), two attributes (position `location 0`, colour `location 1`), stride
6 floats. Every object in Stage A uses exactly this.

### 3.3 What is on screen

| Object | Mesh data | Draw call | Model matrix | Motion |
|---|---|---|---|---|
| Near triangle | 3 vertices | `glDrawArrays` | `slide * spin * scaleMat` | slides (Y), spins (Z), pulses, swings in depth |
| Far triangle | **same VAO** | `glDrawArrays` | `translate(0,0,-1.2) * triangleModel` | follows the near one |
| Quad | 4 vertices + 6 indices | `glDrawElements` | `translate(-1.8, 0, 0)` | none |
| Cube | 24 vertices + 36 indices | `glDrawElements` | `translate(1.8,0,0) * rotate(0.6t, tilted axis)` | spins on a tilted axis |

Totals: **4 draw calls, 3 VAOs, 1 shader program, 31 vertices uploaded
(3 + 4 + 24), 42 indices (6 + 36)**. The two triangles share one VAO, which is
why there are 4 draws but only 3 meshes. This is the whole of Stage A.

### 3.4 GL state, and who owns it

| State | Set where | Controlled by |
|---|---|---|
| `GL_DEPTH_TEST` | `renderScene()` every frame, `main.cpp:873-876` | `D` key |
| `GL_CULL_FACE` | `renderScene()` every frame, `main.cpp:884-890` | `W` key (off in wireframe) |
| `glPolygonMode` | same block | `W` key |
| Viewport | framebuffer callback + once at startup | window resize |
| Clear colour | `renderScene()` every frame | `AppConfig::CLEAR_COLOR` |

Setting these every frame from a `SceneState` flag, instead of once at
startup, is the right design: the picture always matches the flag.

---

## 4. Phase-by-phase audit

Legend: **PASS** = checkpoint holds. **PASS (drift)** = holds, but a document
disagrees with the code. **PARTIAL** = holds only in part.

### 4.0 Summary table

| Phase | Concept | Code | Status | Viva change and where |
|---:|---|---|---|---|
| 0 | Window, GLAD, clear, clean exit | `main.cpp:966-1018` | PASS | `CLEAR_COLOR` `:49` |
| 1 | Loop split, `dt` clamp, FPS report | `:490-504`, `:941-962` | PASS | `REPORT_INTERVAL` `:43`, `MAX_DELTA_TIME` `:40` |
| 2 | Checked shader loader, VAO/VBO, triangle | `Shader.h`, `:623-682` | PASS | corner rows `:86-90` |
| 3 | Uniform setters, `uTint`, warn-once | `Shader.h:109-131,164-184` | PASS (drift) | `TINT` `:58` |
| 4 | `uModel` from `sinf(now)` | `:516-528` | PASS (drift) | `SLIDE_DISTANCE` `:106` |
| 5 | `glm::rotate` from `now` | `:532-534` | PASS (drift) | `SPIN_SPEED` `:123`, `SPIN_AXIS` `:124` |
| 6 | `glm::scale`, `T*R*S`, `O` key | `:540-568`, `:478-487` | **PARTIAL (F1)** | swap the product at `:566-568`, or press `O` |
| 7 * | `lookAt` + `perspective` together | `:601-616`, `basic.vert:43` | PASS | `FIELD_OF_VIEW_DEGREES` `:75`, `NEAR_PLANE` `:76`, `FAR_PLANE` `:77` |
| 8 | Same VAO twice, `D` depth key | `:576-578`, `:873-907` | PASS (indirectly affected by F1) | swap the two draw blocks `:895-907` |
| 9 | EBO, `glDrawElements`, quad | `:185-223`, `:684-763` | PASS (drift) | one number in `QuadConfig::INDICES` `:206-209` |
| 10 * | Cube: 24 vertices, 36 indices | `:225-315`, `:767-839` | PASS | `HALF_SIZE` `:232`, a face's colour rows `:245-278` |
| 11 | Winding + `GL_CULL_FACE` + `W` wireframe | `:884-890`, `:462-471` | PASS | swap two numbers inside one face group `:286-291` |
| 12 | `OrbitCamera` (radius, yaw, pitch) | `Camera.h:20-34,74-80` | PASS | see note 4.12 |
| 13 * | Cursor + scroll callbacks, clamps | `Camera.h:91-134`, `main.cpp:1087-1090` | PASS | `MIN_RADIUS` `Camera.h:43`, `MAX_RADIUS` `:44` |

`*` = graded milestone in the plan.

### 4.1 Phase 0 - OpenGL window

- Context hints request 3.3 Core (`:974-976`); the Apple-only forward-compat
  hint is correctly guarded (`:978-980`).
- Order is correct: make context current (`:996`) **then** `gladLoadGLLoader`
  (`:998`), with a hard failure if it returns 0. This is one of the six known
  project bugs, and it is handled.
- Runtime confirmed `3.3.0` / GLSL `3.30`.

### 4.2 Phase 1 - render loop

- `dt = min(now - last, 0.10)` (`:497-504`); `now` itself is never clamped,
  which is exactly what the document says.
- The report fires when at least `REPORT_INTERVAL` has elapsed; six lines in
  six seconds observed, one per second.
- `startClock()` seeds `lastFrameTime = now`, so the first `dt` is 0, not the
  whole startup time.

### 4.3 Phase 2 - shader loader and first triangle

- Every failure path in `Shader.h` prints and returns false: file open, empty
  file, compile (with driver log), link (with driver log). Shader objects are
  deleted after linking. This meets the project rule that a shader must never
  fail silently.
- VAO records the layout, VBO holds 18 floats, stride is
  `6 * sizeof(float)`, colour offset is `3 * sizeof(float)`.
- Cleanup order in `main()` is correct: GPU objects, then `shader.destroy()`,
  then the window, then `glfwTerminate()`. `ShaderProgram`'s destructor runs
  after `glfwTerminate()` but is a no-op because `destroy()` already zeroed
  `m_id`.

### 4.4 Phase 3 - uniforms

- `use()` is called before any setter (`main.cpp:862`) - the rule the phase
  teaches.
- `uniformLocation()` warns **once** per missing name via
  `m_missingUniforms` (`Shader.h:164-184`); the run produced no warning
  because all four uniforms are genuinely used.
- Observation (not a defect): `glGetUniformLocation` is called on every setter
  call, 10 lookups per frame (2 camera uniforms + `uTint` and `uModel` for each
  of 4 draws). Irrelevant now; Phase 103's "uniforms
  hoisted out of the per-object loop" will want cached locations.

### 4.5 Phase 4 - the model matrix

- `offsetX = SLIDE_DISTANCE * sin(SLIDE_SPEED * now)` - a closed form of the
  clock; nothing stored. This satisfies the no-pre-computed-animation rule.
- **Drift:** the value named `offsetX` is applied to the **Y** component
  (`:528`), so the triangle slides up and down, not left and right. See F2.

### 4.6 Phase 5 - rotation

- `glm::rotate(I, SPIN_SPEED * now, SPIN_AXIS)` about Z (`:532-534`), joined as
  `slide * spin`. Radians throughout. Correct.

### 4.7 Phase 6 - scale, and `T * R * S` - **PARTIAL**

- The order lesson is intact: `slide * spin * scaleMat` (correct) versus
  `scaleMat * spin * slide` (the `O` key) at `:566-568`. Edge-detection on `O`
  is correct (`:478-487`).
- The scale factor itself is wrong. See **F1** below. The transform *order*
  demonstration still works; the *pulse range* does not match the document,
  the code's own comment, or the plan.

### 4.8 Phase 7 (milestone) - view and projection together

- Chain `uProjection * uView * uModel * vec4(p, 1.0)` at `basic.vert:43` is in
  the right order, `w = 1` for the position.
- Aspect ratio is rebuilt every frame from the live framebuffer size, with a
  `max(height, 1)` guard against a minimised window (`:609-611`). Correct.
- `near = 0.1`, `far = 100`, `fov = 45 deg` are named in `CameraConfig`.

### 4.9 Phase 8 - depth testing

- The far copy is `translate(0,0,-1.2) * triangleModel` (`:576-578`): built
  from the finished near matrix so the two can never drift apart. The far copy
  is drawn **second** (`:905-907`), which is what makes switching depth off
  visibly wrong. Correct and deliberate.
- The `D` toggle is edge-detected (`:447-456`); state is applied every frame.
- Indirectly affected by F1: while the near triangle is tiny or flipped, the
  overlap that the demonstration relies on shrinks. The demo still works, but
  less convincingly for roughly a fifth of every pulse period.

### 4.10 Phases 9-10 - indexed drawing and the cube

Checked by compiling the real arrays out of `main.cpp`:

| Check | Result |
|---|---|
| Cube vertex count vs `VERTEX_COUNT` | 24 = 24 |
| Cube index count vs `INDEX_COUNT` | 36 = 36 |
| Every index below 24; every vertex referenced | Yes; yes |
| Each of the 12 triangles: geometric normal points **away from the cube centre** | 12 of 12 |
| Each face's normal equals its intended axis (`+Z, -Z, +X, -X, +Y, -Y`) | 12 of 12 |
| Every triangle has one flat colour; 6 faces have 6 distinct colours | Yes |
| Every triangle has area `0.5` (unit-sided faces) | Yes |
| Quad: both triangles counter-clockwise seen from `+Z` (`normal.z = +0.64` each) | Yes |
| Quad: 4 vertices, 6 indices, corners 0 and 2 each used twice | Yes |

So the winding claim in the Phase 10 document ("checked for all six faces
before typing") is **true**, and `GL_CULL_FACE` cannot be silently removing any
face of the shipped cube.

- The `EBO` is deliberately **not** unbound before the VAO in `createQuad` and
  `createCube` (`:732-738`, `:811-814`). Correct; the VAO keeps the binding.
- The spin axis `normalize(0.4, 1, 0.3)` has components on all three axes, so
  every face reaches the camera over time - the stated purpose.

### 4.11 Phase 11 - winding and wireframe

- One flag (`wireframeEnabled`) drives **two** pieces of state together:
  `glDisable(GL_CULL_FACE)` + `glPolygonMode(GL_LINE)`, or the reverse
  (`:884-890`). The explanation of *why both* (culling happens before
  rasterisation) is accurate, and is the reason the phase exists.
- `glPolygonMode(GL_FRONT_AND_BACK, ...)` is the only form legal in a Core
  profile, and that is the form used.

### 4.12 Phase 12 - the orbit camera

- Position formula in `Camera.h:74-80` matches the document exactly:
  `x = r cos(p) sin(y)`, `y = r sin(p)`, `z = r cos(p) cos(y)`. Default
  `(0,0,4)` confirmed by computation - the same spot as Phase 7's fixed `EYE`.
- Note on reproducibility: **Phase 12 has no commit of its own.** `Camera.h`
  first appears in `b971678`, which is the Phase 12 **and** Phase 13 commit.
  The arrow-key version (`updateOrbitCameraFromKeys`, `ORBIT_SPEED`) described
  in `PHASE_12_EXPLANATION.md` therefore no longer exists anywhere in the code
  or history, and cannot be checked out to demonstrate the flip. The Phase 12
  viva change in the plan ("change the orbit speed") has no code target any
  more; the nearest equivalent is `MOUSE_SENSITIVITY` (`Camera.h:40`).

### 4.13 Phase 13 (milestone) - mouse orbit and limits

| Behaviour | Code | Verified by |
|---|---|---|
| Camera reachable from a C callback | `glfwSetWindowUserPointer(window, &scene.camera)` `main.cpp:1087` | Read |
| Cursor seeded from the real position | `glfwGetCursorPos(...)` `:1088` | Read |
| Tracks cursor even when not dragging | updates before the early return `Camera.h:98-101,108` | Read |
| Orbit only while left button held | `Camera.h:108` | Read |
| Y drag inverted (screen Y points down) | `pitch -= dy * ...` `Camera.h:115` | Read |
| Pitch clamped to +/-89 degrees | `Camera.h:117-118` | **Computed** |
| Zoom by subtracting scroll from radius | `Camera.h:131` | Read |
| Radius clamped to `[1.5, 15.0]` | `Camera.h:132-133` | Read |

Computed results for the clamp:

```text
pitch 89 deg:  eye = (0.0000, 3.9994, 0.0698)   lookAt basis finite, valid
pitch 90 deg:  eye = (-0.0000, 4.0000, -0.0000) view direction parallel to "up";
               the right-vector came out as (-1, 0, 0), i.e. a mirrored basis
```

At exactly 90 degrees, only floating-point noise stops `lookAt` from dividing
by zero, and the result it does give is the flipped one. That is a real,
numeric reason for the 89 degree limit - a stronger answer than "the maths
gets unstable".

Arithmetic cross-checks of the Phase 13 document's own numbers, all correct:
300 px x 0.005 = 1.5 rad = 86 degrees; 800 px x 0.005 = 4 rad = 229 degrees;
5 scroll ticks x 0.5 = 2.5, and `4.0 - 2.5 = 1.5`, exactly `MIN_RADIUS`;
`(15 - 4) / 0.5 = 22` ticks to reach `MAX_RADIUS`.

---

## 5. Findings

| ID | Severity | Summary |
|---|---|---|
| F1 | ~~Medium~~ **FIXED in Phase 19** | Phase 6 pulse maths regressed: `* 0.1f` instead of `* 0.5f` |
| F2 | Low | The slide is vertical, but the plan, Phase 4 document and variable name say horizontal |
| F3 | Low | Nine places where a document disagrees with tuned code |
| F4 | Low | Four stale code comments |
| F5 | Low | README and plan inconsistencies |
| F6 | Info | Three commits each cover two phases |
| F7 | Info | Forward-looking risks for later stages |

### F1 - Phase 6 pulse range is wrong (Medium) - FIXED in Phase 19

> **Resolved on 4 October 2026.** The midpoint factor was changed from `0.1f` to
> `0.5f`. Measured afterwards on 20 instants across a full pulse period, the
> scale factor now runs from exactly `0.5000` to exactly `1.3000`, and the
> triangle's smallest on-screen size went from 66 pixels (a dot) to 5 943. The
> full before-and-after measurement is in
> [PHASE_19_EXPLANATION.md](PHASE_19_EXPLANATION.md). The rest of this section
> describes the defect as it was found, and is kept as the record.

**Where:** [src/main.cpp:540-542](../src/main.cpp#L540-L542)

```cpp
const float scalePulseMid =
    (TriangleScale::PULSE_MIN + TriangleScale::PULSE_MAX) * 0.1f;   // should be 0.5f
```

**Cause:** Introduced in commit `b971678` (the Phase 12+13 commit). The diff
shows `0.5f` changed to `0.1f`; it is unrelated to anything Phase 12 or 13
does, and no document mentions it. It looks like an accidental edit, but confirm
it was not a deliberate experiment before reverting.

**Effect, computed:**

| | Intended (docs, code comment, plan) | Actual |
|---|---|---|
| Midpoint | 0.9 | 0.18 |
| Amplitude | 0.4 | 0.4 |
| Scale range | **0.5 to 1.3** | **-0.22 to +0.58** |
| Largest size | 1.3x | 0.58x (about 45% of intended) |
| Time with a negative factor | never | about 35% of every 5.24 s period |
| Time smaller than 0.1x (almost invisible) | never | about 18% |

What you see: the triangle is less than half the intended maximum size, shrinks
to a point twice per period, and re-emerges turned 180 degrees in the plane.
It does **not** vanish through culling: the matrix is `scale(s, s, 1)`, whose
determinant `s*s` is always positive, so winding is preserved.

The code's own comment above it says the formula "keeps the factor inside
[PULSE_MIN, PULSE_MAX] without a clamp", and the Phase 6 document states the
worked example `mid = 0.9`. Both are now false.

**Impact on other phases:** the Phase 6 `O` demonstration still shows the order
effect but from a much smaller triangle; the Phase 8 depth demo loses part of
its overlap while the triangle is tiny.

**Fix (recommended):** change `0.1f` to `0.5f` on line 541. Then run once and
watch the triangle for a full pulse: it should swell to a comfortable size,
never shrink to a point, and never flip.

### F2 - Slide direction (Low)

`updateScene()` builds `glm::vec3(0.0f, offsetX, offsetZ)` (`main.cpp:528`), so
the value named `offsetX` moves the triangle along **Y**. The plan's Phase 4
checkpoint ("slides left and right") and the Phase 4 document (which even gives
"change `(offsetX,0,0)` to `(0,offsetX,0)`" as a viva exercise) describe the
opposite. `PHASE_5_EXPLANATION.md` does acknowledge "moves the triangle up and
down". This has been true since the Phase 4/5 commit (`509e40b`).
**Recommendation:** keep the behaviour, rename `offsetX` to `offsetY` when
convenient, and correct the Phase 4 text. Until then, be ready to say which axis
it really moves on.

### F3 - Documentation drift (Low)

The documents describe the values as first written; the code has since been
tuned. None of these break a checkpoint.

| Item | Document says | Code has | Document |
|---|---|---|---|
| `SLIDE_SPEED` | `1.5` | `1.0` (`:107`) | Phase 4 |
| Slide direction | left/right | up/down (F2) | Phase 4, plan |
| `SPIN_SPEED` | `2.0` | `0.5` (`:123`) | Phase 5 |
| Triangle corner Y values | `0.5, 0.5, -0.6` (average +0.133) | `0.37, 0.37, -0.73` (average +0.003) | Phase 5 |
| `TINT` | `(3.0, 2.4, 0.6)` | `(0.6, 2.4, 3.0)` - the "cool" variant Phase 3 lists as a viva exercise | Phase 3 |
| Quad corner colours | pure red / green / blue / yellow | tuned pastels (`:194-197`) | Phase 9 |
| Pulse midpoint | `0.9` | `0.18` (F1) | Phase 6 |
| Window colour | "dark-blue window" | golden `(0.82, 0.66, 0.04)` - changed in Phase 1 | Phase 0 |
| Renderer | "NVIDIA GeForce RTX 5050" | Intel UHD 770 reported in this run (see F7) | Phase 0 |

The Phase 5 row has a side effect worth knowing: with the corners now averaging
`y ~ 0`, the triangle spins almost exactly on its middle, so the wobble that
Phase 5 uses to teach "rotation happens about the origin" is essentially gone
from the default view. The lesson is still shown by the `O` key (Phase 6) and
by the viva exercise of moving the slide first.

### F4 - Stale comments in code (Low)

| Location | Comment says | Reality |
|---|---|---|
| `main.cpp:119-122` | corners average `y = +0.13`, so the triangle wobbles | average is `+0.003` (F3) |
| `main.cpp:70-74` | depth range measured "from EYE" | `EYE` was removed in Phase 12 |
| `main.cpp:372-375` | view/projection "do not depend on `now` yet, because the camera itself does not move until Phase 12" | the view now depends on the orbit camera |
| `main.cpp:541` (comment above it) | factor stays within `[PULSE_MIN, PULSE_MAX]` | false until F1 is fixed |

### F5 - README and plan inconsistencies (Low)

- `README.md:74-75` lists the **Stage B row twice** (identical lines).
- `PHASE_PLAN.md` "Completed" table stars Phases 10 and 13 but not Phase 7,
  although Phase 7 is starred in the Stage A table.
- `PHASE_PLAN.md:61` says "Stage A and Phases 3 to 13 are now done" - Stage A
  *is* Phases 3-13, so this reads as two things. Harmless.
- `src/output/` is an empty, untracked directory. Not referenced by anything;
  safe to delete.

### F6 - Commit granularity (Info)

Twelve commits cover fourteen phases:

| Commit | Phase(s) |
|---|---|
| `84d7ae9` | 0 |
| `a55422f` | 1 |
| `1c465a4` | 2 (also creates `PHASE_PLAN.md`) |
| `2c52003` | 3 |
| `509e40b` | **4 and 5** |
| `b8a01a4` | 6 |
| `b59329d` | 7 |
| `2267ec7` | **8 and 9** |
| `093ce91` | 10 |
| `7b30762` | 11 |
| `b971678` | **12 and 13** |

Consequence: you cannot check out the standalone state of Phases 4, 5, 8, 9,
12, or 13, and F1 could only be traced to a two-phase commit. It does not affect
correctness. For Stage B, one commit per phase would make each checkpoint
individually reproducible, which is what the plan's "one concept, one
checkpoint" rule is aiming at. Note that Phase 13 is a milestone (`*`) that was
committed together with its neighbour.

### F7 - Forward-looking risks (Info)

These are not Stage A defects. They are things Stage A leaves for later phases
to trip over.

1. **Key conflicts in Phase 79.** Stage E gives `W/A/S/D` to the ship. Stage A
   already uses `W` (wireframe, Phase 11) and `D` (depth test, Phase 8). The
   plan moves camera zoom to `Q/E` but does not say where the wireframe and
   depth toggles go. Decide before Phase 79; the debug keys are worth keeping.
2. **Phase 19 repeats Phase 11.** Plan row 19 ("Wireframe toggle with
   `glPolygonMode`") is already implemented and working since Phase 11. Phase
   19's real new content is comparing `N = 4` against `N = 32` in wireframe;
   it needs no new toggle.
3. **The zoom clamp protects the origin, not off-origin objects.**
   `MIN_RADIUS = 1.5` keeps the camera off the *target*, but the cube sits at
   `(1.8, 0, 0)` with half-size `0.5`. At yaw 90 degrees, pitch 0, radius 1.5 the
   eye is at `(1.5, 0, 0)` - **inside the cube**, and back-face culling then
   hides its inside faces. The moving triangle can likewise swing through the
   camera when zoomed in (`z` reaches `+1.5`). The Phase 13 document's claim that
   the limit stops the camera "clipping through the middle of an object" is only
   true for an object at the origin. Not worth changing: the Stage A scene is
   temporary, and Stage B objects sit at the origin. If asked in a viva, say so.
4. **GPU selection on a hybrid laptop.** Phase 0 recorded an NVIDIA RTX 5050;
   this run was given an Intel UHD 770 and reported 82-109 FPS under V-sync.
   Windows chose the GPU. Before the high-tessellation demonstrations (Stage C,
   Phase 33) and the Phase 103 measurements, pin the executable to the NVIDIA
   GPU in Windows Graphics settings, otherwise FPS comparisons between Flat,
   Gouraud, and Phong will measure the wrong chip.
5. **Mouse sensitivity is per screen coordinate,** not per pixel, so it can
   feel different on a scaled (high-DPI) display. Cosmetic.
6. **Three near-identical `create*/destroy*` pairs and three GPU-handle
   structs** (triangle, quad, cube). This is intentional and is exactly what
   Phase 14's `Mesh` replaces; the comment at `main.cpp:348-352` says so.

---

## 6. Controls and interaction state

| Input | Action | Mechanism | Phase |
|---|---|---|---|
| `ESC` | Close | polled each frame | 0 |
| `O` | `T*R*S` <-> `S*R*T` | polled + edge-detected | 6 |
| `D` | Depth test on/off | polled + edge-detected | 8 |
| `W` | Wireframe + culling off/on | polled + edge-detected | 11 |
| Left-drag | Orbit (yaw, pitch) | GLFW cursor callback | 13 |
| Scroll | Zoom (radius) | GLFW scroll callback | 13 |

Edge detection (`...WasDown` flags) makes each toggle fire once per physical
press instead of about 120 times a second. The three toggles are independent and
can be combined freely.

Deliberate design choice worth quoting in a viva: toggles are *polled and
edge-detected*, camera movement is *event-driven*, because the two are
different kinds of input.

---

## 7. Resource lifecycle

Creation order in `main()`: context -> GLAD -> shader -> triangle -> quad ->
cube -> callbacks -> loop.

Every creation failure path unwinds what was already created, in reverse, then
destroys the window and terminates GLFW (`main.cpp:1032-1071`). `createTriangle`,
`createQuad`, and `createCube` each check `glGetError()` after setup and return
false on error. Normal exit destroys cube, quad, triangle, shader, window, GLFW
(`:1116-1123`) - reverse of creation, with the GL context still alive. No leak
or double-delete path found.

---

## 8. Stage A viva pack

### 8.1 Likely questions

| Question | Short answer |
|---|---|
| What does the vertex shader do? | Multiplies each vertex by `uProjection * uView * uModel`, with `w = 1` because it is a position. |
| Attribute versus uniform? | Attribute: per vertex, stored in the VBO. Uniform: one value for the whole draw call, set from C++ every frame. |
| Why must `use()` come before `setMat4`? | Uniforms are written into the currently bound program; with none bound the write is silently lost. |
| Why is the matrix on the left of the vector? | GLSL/OpenGL column-vector convention: the matrix acts on what is to its right, so products read right to left. |
| Why `T * R * S`? | Scale and rotate act about the origin, so they must happen while the object is still there; translate last. Reversed, the move gets scaled and turned. |
| Why can't view or projection be introduced alone? | View alone can push everything outside the clip cube; projection alone assumes a camera at the origin and can divide by a near-zero `w`. |
| Why is aspect ratio recomputed every frame? | The window can be resized any time; a stale aspect stretches the picture. |
| What is an EBO for? | It lists which uploaded vertex to use next, so a shared corner is stored once. |
| Why does the cube need 24 vertices, not 8? | A vertex carries one colour, and each corner belongs to three faces that each need their own colour. |
| What is winding order? | The order a triangle's corners are listed, seen from one side; counter-clockwise is front. |
| Why doesn't wireframe alone reveal a culled face? | Culling discards the triangle before rasterisation, so line mode never sees it; culling must be disabled too. |
| Why does `D` not break the cube? | A single convex closed shape has nothing behind itself once back faces are culled. Depth testing only matters between separate overlapping objects. |
| Why clamp pitch to 89, not 90? | At 90 degrees the view direction is parallel to "up"; `lookAt` becomes degenerate and produces the flipped basis. |
| Why a window user pointer? | GLFW callbacks are plain C function pointers and cannot capture the camera; the pointer is the sticky note that lets them reach it. |
| Is the animation pre-computed? | No. Slide, spin, pulse, depth swing, and cube spin are all closed forms of `glfwGetTime()`; the only stored state is the camera and the key flags, which are input state. |

### 8.2 A 90-second Stage A demonstration

| Time | Action | What to say / what to see |
|---|---|---|
| 0:00 | Start | "Everything moving is a function of the clock - nothing is pre-recorded." Triangle slides, spins, pulses, swings toward and away; cube spins. |
| 0:15 | Press `O`, then `O` again | "Same three matrices, wrong order. The move gets scaled and turned - it smears." |
| 0:30 | Press `D`, then `D` | "Depth test off: the far triangle, drawn last, wins. On: the nearer one wins. Draw order stops mattering." |
| 0:45 | Press `W` | "Wireframe **and** culling off, so hidden faces show. All 12 cube triangles, front and back." Press `W` again. |
| 1:00 | Left-drag around the cube, scroll in and out | "Orbit camera: one distance and two angles. Zoom is clamped." |
| 1:15 | Drag to the extreme top | "Pitch stops at 89 degrees. No flip." |

Do the demonstration **after** F1 is fixed; with the current code the pulse
looks visibly wrong during the first step.

---

## 9. Before starting Phase 14

Do these in order:

1. ~~**Fix F1.**~~ **Done in Phase 19.** In [src/main.cpp:541](../src/main.cpp#L541) change `* 0.1f` to
   `* 0.5f`. Build with `F7`, run with `Shift+F5`, and watch one full pulse.
2. Optionally reconcile the drift in F3/F4 (documents and four comments). None of
   it blocks Phase 14.
3. Delete the duplicate Stage B row at `README.md:75`.
4. Commit as its own change, so the fix is separable from Phase 14's work.
5. Write down now where `W` and `D` go in Phase 79 (F7.1).

Manual checks I could not perform, worth doing yourself once:

- [ ] Left-drag orbits smoothly in both directions; no jump on the first drag.
- [ ] Scroll all the way in and all the way out; both stops hold.
- [ ] Drag to the top and bottom; no flip.
- [ ] `D`, `W`, `O` each toggle exactly once per press, from an orbited view.
- [ ] Resize the window very wide and very tall; the cube stays a cube.
- [ ] Minimise and restore; no crash, no stretched picture.
- [ ] After F1: the triangle pulses within `0.5x-1.3x` and never flips.

---

## Appendix A - Tunable values in Stage A

All in [src/main.cpp](../src/main.cpp) unless noted. These are the values a
teacher is most likely to ask you to change.

| Value | Current | Line | Effect |
|---|---:|---:|---|
| `WINDOW_WIDTH` / `WINDOW_HEIGHT` | 1280 / 720 | 32-33 | Start size |
| `MAX_DELTA_TIME` | 0.10 s | 40 | Largest `dt` an update may see |
| `REPORT_INTERVAL` | 1.0 s | 43 | Console FPS report rate |
| `VSYNC_INTERVAL` | 1 | 46 | 0 = uncapped |
| `CLEAR_COLOR` | (0.82, 0.66, 0.04) | 49 | Background |
| `TINT` | (0.6, 2.4, 3.0) | 58 | Near triangle's colour filter |
| `FIELD_OF_VIEW_DEGREES` | 45 | 75 | Lens width |
| `NEAR_PLANE` / `FAR_PLANE` | 0.1 / 100 | 76-77 | Clip range |
| `SLIDE_DISTANCE` / `SLIDE_SPEED` | 0.5 / 1.0 | 106-107 | Slide range, rad/s |
| `SPIN_SPEED` / `SPIN_AXIS` | 0.5 / Z | 123-124 | Triangle spin |
| `PULSE_MIN` / `PULSE_MAX` / `PULSE_SPEED` | 0.5 / 1.3 / 1.2 | 136-138 | Pulse (see F1) |
| `DEPTH_AMPLITUDE` / `DEPTH_SPEED` | 1.5 / 0.8 | 160-161 | Depth swing |
| `FAR_COPY_Z_OFFSET` / `FAR_COPY_TINT` | -1.2 / (3, 0.5, 0.4) | 175, 181 | Far triangle |
| `QuadConfig::POSITION` | (-1.8, 0, 0) | 221 | Quad location |
| `CubeConfig::HALF_SIZE` | 0.5 | 232 | Cube size |
| `CubeConfig::POSITION` | (1.8, 0, 0) | 302 | Cube location |
| `CubeConfig::SPIN_SPEED` | 0.6 | 308 | Cube spin rate |
| `MOUSE_SENSITIVITY` | 0.005 rad/px | Camera.h:40 | Orbit speed |
| `ZOOM_SPEED` | 0.5 units/step | Camera.h:42 | Zoom step |
| `MIN_RADIUS` / `MAX_RADIUS` | 1.5 / 15.0 | Camera.h:43-44 | Zoom limits |
| `MAX_PITCH_DEGREES` | 89 | Camera.h:51 | Pitch limit |
| Default `radius` / `yaw` / `pitch` | 4.0 / 0 / 0 | Camera.h:21-23 | Start view |

## Appendix B - Requirement compliance (Stage A)

| Project rule | Status | Note |
|---|---|---|
| C++17, OpenGL 3.3 Core, GLSL 330, GLFW, GLAD, GLM, CMake | Met | `CMAKE_CXX_STANDARD 17`, `#version 330 core`, GLAD generated for `gl=3.3 core` |
| No Python anywhere | Met | No `.py` or requirements files tracked; the word appears only in "no Python" rules |
| No engine / physics / scene graph / model loader / GUI toolkit | Met | Only GLFW, GLAD, GLM |
| One phase at a time, one visual checkpoint | Met in content; see F6 for commit granularity | |
| Named, grouped tuning values | Met | Config namespaces; Appendix A |
| Autonomous motion is a closed form of `glfwGetTime()` | Met | No arrays of positions or keyframes anywhere |
| Parent matrices unscaled; scale only at draw | Not yet applicable (no hierarchy until Stage D) | Stage A's only scale is the triangle's pulse |
| `w = 1` for positions, `w = 0` for directions | Met (positions only so far) | `basic.vert:43` |
| Shader errors always printed | Met | `Shader.h` prints compile and link logs |
| No lighting, meshes, ships yet | Met | `basic.frag` has no lighting; nothing past Phase 13 was added |
| Do not copy the `Broadside` reference | Met | Nothing from `Broadside/src` present |

## Appendix C - Raw verification output

Geometry and maths program (compiled from the arrays in `main.cpp`):

```text
CUBE vertices=24 (declared 24) indices=36 (declared 36)
tri  0 face +Z  outward=1  normal==+Z:1  flatColour=1  area=0.500
tri  1 face +Z  outward=1  normal==+Z:1  flatColour=1  area=0.500
tri  2 face -Z  outward=1  normal==-Z:1  flatColour=1  area=0.500
tri  3 face -Z  outward=1  normal==-Z:1  flatColour=1  area=0.500
tri  4 face +X  outward=1  normal==+X:1  flatColour=1  area=0.500
tri  5 face +X  outward=1  normal==+X:1  flatColour=1  area=0.500
tri  6 face -X  outward=1  normal==-X:1  flatColour=1  area=0.500
tri  7 face -X  outward=1  normal==-X:1  flatColour=1  area=0.500
tri  8 face +Y  outward=1  normal==+Y:1  flatColour=1  area=0.500
tri  9 face +Y  outward=1  normal==+Y:1  flatColour=1  area=0.500
tri 10 face -Y  outward=1  normal==-Y:1  flatColour=1  area=0.500
tri 11 face -Y  outward=1  normal==-Y:1  flatColour=1  area=0.500
QUAD vertices=4 indices=6
quad tri 0 normal.z=+0.640
quad tri 1 normal.z=+0.640
pulse mid factor 0.5 -> scale range [0.500, 1.300]     <- intended
pulse mid factor 0.1 -> scale range [-0.220, 0.580]    <- what the code does
camera default = (0.000, 0.000, 4.000)
view at pitch 89 deg finite=1, eye=(0.0000, 3.9994, 0.0698)
FAILS=0
```

Build: MSVC 19.44.35228.0, x64, Debug and Release, 0 compiler warnings at
default level and at `/W4 /permissive-`. Vendored versions match the README:
GLFW 3.5.1, GLM 1.0.3, GLAD 0.1.36 (`gl=3.3`, core).
