# Phase 25 - Rebuilding Meshes While the Program Runs

## Status

Phase 25 is complete and verified, and it is the **last phase of Stage B**. Every
mesh the project will ever need now exists, and the amount of geometry in them can
be changed without restarting.

Debug and Release both build with no compiler warnings, the strict `/W4` build
produces none either, and three separate test suites covering **3,010 checks**
passed with zero failures.

```text
Ship Battle Simulator - Phase 25 | draws 10 | tris 640 | verts 437
```

Press `+` and the grid, cylinder and sphere are thrown away and built again, more
finely, while the scene keeps turning. Press `-` and they come back coarser. Hold
`W` at the same time and you can watch the polygon count change.

### THE CHECKPOINT: the count changes live, and 100 rebuilds leak nothing

Both halves pass. The measured frame cost at every level the keys can reach:

| level | grid | cylinder | sphere | draws | triangles | vertices submitted |
|---:|---:|---:|---:|---:|---:|---:|
| −2 | 2 | 4 | 3 × 4 | 10 | 92 | 127 |
| −1 | 4 | 8 | 6 × 9 | 10 | 206 | 196 |
| **0** | **8** | **16** | **12 × 18** | **10** | **640** | **437** |
| +1 | 16 | 32 | 24 × 36 | 10 | 2,348 | 1,339 |
| +2 | 32 | 64 | 48 × 72 | 10 | 9,124 | 4,823 |
| +3 | 64 | 128 | 96 × 144 | 10 | 36,116 | 18,511 |

Level 0 is exactly the Phase 24 frame. The top level is **56 times** its triangle
count, and every one of these numbers was *measured* with the Phase 24 counters,
not worked out by hand.

**The draw count is 10 at every single level.** That is the most important column
in the table, and the next section explains why.

For the leak half:

```text
100 rebuilds, 300 mesh uploads
  every upload succeeded                                    OK
  no OpenGL error raised by any rebuild                     OK
  rebuilds that got the SAME vertex-array name back: 100/100
  lowest free name before: buffer 15, vertex array 8
  lowest free name after : buffer 15, vertex array 8        OK
```

## What changed

| File | Change |
|---|---|
| `src/main.cpp` | New `DetailConfig` namespace: `LEVEL_START`, `LEVEL_MIN`, `LEVEL_MAX` |
| `src/main.cpp` | `CELLS_MIN`, `SEGMENTS_MIN`, `STACKS_MIN`, `SLICES_MIN` beside the level-0 values |
| `src/main.cpp` | New `MeshDetail` struct: a level, the four division counts, and `needsRebuild` |
| `src/main.cpp` | New `detailAtLevel()`, `applyDetailLevel()`, `changeDetail()` |
| `src/main.cpp` | New `rebuildMeshes()`; `createMeshes()` now calls it |
| `src/main.cpp` | `+` and `-` in `processInput()`; the rebuild step in the main loop |
| `src/main.cpp` | `SceneState scene` moved above the mesh declarations |
| `src/main.cpp` | The `W` key's grid prediction now reads `scene.detail.cells` |
| `src/Mesh.h` | **Unchanged.** `upload()` has called `destroy()` first since Phase 14 |

That last row is the quiet headline of this phase.

## The one idea

**A mesh can be replaced, and the one line that makes it safe was written eleven
phases ago.**

```cpp
bool upload(const char* name, ...)
{
    // Uploading into a Mesh that already holds buffers would leak the old
    // ones. Freeing first also makes this safe to call twice, which
    // Phase 25's rebuildMeshes() will rely on.
    destroy();
    ...
```

That comment has been sitting in [src/Mesh.h](../src/Mesh.h) since Phase 14,
naming this phase by number. Everything else here is plumbing: a level, two keys,
and a function that calls three generators. The thing that could have gone
catastrophically wrong - abandoning three GPU buffers on every key press, forever,
with no way to free them - was already prevented.

This is worth noticing as a habit, not just a fact. `destroy()` was not put there
because a leak had been observed. It was put there because the author asked "what
happens if this is called twice?" and answered it in the code.

## `rebuildMeshes()` builds three meshes, not seven

```cpp
static bool rebuildMeshes(Mesh& gridMesh, Mesh& cylinderMesh, Mesh& sphereMesh,
                          const MeshDetail& detail)
{
    if (!makeGrid(gridMesh, detail.cells, GridConfig::CORNER_COLORS))
        return false;
    if (!makeCylinder(cylinderMesh, detail.segments, ...))
        return false;
    return makeSphere(sphereMesh, detail.stacks, detail.slices, ...);
}
```

The triangle, quad, cube and smooth cube take no division parameter - they are
always 3, 4, 24 and 8 vertices - so there is nothing about them a detail level
could change. Rebuilding them would do identical work for an identical result.

`createMeshes()` calls this same function for the startup build, so the arguments
for these three uploads are written **once**. A second copy inside
`createMeshes()` would be a copy free to drift: someone adding a parameter in one
place and not the other would get a program whose startup geometry differed from
its geometry after one `+` and one `-`.

## Why the level is one number and not four

The first version of this phase gave each parameter its own doubling and its own
limits, halving the **current** value on each press. It was wrong, and the test
caught it:

```text
start                 : stacks 12  slices 18
after '-' '-'         : stacks  3  slices  4
after '+' '+'         : stacks 12  slices 12     <-- not 18
```

Integer halving throws away the remainder. `18 → 9 → 4`, and doubling 4 twice
gives 16, clamped and rounded along the way to 12. So two presses down and two
presses up did not come back to where it started.

That is more than untidy. **Phase 22 chose 12 and 18 precisely because they are
different**, so that a bug swapping latitude for longitude could not hide behind
matching numbers. A detail scheme that quietly turned the sphere into 12 × 12
would have undone that - and the program would have looked completely fine.

The fix is to stop mutating. One `level` is stored, and every division count is
worked out **from the level-0 constant** each time:

```cpp
static constexpr int detailAtLevel(int startValue, int level, int minimum)
{
    const int scaled = (level >= 0) ? (startValue << level) : (startValue >> -level);
    return (scaled < minimum) ? minimum : scaled;
}
```

Now level 0 always means exactly 8, 16, 12 and 18, however many presses it took to
get back there, and the clamping is harmless because it is recomputed rather than
accumulated.

A shift rather than a multiply is deliberate: each level **doubles**, so `<< 3` is
"times 8" and `>> 2` is "divided by 4", and a right shift rounds down exactly as
integer division would. Doubling rather than adding one is also deliberate - going
from 8 cells to 9 is invisible, 8 to 16 is obvious, so each press is worth
pressing, and the whole range is six presses wide instead of a hundred.

### The compiler now enforces both properties

```cpp
static_assert(detailAtLevel(SphereConfig::SLICES, 0, SphereConfig::SLICES_MIN)
                  == SphereConfig::SLICES, "level 0 must give SphereConfig::SLICES");

static constexpr bool sphereDivisionsDifferAt(int level) { ... }

static_assert(sphereDivisionsDifferAt(-2) && sphereDivisionsDifferAt(-1)
                  && sphereDivisionsDifferAt(0) && sphereDivisionsDifferAt(1)
                  && sphereDivisionsDifferAt(2) && sphereDivisionsDifferAt(3),
              "sphere STACKS and SLICES must differ at every reachable detail level");
```

`detailAtLevel` is `constexpr`, so these run at **compile time**. If anyone later
changes a level-0 value, a floor, or the level range in a way that makes the
sphere's two division counts equal at some level, the project stops compiling and
says so. The bug that was found by a test is now prevented by the compiler.

## Why the draw count never changes

This is the column of the table worth stopping on:

| | triangles | **draw calls** |
|---|---:|---:|
| level 0 | 640 | **10** |
| level +3 | 36,116 | **10** |

Fifty-six times the geometry, the same ten draws. Detail changes how finely the
same objects are divided; it never changes how many objects there are.

That distinction is the reason the two numbers are both in the window title, and
it is the whole argument Phase 103 will make about optimization: a frame's cost has
two independent parts, and they are improved by completely different means. More
triangles cost vertex processing and rasterisation. More draws cost the
*conversation* between the CPU and the graphics card - state changes, uniform
uploads, command submission - and that overhead is often the one that matters.

Press `+` three times and you can see which one is which, live, in the title bar.

## Where the rebuild happens, and why there

```cpp
while (...) {
    updateClock(clock);
    processInput(window, scene);

    if (scene.detail.needsRebuild) {
        if (!rebuildMeshes(gridMesh, cylinderMesh, sphereMesh, scene.detail)) { ... break; }
        scene.detail.needsRebuild = false;
    }

    updateScene(...);
    renderScene(...);
```

`processInput()` changes the level and sets a flag. It never touches a mesh. Three
reasons:

- **the key handler has no business owning GPU objects.** It reads the keyboard
  and writes scene state, exactly as it has since Phase 6;
- **rebuilding after `renderScene()`** would draw one frame at the old detail
  after the new detail had already been announced on the console;
- **a flag collapses repeats.** If some future key also changed the detail, two
  changes in one frame would still produce one rebuild.

A failed rebuild leaves the meshes empty, so there is nothing sensible to draw and
the loop ends with `break` rather than `return` - which means the normal cleanup
at the bottom of `main()` still runs and still frees everything.

## Proving there is no leak

"No leak" is the hard half of the checkpoint, because a leak is invisible: the
program looks and behaves identically either way, until it runs out of memory.

The test uses the fact that **OpenGL reuses object names**. Ask for a vertex array
and the driver gives you the lowest free number. So:

```text
rebuilds that got the SAME vertex-array name back: 100 of 100
```

Each rebuild got back the very name the grid had just been using. A name can only
be reissued if it is free, and it can only be free if `destroy()` really deleted
the old object before the new one was generated. A leaked VAO would still be live,
and its name could not come back.

And across all 300 uploads:

```text
lowest free name before: buffer 15, vertex array 8
lowest free name after : buffer 15, vertex array 8
```

Nothing accumulated at all.

### A passing test that cannot fail proves nothing

Both results above would also appear if the test were simply blind. So the suite
runs the whole thing a second time with **one line disabled** - the `destroy()` at
the top of `Mesh::upload()` - reintroducing exactly the leak this phase depends on
not having:

```text
=== negative control: the same test with destroy() disabled ===
  rebuilds that got the SAME vertex-array name back: 0 of 100
  lowest free buffer name moved     15 -> 615
  lowest free vertex array name moved 8 -> 308
```

Look at those numbers. 300 uploads leaked exactly **600 buffers** (two each, a VBO
and an EBO) and exactly **300 vertex arrays**. The probe did not merely notice the
leak, it counted it precisely. That is what makes the passing run meaningful.

## A quieter bug this phase had to fix

Phase 19's `W` key prints what the grid *should* contain, predicted from the
configuration. It read `GridConfig::CELLS` - a `constexpr`, now only the starting
value:

```cpp
const int n = scene.detail.cells;   // was GridConfig::CELLS
```

Left alone it would have kept printing 8 after `+` had already rebuilt the grid at
16. A prediction that cannot be wrong is worthless, and one that is *silently*
wrong is worse than none: it would have made the Phase 19 demonstration look
broken while the program was correct.

## Where each job happens

| Function | Job in this phase |
|---|---|
| `detailAtLevel()` | One mesh's divisions at one level, from its level-0 constant |
| `applyDetailLevel()` | Fills in all four division counts from `detail.level` |
| `changeDetail()` | Moves the level one step, clamps it, asks for a rebuild |
| `processInput()` | Reads `+` and `-` with the usual held-key edge detection |
| `rebuildMeshes()` | Uploads the three parameterised meshes |
| `createMeshes()` | Calls `rebuildMeshes()` for the startup build, plus the other four |
| the main loop | Performs the rebuild between reading input and using the meshes |

## Likely teacher questions

### What happens to the old mesh when you press `+`?

`Mesh::upload()` calls `destroy()` before it uploads anything, so the old VAO, VBO
and EBO are deleted and handed back to the driver first. The handles are replaced,
not added to. Without that one line every press would abandon three GPU buffers
permanently.

### How do you know it does not leak?

Two ways. The driver reissued the exact same vertex-array name on all 100 rebuilds,
which is only possible if the old object was deleted first. And the lowest free
buffer name was 15 before 300 uploads and still 15 afterwards. The same test with
`destroy()` disabled shows the name climbing from 15 to 615 - exactly 600 leaked
buffers - which proves the test can detect a leak rather than being blind to one.

### Why does the draw count stay at 10 when the triangles go up 56 times?

Because detail changes how finely the same objects are divided, not how many
objects there are. There are always ten draws because there are always ten things
drawn. The two numbers are independent, and a frame can be slow for either reason.

### Why is the detail one level rather than a value per mesh?

So that going down and back up returns to exactly where it started. Halving a
running value loses the remainder - 18 becomes 9 then 4 - so `-` `-` `+` `+` left
the sphere at 12 × 12 instead of 12 × 18. Phase 22 made those two numbers
different on purpose, so that a latitude/longitude mix-up could not hide, and
collapsing them would have thrown that away. Computing each level from the
original constant makes it reversible, and a `static_assert` now checks it at
compile time.

### Why do `+` and `-` not rebuild inside the key handler?

The key handler reads input and writes scene state; it should not own GPU objects.
It sets `needsRebuild`, and the main loop does the work between reading input and
using the meshes - before rendering, so no frame is ever drawn at the old detail
after the new level was announced.

### Why rebuild only three of the seven meshes?

The other four take no division parameter. A cube is always 24 vertices, a quad
always 4, so there is nothing a detail level could change about them. Rebuilding
them would be identical work for an identical result.

### What stops `-` from making a mesh that is not a surface?

Two things. The level is clamped to `LEVEL_MIN`, and each mesh also has its own
`_MIN` floor - a grid needs at least one cell, a cylinder at least three segments
to enclose a volume, a sphere at least three stacks and slices. The floors are a
safety net under the level clamp, so lowering a level-0 value later cannot produce
a degenerate shape.

### Why does the cylinder look like a box at the lowest level?

Because at `SEGMENTS = 4` it *is* a box - a square prism. That is the point: it
makes it impossible to mistake a round-looking shape for a genuinely round one.
It is also the setup for Phase 33's Demo B, where the same faceting is what breaks
a specular highlight on the cannon barrel.

## Simple viva modifications

- **The named exercise: change the detail limits.** `DetailConfig::LEVEL_MIN` and
  `LEVEL_MAX` in [src/main.cpp](../src/main.cpp). Raise `LEVEL_MAX` to 4 and the
  grid goes to 128 cells and 32,768 triangles; the `static_assert` will tell you
  immediately if a wider range breaks the sphere's unequal divisions.
- **Change what a level means:** make it ×3 instead of ×2 by replacing the shift
  with a multiply. Fewer, bigger steps.
- **Prove the rebuild is real:** press `W` for wireframe and hold `+`. The outlines
  get finer while the shapes stay exactly the same size - division and size are
  still separate ideas, as they have been since Phase 16.
- **Prove nothing leaks:** press `+` and `-` a hundred times and watch Task
  Manager's GPU memory. It does not climb.
- **Prove reversibility:** press `-` twice then `+` twice and check the console
  reads `STACKS 12 SLICES 18` again, not `12 12`.
- **Prove detail is not objects:** watch the title while pressing `+`. Triangles
  multiply, draws stay at 10.
- **Rebuild one mesh only:** comment out the `makeSphere` call in
  `rebuildMeshes()`. The grid and cylinder change detail while the ball does not.

## Checkpoint

Phase 25 passes when:

- Debug and Release builds succeed with no compiler warnings;
- both shaders compile and link, with no missing-uniform warning;
- `+` and `-` rebuild the grid, cylinder and sphere at a new detail level;
- the **polygon count changes live**, visibly in wireframe and numerically in the
  window title, while the draw count stays at 10;
- **100 rebuilds produce no OpenGL error and no leak**, demonstrated by the driver
  reissuing the same object names and by the lowest free name not moving;
- `-` then `+` returns to exactly the starting detail, sphere included;
- a held key steps the level once, not once per frame;
- the `W` key's prediction reports the *current* grid division, not the starting
  one;
- every earlier object and key still behaves as before, and level 0 is byte-for-
  byte the Phase 24 scene: 10 draws, 640 triangles, 437 vertices;
- the window remains responsive, reports frame timing, and closes cleanly with
  every GPU object freed.

## What is not part of Phase 25

No new shapes - Stage B is finished and the five meshes are final. No lighting:
Phase 26 begins Stage C with the normal matrix, and everything about how these
surfaces are *shaded* belongs there. No level-of-detail switching by distance; the
detail here is a demonstration control, not an optimization. No measurement of
rebuild cost - Phase 103 is the phase that argues about performance with numbers.

Stage B is done. **Phase 26 starts Stage C**, with
`uNormalMatrix = (M⁻¹)ᵀ` computed on the CPU once per object per frame - the fix
that keeps a normal correct on a non-uniformly scaled object, checked with the
Phase 15 debug view on a stretched cube.

---

## Note: verification and the audit of earlier phases

### Three suites, 3,010 checks, no failures

| Suite | What it does | Checks |
|---|---:|---:|
| CPU geometry | Calls the real `build*Geometry` functions at **every integer level** in all four ranges, and the full invariants - watertight, no degenerate triangles, unit normals, exact counts - at every reachable level | 2,946 |
| GL harness | The real `changeDetail`, `rebuildMeshes`, `renderScene` and `processInput` against a live OpenGL context, including the 100 rebuilds, the negative control and a fake keyboard | 32 |
| Build and run | Clean Debug, Release and strict `/W4`, plus running both executables | 32 |

The GL harness drives the project's own `processInput()` with a faked
`glfwGetKey`, so the key constants and the held-key edge detection that are under
test are the project's own code, not a reimplementation of it.

### Two bugs found and fixed inside this phase

1. **Non-reversible detail**, described above. Found by a test that pressed `-`
   twice and `+` twice and compared. Fixed by deriving every level from the
   level-0 constants, and now prevented by `static_assert`.
2. **A stale prediction.** The `W` key still predicted from `GridConfig::CELLS`
   after detail became changeable.

### Two faults in my own test code, both corrected

- The first leak test guarded on `oldVao != newVao`, so when the driver reissued
  the *same* name - which is the normal case, and itself the proof - every check
  was skipped. It reported "nothing leaked" having tested nothing. Rewritten
  around name reuse, and given a negative control.
- The Phase 24 suite's "no shader problem reported" check only tested that the
  word *shader* appeared somewhere in the output. It has been replaced with a
  check for the `[shader] linked program N` line and the absence of any failure
  text.

### No errors found in earlier documents

The Phase 24 scene table was recomputed from the generators and still reads
10 draws / 640 triangles / 437 vertices, and Phases 18 and 20-23 were re-verified
in Phase 24. The counts in every Stage B document stand.

### Still open, and still deliberate

The test triangle passes through the largest cube about 0.81% of the time, from
the Phase 19 pulse fix. It renders correctly and moving a cube would invalidate
measured evidence in three documents. Recorded in full at the end of
[PHASE_20_EXPLANATION.md](PHASE_20_EXPLANATION.md).

One checkpoint elsewhere is superseded and annotated: Phase 20's cylinder counts
(`32 / 96 / 32`), which Phase 21 changed to `66 / 192 / 64` by adding the end caps.
