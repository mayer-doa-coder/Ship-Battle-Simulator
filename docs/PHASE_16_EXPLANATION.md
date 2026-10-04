# Phase 16 - The Unit-Mesh Rule and `drawMesh()`

## Status

Phase 16 is complete and verified. Debug and Release builds succeeded with no
compiler warnings, both shaders linked with no missing-uniform warning, and a
live Release run showed three differently sized cubes turning in step, drawn
from one mesh. Nothing was printed to the error output, the loop held about
`120 FPS`, and the program closed cleanly.

This is a graded milestone (`*` in the plan), and unlike Phases 14 and 15 it
**deliberately changes the picture**: there are three cubes now where there was
one.

### The headline number

```text
[mesh] triangle vertices=3  indices=3  triangles=1
[mesh] quad     vertices=4  indices=6  triangles=2
[mesh] cube     vertices=24 indices=36 triangles=12
```

Three cubes on screen. **One** `[mesh] cube` line. The cube is uploaded once, at
startup, and nothing is sent to the graphics card per cube ever again.

### 1. The mesh really is a unit mesh

```text
UNIT_HALF_EXTENT = 0.500
extents: x[-0.500,+0.500] y[-0.500,+0.500] z[-0.500,+0.500]  size=(1.000, 1.000, 1.000)
exactly 1 x 1 x 1, centred on its origin : YES
```

That matters for a practical reason: because the mesh is exactly 1 unit across,
a scale of `3` means "3 units across" with no arithmetic in between. A mesh of
some other size would make every scale value a puzzle.

### 2. Scale in the matrix is *exactly* the same as size baked into the mesh

This is the claim the whole phase rests on, so it was tested against an
independently written "bake the size into the vertices" cube - the Phase 14 way
- rather than against itself:

```text
scale 1.00  -> 1.0 units across, worst vertex difference = 0 (exact)
scale 0.75  -> 0.8 units across, worst vertex difference = 0 (exact)
scale 0.48  -> 0.5 units across, worst vertex difference = 0 (exact)
scale 0.30  -> 0.3 units across, worst vertex difference = 0 (exact)
scale 3.00  -> 3.0 units across, worst vertex difference = 0 (exact)
scale 0.10  -> 0.1 units across, worst vertex difference = 0 (exact)
```

Not "close enough" - **zero** difference, at every scale tried, including one
larger than the original and one much smaller. Putting the size in the matrix
costs nothing in accuracy. It is the same multiplication, done on the GPU at
draw time instead of on the CPU at build time.

### 3. Three cubes, three measurably different sizes, none touching

Each cube was rendered on its own and measured:

| Cube | Scale asked | Screen box | Silhouette |
|---|---:|---|---:|
| 0 large | `0.75` | 216 x 260 px | 42 302 px |
| 1 medium | `0.48` | 147 x 155 px | 17 516 px |
| 2 small | `0.30` | 87 x 89 px | 6 333 px |

Comparing the square roots of the silhouette areas, which is the fair way to
turn an area back into a linear size:

```text
cube 0 / cube 1 : measured 1.554, scales asked 1.563
cube 1 / cube 2 : measured 1.663, scales asked 1.600
```

Within a few percent. The residual is honest perspective, not error: each cube
sits at a slightly different place, so it is seen along a slightly different
ray and its near corner is at a slightly different depth, which changes the
shape of its silhouette a little. The next check removes that variable
completely.

None of the three bounding boxes overlaps another, and all three were confirmed
to sit inside the default view with room to spare:

```text
cube 0 <-> cube 1 : distance 1.253, needs > 1.065  OK (margin 0.188)
cube 0 <-> cube 2 : distance 1.358, needs > 0.909  OK (margin 0.449)
cube 1 <-> cube 2 : distance 1.210, needs > 0.675  OK (margin 0.535)
```

### 4. The viva change, performed and measured

The plan's modification for this phase is *change one cube's scale and show the
mesh data did not change*. It was done: `SCALES[0]` was changed from `0.75` to
`0.45`, one number, nothing else touched. Each cube was then re-rendered and
compared against before:

```text
cube 0 : 83 398 bytes differ -> width 216 px became 129 px
                                (0.597 of before; 0.45 / 0.75 = 0.600)
cube 1 : 0 bytes differ      -> pixel-for-pixel UNCHANGED
cube 2 : 0 bytes differ      -> pixel-for-pixel UNCHANGED
```

This is the cleanest evidence in the phase. Cube 0 did not move, did not turn
differently, and is at the same depth - the **only** thing that changed is the
number in the matrix, and its on-screen width changed by `0.597` against a
requested `0.600`, a difference of half a percent. Meanwhile the other two cubes
are identical byte for byte, and the `[mesh] cube` line still reads
`vertices=24 indices=36 triangles=12`.

### 5. The scale does not disturb the normals

```text
uniform scale leaves every normal's DIRECTION unchanged : YES
```

Confirmed on screen too: with the Phase 15 `N` view on, the whole frame still
contains nothing but face-normal colours and the background. Three cubes of
three sizes, all showing the same colours - because they are the same mesh.

## What changed

| File | Change |
|---|---|
| `src/Mesh.h` | `UNIT_HALF_EXTENT` added; `buildCubeGeometry()` and `makeCube()` **lost their `halfSize` parameter** |
| `src/main.cpp` | New `drawMesh(shader, mesh, model, tint)`; all six draws go through it |
| `src/main.cpp` | `CubeConfig::HALF_SIZE` and `POSITION` gone; `COUNT`, `POSITIONS[3]`, `SCALES[3]` added |
| `src/main.cpp` | `SceneState::cubeModel` became `cubeFrames[3]` - **translation and rotation only, no scale** |
| `src/main.cpp` | `updateScene()` builds three unscaled frames; `renderScene()` draws three cubes in a loop |
| `shaders/*` | Unchanged |

## The one idea

A mesh is **memory on the graphics card**. A matrix is **sixteen numbers sent
with a draw call**. Those two things cost wildly different amounts, so size
belongs in the cheap one.

```text
the wrong way                        the unit-mesh way
-------------                        -----------------
makeCube(mesh1, 0.75, colors)        makeCube(cubeMesh, colors)       <- once
makeCube(mesh2, 0.48, colors)
makeCube(mesh3, 0.30, colors)        drawMesh(..., frame0 * scale(0.75), ...)
                                     drawMesh(..., frame1 * scale(0.48), ...)
3 uploads, 3 VAOs, 72 vertices       drawMesh(..., frame2 * scale(0.30), ...)
on the GPU, and a fourth size
needs a fourth upload                1 upload, 1 VAO, 24 vertices on the GPU,
                                     and any number of sizes for free
```

**The rule:** a generator makes a shape of size 1, centred on its own origin,
and takes no size parameter. An object's real size is a `glm::scale` applied in
its model matrix, at the moment it is drawn.

Phase 14's `makeCube` still took a `halfSize`. Phase 16 takes it away. That
removal *is* the phase.

## Why this is the answer to a real viva question

> "Where is scaling used in this project?"

Every object in the finished scene is a unit mesh scaled to its dimensions. The
hull is a stretched cube. The barrel is a stretched cylinder. A crew member's
arm is a small cube. A particle grows over its lifetime by changing one number.
None of those is a separate mesh - they are all the same handful of unit meshes
with different matrices, and that is the single biggest optimisation in the
project.

You cannot give that answer honestly unless the rule is actually followed, which
is why it gets its own phase rather than being mentioned in passing.

## `drawMesh()` - one call per object

```cpp
static void drawMesh(ShaderProgram& shader,
                     const Mesh& mesh,
                     const glm::mat4& model,
                     const glm::vec3& tint)
{
    shader.setVec3("uTint", tint);
    shader.setMat4("uModel", model);
    mesh.draw();
}
```

Three lines, and it finishes a job that started two phases ago:

| Phase | What moved out of `renderScene()` |
|---|---|
| 14 | which VAO to bind, which draw call to use, how many indices |
| 16 | which matrix, which tint |

What is left in `renderScene()` is one line per object saying what is different
about that object. Six draws now fit in less space than four did before.

```cpp
drawMesh(shader, triangleMesh, scene.triangleModel, AppConfig::TINT);
drawMesh(shader, triangleMesh, scene.farCopyModel,  DepthTestConfig::FAR_COPY_TINT);
drawMesh(shader, quadMesh,     scene.quadModel,     NO_TINT);
for (int i = 0; i < CubeConfig::COUNT; ++i) { ... }
```

The reference project's version is `drawMesh(mesh, model, material)`. This one
carries a tint instead of a material and is handed the shader explicitly rather
than reaching for a global; Phase 29 replaces the tint with a real material and
the two converge.

## Which side the scale goes on, and why it matters

```cpp
drawMesh(shader, cubeMesh, scene.cubeFrames[i] * cubeScale, NO_TINT);
//                         ^^^^^^^^^^^^^^^^^^^   ^^^^^^^^^
//                         frame                 scale, on the RIGHT
```

Read right to left, as always since Phase 6:

1. **`cubeScale`** resizes the unit cube about its own origin, where all 24 of
   its vertices are measured from;
2. **the frame** then spins it and carries it to its place.

Written the other way round, `cubeScale * frame`, the scale would act on the
finished placement instead - it would resize *how far from the origin the cube
sits* as well as the cube. A cube at `x = 2.2` scaled by `0.48` would jump to
`x = 1.056`. That is exactly Phase 6's smear, in three dimensions, and it is
why `T * R * S` has the `S` on the right.

## Frames: the rule that Stage D depends on

A **frame** is a matrix holding where something is and how it is turned - a
translation and a rotation, and deliberately **no scale**:

```cpp
// updateScene(): there is no glm::scale anywhere in this loop
for (int i = 0; i < CubeConfig::COUNT; ++i) {
    const glm::mat4 cubeSlide = glm::translate(glm::mat4(1.0f), CubeConfig::POSITIONS[i]);
    scene.cubeFrames[i] = cubeSlide * cubeSpin;
}
```

`CubeConfig::SCALES` is never even read in `updateScene()`. It is read once, in
`renderScene()`, at the moment of drawing, and the result is never stored back.

With three separate cubes this looks like fussiness. It is not. It is the single
most important habit in the project, and the reason is Stage D:

```text
hull frame  ->  deck frame  ->  cannon mount  ->  yoke  ->  barrel
```

Every one of those is a child of the one before it, which means each one's
matrix is multiplied by its parent's. If a scale were ever left in a parent's
matrix, **every child below it would inherit that scale**. The hull is a
stretched cube - 4 units long and 1 wide - so a scale left in the hull's frame
would stretch the deck, the masts, the cannon, and the crew standing on it, all
by 4 to 1. The project's own notes call this the most common bug in the whole
build.

Storing frames unscaled and multiplying the scale on only inside `drawMesh`
makes that bug **impossible by construction** rather than something to remember
not to do. This phase is where the habit starts, with three cubes, where it is
easy to see and nothing is at stake yet.

### "But `triangleModel` has a scale in it"

It does, and that is worth answering rather than hiding, because a careful
reader will spot it. `scene.triangleModel` still contains the pulsing
`glm::scale` that Phase 6 introduced, built in `updateScene()`.

The rule is about **frames**, not about every matrix. A frame is a matrix that
something else hangs off - a parent in a hierarchy. `triangleModel` is not a
frame: it is a finished model matrix for one object with no children, and its
pulsing scale **is** the Phase 6 demonstration, which must keep working. There
is nothing beneath it to inherit anything.

`scene.cubeFrames` is different in kind. It is named `frames` precisely because
it is the shape the ship hierarchy will take, and so it is held to the stricter
rule from the start. If the triangle ever gained a child, its matrix would have
to be split into a frame plus a draw-time scale in exactly the same way.

## Why all three cubes share one spin

```cpp
const float cubeSpinAngle = CubeConfig::SPIN_SPEED * now;
const glm::mat4 cubeSpin = glm::rotate(glm::mat4(1.0f), cubeSpinAngle, CubeConfig::SPIN_AXIS);
```

Worked out once, outside the loop, and reused by all three. They turn in perfect
step.

That is a deliberate choice for the demonstration: it leaves the **scale** as
the only thing that differs between the three cubes apart from where they stand.
Three cubes spinning at three different speeds would look livelier and prove
less - a viewer could reasonably wonder whether they were three different
objects. Turning identically, they are visibly three sizes of one thing.

It is also, incidentally, two fewer `glm::rotate` calls per frame.

## Why the scales are all uniform

`0.75`, `0.48`, `0.30` - each applied equally to x, y, and z.

A **non-uniform** scale, such as `(4, 1, 1)` for a ship's hull, breaks the naive
way of transforming a normal: the normal comes out no longer perpendicular to
the surface, and needs the normal matrix `(M^-1)^T` to correct it. That is Phase
26's whole lesson, and demonstrating it needs the normal to actually be
transformed, which does not happen until then.

So non-uniform scaling is left out of this phase on purpose. The rule being
taught here - scale lives in the matrix - is the same either way, and
introducing a broken-looking normal eleven phases before the fix is available
would teach the wrong thing.

## What is still not a unit mesh, and why

The triangle and the quad still have their sizes written into their vertex data
by hand: the quad's corners are at `+-0.4`, not `+-0.5`, and no scale is applied
when they are drawn.

That is left alone on purpose. They are the "before" case, still carrying the
hand-written data Phases 2 and 9 gave them, and `makeQuad()` in Phase 17 is the
phase that turns the quad into a proper unit mesh with a generator of its own.
Converting it here would do Phase 17's job and blur what this phase is showing.

So right now the project contains one object that follows the rule and two that
do not - which is a perfectly good thing to be able to point at and explain.

## The scene, counted

| | Before (Phase 15) | After (Phase 16) |
|---|---:|---:|
| Meshes uploaded | 3 | 3 |
| VAOs | 3 | 3 |
| Vertices on the GPU | 31 | 31 |
| Draw calls | 4 | **6** |
| Triangles drawn per frame | 16 | **40** |
| Objects on screen | 4 | **6** |

Two more objects, 24 more triangles per frame, and **not one extra byte on the
graphics card**. That table is the optimisation argument in miniature, and
Phase 62 measures the same thing for the finished scene.

## Where each job happens

| Function | Job in this phase |
|---|---|
| `buildCubeGeometry()` | Builds a 1 x 1 x 1 cube. Takes no size |
| `makeCube()` | Uploads it, once, at startup |
| `updateScene()` | Builds three **unscaled** frames - place and rotation only |
| `drawMesh()` | Sets `uTint` and `uModel`, then draws |
| `renderScene()` | Multiplies each frame by its scale and calls `drawMesh` |

## Likely teacher questions

### What is the unit-mesh rule?

A mesh generator produces a shape exactly 1 unit across, centred on its own
origin, and takes no size parameter. An object's real size is a `glm::scale` in
its model matrix, applied when it is drawn.

### Why not just build a mesh at the size you need?

Because a mesh lives in graphics-card memory and a matrix is sixteen numbers
sent with the draw call. Three sizes would mean three uploads, three VAOs, and
72 vertices stored instead of 24 - and a fourth size would need a fourth upload.
With the scale in the matrix, one mesh covers every size, including sizes decided
while the program is running.

### Is scaling in the matrix as accurate as baking the size in?

Yes, exactly. It was measured against a separately written "baked" cube at six
different scales, and the worst difference was zero every time. It is the same
multiplication, done at a different moment.

### Where is scaling used in this project?

Everywhere - every object is a unit mesh scaled to its dimensions. The hull is a
stretched cube, the barrel a stretched cylinder, a crew member's limb a small
cube, and a particle grows over its life by changing one scale value.

### Why does the scale go on the right of the frame?

Because a matrix product is read right to left, so the rightmost matrix acts
first. The scale must resize the mesh about its own origin **before** the frame
carries it anywhere. On the left it would resize the finished placement too, so
the object's distance from the origin would shrink along with it.

### What is a "frame", and why does it never contain a scale?

A frame is a matrix holding only a position and a rotation. It contains no scale
so that nothing can inherit one. In Stage D the cannon is a child of the deck
and the deck a child of the hull, and the hull is a stretched cube - a scale
left in its frame would stretch the deck, the masts, the cannon, and the crew.
Applying scale only inside `drawMesh` makes that impossible.

### Why do all three cubes spin together?

So that scale is the only difference between them besides position. Different
spins would make them look like three unrelated objects instead of three sizes
of one.

### Why are all three scales uniform?

A non-uniform scale breaks the naive transformation of a normal and needs the
normal matrix to fix it, which is Phase 26. Using one here would show a bug
before the phase that explains it.

### Does scaling a cube change its normals?

Not for a uniform scale - the directions are unchanged, which was both computed
and confirmed on screen with the `N` view. A **non-uniform** scale does change
them incorrectly, and that is precisely what Phase 26 is about.

### How many cubes are on the graphics card?

One. Twenty-four vertices and thirty-six indices, uploaded once at startup, as
the single `[mesh] cube` line at startup confirms.

### If you change one cube's scale, what happens to the mesh?

Nothing at all. This was measured: changing `SCALES[0]` from `0.75` to `0.45`
changed that cube's on-screen width by a factor of `0.597` against a requested
`0.600`, while the other two cubes came out pixel-for-pixel identical and the
mesh still reported 24 vertices and 36 indices.

### The triangle's model matrix contains a scale. Does that break the rule?

No. The rule is about *frames* - matrices that other objects hang off and
inherit. `triangleModel` is a finished model matrix for an object with no
children, and its pulsing scale is Phase 6's demonstration. `cubeFrames` is held
to the stricter rule because it is the shape the ship hierarchy will take.

### Why are the triangle and quad not unit meshes?

Their sizes are still written into their vertex data from Phases 2 and 9.
`makeQuad()` in Phase 17 turns the quad into a unit mesh; doing it here would be
Phase 17's work.

## Simple viva modifications

- **The named change: resize one cube.** Edit one number in
  `CubeConfig::SCALES`, rebuild, and point out that the startup line still says
  `vertices=24 indices=36` and the other two cubes are untouched.
- **Add a fourth cube** by adding one row to `CubeConfig::POSITIONS` and one
  value to `SCALES`. A fourth cube appears with **no** new mesh, no new upload
  and no new VAO. `COUNT` counts the positions for you, so there is no third
  thing to keep in step; if you forget the scale, a `static_assert` says so.
  (Phase 18 changed this. It used to need `COUNT` edited by hand as well, and
  an identity matrix added to `SceneState::cubeFrames` - see the note at the end
  of [PHASE_18_EXPLANATION.md](PHASE_18_EXPLANATION.md).)
- **Make one cube huge:** set a scale to `3.0` and watch one mesh fill the
  screen while the other two are unchanged.
- **Move a cube:** edit a row of `POSITIONS`. Note that it is the frame that
  changes, not the scale.
- **Break the order on purpose:** change `scene.cubeFrames[i] * cubeScale` to
  `cubeScale * scene.cubeFrames[i]` and watch the cubes slide toward the origin
  as well as shrinking - Phase 6's lesson, in 3D. Put it back.
- **Show the leak the frames prevent:** put a `glm::scale` into the frame inside
  `updateScene()` instead, prove the picture looks the same for now, and then
  explain why that version would be a disaster for Stage D's ship.
- **Prove it is one mesh with the normals view:** press `N`. All three cubes
  show the same colours on the same faces at the same moment, because they are
  the same 24 vertices.
- **Prove it is one mesh with wireframe:** press `W` and count - each cube shows
  the same 12 triangles.

## Checkpoint

Phase 16 passes when:

- Debug and Release builds succeed with no compiler warnings;
- both shaders compile and link, with no missing-uniform warning;
- **three cubes of three clearly different sizes** are visible, turning in step;
- the startup output contains exactly **one** `[mesh] cube` line reporting
  `vertices=24 indices=36 triangles=12`;
- `makeCube()` takes no size parameter, and no `glm::scale` appears anywhere in
  `updateScene()`;
- changing one entry of `CubeConfig::SCALES` resizes exactly one cube and leaves
  the mesh data and the other cubes untouched;
- the two triangles, the quad, the `N`, `D`, `W`, and `O` keys, and the mouse
  orbit and zoom all still behave as before;
- the window remains responsive, reports frame timing, and closes cleanly with
  every GPU object freed.

## What is not part of Phase 16

No new shapes - `makeQuad()`, `makeGrid()`, `makeCylinder()`, and `makeSphere()`
are Phases 17 to 22, and the quad and triangle still carry hand-written vertex
data. No non-uniform scaling, and so no normal matrix: that pairing is Phase 26.
No hierarchy - the three cubes are independent, and parent-child frames arrive
in Stage D. No lighting.

**Still deliberately unfixed.** The Stage A review's finding on the Phase 6
pulse maths (`* 0.1f` where `* 0.5f` was meant, in `src/main.cpp`) is untouched
for a third phase, so that the pixel comparisons across Phases 14, 15, and 16
stayed like-for-like. All three milestones are now proven, so this is a good
moment to fix it as its own small change.

**A correction made to Phase 15's document.** While verifying this phase, the
normals view produced `127` where `PHASE_15_EXPLANATION.md` recorded `128`.
Neither is wrong: `0.5 x 255 = 127.5` is an exact tie and the driver may round
it either way. The same unmodified executable gives `128` on this machine's
Intel GPU and `127` on its NVIDIA one. Phase 15's document has been corrected to
predict the float values, which are exact everywhere, and to note that the
mid-grey channel reads as 127 or 128 depending on the machine. It is also a
reminder to pin the executable to one GPU before any FPS measurement later on.

Phase 17 adds `makeQuad()`: the simplest generator, four vertices with normal
`(0, 0, 1)`, which finally makes the quad a unit mesh too.
