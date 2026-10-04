# Phase 24 - Live Counters in the Window Title

## Status

Phase 24 is complete and verified. Debug and Release builds succeeded with no
compiler warnings, both shaders linked with no missing-uniform warning, and a live
Release run reported the frame's cost in the window title. Nothing was printed to
the error output, and the program closed cleanly.

This is a small phase, marked `(o)` in the plan. Its value is not the feature - it
is that the project can now **check its own arithmetic**.

```text
Ship Battle Simulator - Phase 24 | draws 10 | tris 640 | verts 437
```

### THE CHECKPOINT: does the reported count match a hand calculation?

The phase passes when the number on screen agrees with one worked out on paper. So:

| mesh | verts | tris | times drawn | verts sent | tris sent |
|---|---:|---:|---:|---:|---:|
| triangle | 3 | 1 | 2 | 6 | 2 |
| quad | 4 | 2 | 1 | 4 | 2 |
| grid | 81 | 128 | 1 | 81 | 128 |
| cylinder | 66 | 64 | 1 | 66 | 64 |
| sphere | 200 | 396 | 1 | 200 | 396 |
| cube | 24 | 12 | **3** | 72 | 36 |
| smoothcube | 8 | 12 | 1 | 8 | 12 |
| **total** | | | **10** | **437** | **640** |

```text
measured by the program : draws 10, triangles 640, vertices submitted 437
worked out by hand      : draws 10, triangles 640, vertices submitted 437
MATCH
```

Every per-mesh figure in that table comes from the `[mesh]` lines the program
already prints at startup, so the whole check can be done from the console output
with no guessing.

### The counters also audited five earlier documents

Every phase document since 18 has carried a "the scene, counted" table claiming a
draw count and a triangle count. Those were worked out by hand at the time and
never verified. Now they can be:

| phase | claimed tris | recomputed | claimed draws | recomputed | claimed GPU verts | recomputed | ok |
|---|---:|---:|---:|---:|---:|---:|---|
| 18 | 168 | 168 | 7 | 7 | 112 | 112 | yes |
| 20 | 200 | 200 | 8 | 8 | 144 | 144 | yes |
| 21 | 232 | 232 | 8 | 8 | 178 | 178 | yes |
| 22 | 628 | 628 | 9 | 9 | 378 | 378 | yes |
| 23 | 640 | 640 | 10 | 10 | 386 | 386 | yes |

All fifteen figures check out, and the current one is confirmed against a live
measurement rather than arithmetic alone.

## What changed

| File | Change |
|---|---|
| `src/main.cpp` | New `RenderStats` struct: draw calls, vertices, triangles |
| `src/main.cpp` | `drawMesh()` takes a `RenderStats&` and accumulates into it |
| `src/main.cpp` | `renderScene()` now **returns** the frame's `RenderStats` |
| `src/main.cpp` | New `updateWindowTitle()`, called once per frame from the loop |
| `src/Mesh.h` | Unchanged - `vertexCount()` and `triangleCount()` already existed |
| `shaders/*` | Unchanged |

No new geometry, no new GL state, no new uniform. `Mesh` has exposed
`vertexCount()`, `indexCount()` and `triangleCount()` since Phase 14, waiting for
exactly this.

## The one idea

**Count at the one place every draw passes through.**

```cpp
static void drawMesh(ShaderProgram& shader,
                     RenderStats& stats,
                     const Mesh& mesh,
                     const glm::mat4& model,
                     const glm::vec3& tint)
{
    shader.setVec3("uTint", tint);
    shader.setMat4("uModel", model);
    mesh.draw();

    ++stats.drawCalls;
    stats.vertices += mesh.vertexCount();
    stats.triangles += mesh.triangleCount();
}
```

Every draw in the project goes through `drawMesh()` - that has been true since
Phase 16. Counting there means the totals **cannot drift away from reality**. Add a
draw, remove one, or accidentally issue one twice, and the numbers change by
themselves with nothing else to remember to update.

The alternative - working the totals out from the config - would be a second
source of truth that silently goes stale. Which brings up the distinction this
phase is really about.

## A measurement is not a prediction

The project now has two things that report geometry counts, and they are different
in kind:

| | the `W` key (Phase 19) | the window title (Phase 24) |
|---|---|---|
| where the numbers come from | `GridConfig::CELLS`, by formula | the meshes, as they are drawn |
| what it tells you | what the code **intends** | what the code **did** |
| when it is written | on a key press | every frame |
| if the code is wrong | it still prints the intended number | it prints the wrong number, visibly |

Both are worth having, and the reason is the whole point: **a prediction and a
measurement that disagree have found you a bug.** Phase 19's line says a grid at
`CELLS = 8` should be 128 triangles; the title says the frame drew 640 across ten
draws. Either can be checked against the other and against the `[mesh]` lines.

That is why this phase, which adds no feature anyone asked for, is worth its place
before Phase 25 starts changing geometry at runtime.

## `verts 437` is not `386`, and the gap is the point

Two different vertex counts are worth keeping straight:

```text
vertices resident on the GPU (each mesh uploaded once) : 386
vertices submitted per frame                          : 437
saved by reuse                                        :  51
```

The title reports **437**, the per-frame figure, because that is what the frame
costs. The 386 is what the graphics card is storing.

The 51-vertex gap is the mesh-reuse argument, itemised: the triangle mesh is drawn
twice (`+3`) and the cube mesh three times (`+48`). Without reuse the project would
have to store all 437, and every extra copy of a cube would cost another 24
vertices of memory instead of nothing at all.

It is a modest saving today. Stage I puts a fleet of ships in the water from one
set of meshes, and then it is not modest. Phase 62 measures it properly for the
finished scene.

## Why the title is only rewritten when a number changes

```cpp
if (stats.drawCalls == lastShown.drawCalls &&
    stats.vertices == lastShown.vertices &&
    stats.triangles == lastShown.triangles) {
    return;
}
```

`glfwSetWindowTitle` goes through to the operating system. Asking Windows to
re-set the same string 120 times a second is pure waste, and on some window
managers it causes visible flicker.

The counters themselves are still accumulated every frame - that is three integer
adds per draw, which costs nothing - and only the *title write* is skipped. In this
scene the geometry never changes, so the title is written **once**; verified, the
`[counters]` line appears exactly one time in a four-second run.

That is not a limitation. It is the correct behaviour for a counter that is
genuinely constant, and Phase 25's `+` and `-` keys are what will make it move.

## The title bar as a HUD

Drawing text properly needs a font, a texture atlas, a second shader and its own
geometry - a large amount of machinery for a few numbers, and all of it outside
this project's scope. The title bar needs `snprintf` and one GLFW call.

It has one advantage over the console too: **it is inside the window**, so it
appears in a screen recording or a screenshot of the running program. A console
window usually is not, and for a graded demonstration that matters. The counters
are also printed to the console once, so they survive in the log if the title bar
is cropped out of shot.

Phase 61 grows this same mechanism into the full HUD - shading mode, environment,
heading, azimuth, reload, flight time - and this phase is the small first version
of it.

## A bug caught while writing this

The first version put the returned stats in a local called `stats`:

```cpp
const RenderStats stats = renderScene(...);
```

`main()` already has a `FrameStats stats` for the timing report, declared before
the loop. The new declaration would **shadow** it, and `reportFrame(stats, clock)`
a few lines later would be handed a `RenderStats`. That is a type error, so the
compiler would have caught it - but it would have been a confusing one, and
`/W4` would have flagged the shadowing as `C4456` first.

It is renamed `frameCost`, with a comment saying why. Two variables called `stats`
measuring different things in the same scope is worth avoiding even when the
compiler would save you.

## Where each job happens

| Function | Job in this phase |
|---|---|
| `drawMesh()` | Adds one draw, its vertices and its triangles to the running total |
| `renderScene()` | Starts a fresh `RenderStats` at zero and returns it |
| `updateWindowTitle()` | Formats the title and writes it, but only when a number changed |
| the main loop | Captures the stats as `frameCost` and passes them on |

## Likely teacher questions

### Where are the counters measured, and why there?

In `drawMesh()`, because every draw in the project goes through it. Counting at
the single funnel means the totals cannot get out of step with what was actually
drawn - there is no second place to remember to update.

### Does the reported triangle count match a hand calculation?

Yes, exactly. Sum each mesh's triangle count times the number of times it is
drawn: `1x2 + 2 + 128 + 64 + 396 + 12x3 + 12 = 640`, with 10 draws. The program
measures the same. Each mesh's own figures come from the `[mesh]` lines printed at
startup.

### Why does the title say 437 vertices when the meshes add up to 386?

Because 437 is what is **submitted** per frame and 386 is what is **stored** on the
graphics card. The difference of 51 is mesh reuse: the triangle is drawn twice
(+3) and the cube three times (+48). Without reuse all 437 would have to be stored.

### Why not update the title every frame?

`glfwSetWindowTitle` is an operating-system call, and re-setting an identical
string 120 times a second is waste and can flicker. The counters are still
accumulated every frame; only the write is skipped. Here the geometry is constant,
so the title is written once - confirmed, the `[counters]` line appears exactly
once per run.

### Why the title bar rather than text on screen?

Real text needs a font, a texture atlas, a shader and geometry - a lot of
machinery outside this project's scope. The title bar needs `snprintf` and one
call, and unlike a console window it appears in a screen recording of the
program.

### How is this different from what the `W` key prints?

The `W` key predicts, from `GridConfig`, what the grid *should* contain. The title
measures what the frame *did* contain. A prediction tells you the intent, a
measurement tells you the result, and the two disagreeing is how you find a bug.

### What would happen if you added a draw and forgot to update the counters?

Nothing to forget - that is the design. The count comes from `drawMesh()`, so a new
draw counts itself.

## Simple viva modifications

- **The named exercise: add the vertex count to the title.** It is already there;
  the natural extension is to add a **fourth** number, the vertices resident on the
  GPU (sum each mesh's `vertexCount()` once, at startup). Showing `verts 437 sent /
  386 stored` puts the whole mesh-reuse argument in the title bar.
- **Add the index count:** `Mesh::indexCount()` exists and is unused by the HUD.
- **Prove the counters are measured, not hard-coded:** comment out one `drawMesh`
  call. The draw count drops by one and the triangle count by that mesh's share,
  with nothing else edited.
- **Prove the per-draw counting:** change `CubeConfig::POSITIONS` to hold four
  cubes. Draws go 10 to 11, triangles 640 to 652, submitted vertices 437 to 461 -
  and the stored count does not move at all.
- **Prove the change-detection:** add FPS to the title and watch it start updating
  every frame; then take it out again and explain why a constant does not need
  rewriting.
- **Check it by hand:** read the `[mesh]` lines at startup, multiply each by how
  many times it is drawn, and confirm the total against the title.

## Checkpoint

Phase 24 passes when:

- Debug and Release builds succeed with no compiler warnings;
- both shaders compile and link, with no missing-uniform warning;
- the window title reports **draw calls, triangles and vertices** for the frame;
- the reported triangle count **matches a hand calculation** from the `[mesh]`
  lines - `640` across `10` draws for the current gallery;
- the counters are measured in `drawMesh()`, not computed from config, so removing
  a draw changes them with no other edit;
- the title is not rewritten when nothing has changed;
- every earlier object and key still behaves as before;
- the window remains responsive, reports frame timing, and closes cleanly with
  every GPU object freed.

## What is not part of Phase 24

Not the full HUD - shading mode, environment, heading, azimuth, elevation, muzzle
speed, reload, flight time and the rest are Phase 61, and most of what they report
does not exist yet. No FPS in the title: the console has reported it since Phase 1
and duplicating it here would make the title change every frame for no gain.

No runtime tessellation control - the `+` and `-` keys that rebuild meshes live are
Phase 25, and they are what will make these counters visibly move. No optimization
work: Phase 62 is the measurement pass that uses numbers like these to argue about
cost. No lighting.

Phase 25 adds `rebuildMeshes()` on `+` and `-`, with clamped limits - the last
phase of Stage B.

---

## Note: the audit of earlier phases

### Five documents verified, no errors found

Every "the scene, counted" table from Phase 18 onward was recomputed from the mesh
data and draw multiplicities - fifteen figures across five documents - and all
fifteen agree. The current frame is additionally confirmed against a live
measurement.

This is the first phase where those claims could be checked mechanically rather
than trusted, which is a good argument for having built the counters at all.

### One bug, introduced and fixed within the phase

The shadowed `stats` variable described above. Caught before building, renamed to
`frameCost`.

### One real pre-existing defect found and fixed: `LNK4098` in Debug

A clean Debug build was emitting a linker warning that had been there since
Phase 1 and had never been reported:

```text
LINK : warning LNK4098: defaultlib 'MSVCRT' conflicts with use of other libs
```

The cause, confirmed with `dumpbin /DIRECTIVES` on the vendored library:

```text
CRT directives found in external/glfw/lib-vc2022/glfw3.lib: MSVCRT
```

The prebuilt GLFW was compiled against the **release** C runtime. A Debug build of
this project uses the **debug** runtime (`MSVCRTD`), so the linker is asked for two
C runtimes in one executable and warns. Release builds match GLFW and never
warned, which is why it went unnoticed.

Fixed in [CMakeLists.txt](../CMakeLists.txt) by excluding the release runtime from
the Debug link only:

```cmake
set_property(TARGET ship_battle_simulator APPEND_STRING
    PROPERTY LINK_FLAGS_DEBUG " /NODEFAULTLIB:MSVCRT")
```

`set_property` is used rather than `target_link_options` because the latter needs
CMake 3.13 and this project declares 3.10.

Verified: the generated Visual Studio project lists `MSVCRT` under
`IgnoreSpecificDefaultLibraries` in the **Debug** configuration block and in none
of the other three, both configurations then build with no warnings at all, and
both executables run, print identical counters, and exit 0.

### A note on how this was missed for 24 phases

Every checkpoint says "no compiler warnings", and this is a **linker** warning, so
the letter of the checkpoint was met. But the intent was not, and earlier
verification filtered build logs for compiler diagnostics in a way that let a
`LINK :` line through. Build logs are now checked for `warning` and `error`
anywhere, not for compiler warnings specifically.

### Still open, and still deliberate

The test triangle passes through the largest cube about 0.81% of the time, a
consequence of the Phase 19 pulse fix. It renders correctly and moving a cube
would invalidate measured evidence in three documents. Recorded in full at the end
of [PHASE_20_EXPLANATION.md](PHASE_20_EXPLANATION.md).

One checkpoint elsewhere is superseded and annotated: Phase 20's cylinder counts
(`32 / 96 / 32`), which Phase 21 changed to `66 / 192 / 64` by adding the end caps.
