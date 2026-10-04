# Phase 20 - `makeCylinder(segments)`: the First Curved Surface

## Status

Phase 20 is complete and verified. Debug and Release builds succeeded with no
compiler warnings, both shaders linked with no missing-uniform warning, and a
live Release run showed an open tube standing on the left of the scene. Nothing
was printed to the error output, the loop held about `120 FPS`, and the program
closed cleanly.

The strict-warning build was **re-verified properly this phase**, because the way
it had been run was unsound - see "A flaw in my own verification" at the end of
this document. Confirmed for the current code, which is cumulative and therefore
covers every phase from 14 to 20:

```text
CMakeCache.txt : CMAKE_CXX_FLAGS:STRING=/W4 /permissive- /EHsc
vcxproj        : <WarningLevel>Level4</WarningLevel>
actual cl line : /W4 /WX-
full clean rebuild, warnings in project sources: none
```

```text
[mesh] triangle vertices=3  indices=3   triangles=1
[mesh] quad     vertices=4  indices=6   triangles=2
[mesh] grid     vertices=81 indices=384 triangles=128
[mesh] cylinder vertices=32 indices=96  triangles=32
[mesh] cube     vertices=24 indices=36  triangles=12
```

At `SEGMENTS = 16`: `2 x 16 = 32` vertices, `2 x 16 = 32` triangles,
`6 x 16 = 96` indices.

> **Superseded by Phase 21.** Those counts are for the side wall alone. Phase 21
> added the two end caps to the same mesh, so the current code reports
> `vertices=66 indices=192 triangles=64` and the formulas become
> `4 * segments + 2` and `4 * segments`. Everything else in this document still
> describes the shipped code. See
> [PHASE_21_EXPLANATION.md](PHASE_21_EXPLANATION.md).

### 1. The formulas hold for every segment count from 3 to 64

```text
vertices = 2*segments and indices = 6*segments, for all 62 values   OK
every index is inside the vertex array                              OK
every vertex is referenced                                          OK
no NaN or infinity anywhere                                         OK
```

| segments | vertices | triangles | indices |
|---:|---:|---:|---:|
| 3 | 6 | 6 | 18 |
| 6 | 12 | 12 | 36 |
| 16 | 32 | 32 | 96 |
| 64 | 128 | 128 | 384 |

### 2. The shape

```text
extents x[-0.5000,+0.5000] y[-0.5000,+0.5000] z[-0.5000,+0.5000]
exactly 1 unit tall, centred on its own origin                      OK
worst deviation from a radius-0.5 circle: 2.98e-08                  OK
exactly two rings of vertices (a bottom and a top)                  OK
the bottom ring is BOTTOM_COLOR and the top ring is TOP_COLOR       OK
```

### 3. The analytic normals are exactly right

| segments | unit-length error | max abs(normal.y) | vs exact radial | top vs bottom |
|---:|---:|---:|---:|---:|
| 6 | 0 | 0 | 0 | 0 |
| 16 | 1.19e-07 | 0 | 0 | 0 |
| 64 | 1.19e-07 | 0 | 0 | 0 |

Every normal is unit length, has **exactly zero** `y`, points **exactly** out
from the axis, and is shared by both ends of its segment. And they genuinely
differ from one another, which no earlier mesh's did:

```text
distinct normals at SEGMENTS=16: 16   (the grid and quad have 1; the cube has 6)
worst error in the angle between neighbouring normals: 5.66e-07 rad
  (the step is 0.3927 rad = 360/16 degrees)
```

### 4. Winding, and the area converging on a true tube

| segments | outward? | agrees with stored normal? | surface area | % of a true tube (pi) |
|---:|---|---|---:|---:|
| 3 | ok | ok | 2.598076 | 82.70% |
| 6 | ok | ok | 3.000000 | 95.49% |
| 16 | ok | ok | 3.121445 | 99.36% |
| 64 | ok | ok | 3.140331 | 99.96% |

Every triangle at every segment count faces outward and agrees with its stored
normal. The area column is the nicest number in the phase: the flat strips always
cut *inside* the true circle, so the mesh's area is always less than the real
`2 pi r h = pi`, and it closes the gap as segments rise. At 3 segments the "tube"
is a triangular prism with 83% of the area; at 64 it is within 0.04%.

### 5. The seam is shared, not duplicated

```text
triangles per vertex: 32 vertices use 3
distinct positions: 32 of 32 vertices
```

Every vertex serves exactly three triangles - a perfectly regular tube - and no
two vertices share a position. The ring closes with a modulo instead of a
duplicated last column.

### 6. The checkpoint: the normals sweep smoothly round the circumference

The strongest form of this test is to walk one row of pixels straight across the
tube in the `N` debug view and watch the **red** channel, which encodes
`normal.x`:

```text
one row across the middle of the tube: 119 pixels wide
  red runs 16 -> 247, spanning 16..254
  times it went backwards: 3      biggest jump between neighbours: 10
distinct normal colours: 6 segments -> 118,  16 -> 119,  64 -> 119
```

Red climbs essentially monotonically from one edge to the other across nearly the
whole `0..255` range, in steps of at most 10 levels. That is a smooth sweep, and
it is exactly what the maths predicts: for a tube seen side-on, `normal.x` goes
from `-1` on one edge, through `0` at the middle, to `+1` on the other.

For contrast, the grid and the quad each show **exactly one** flat colour in this
view. This is the first mesh in the project whose normals vary at all.

### 7. It is an open tube - and the hole is measurable

| camera | culling ON | culling OFF | difference |
|---|---:|---:|---:|
| level with the tube's middle | 28 451 px | 28 451 px | 0 |
| below, looking up into it | 13 378 px | 18 517 px | **5 139** |

Seen edge-on the openings vanish, because you are looking along the rim. Looking
up into it, switching culling off **fills in 5 139 pixels** - 27.8% of the shape's
area from that angle. Those are the pixels you were seeing *straight through the
tube* before. That is the missing lid, measured, and it is Phase 21's job.

### 8. The viva change, and an honest caveat

| segments | silhouette | faceting error in the sweep |
|---:|---:|---:|
| 6 | 24 016 px | **+3.13** levels |
| 16 | 24 375 px | +0.72 levels |
| 64 | 24 395 px | 0 (baseline) |

Raising segments grows the silhouette, because flat strips cut inside the true
circle - but only by **1.58%** from 6 to 64, which is too small to be a good
demonstration on its own. Worse, a hexagon's width depends on which way it
happens to be turned, so a single still frame is a poor way to judge faceting.
The honest places to look are the **wireframe**, the **area** column in section 4,
and the sweep-error column above.

## What changed

| File | Change |
|---|---|
| `src/Mesh.h` | `#include <cmath>`, `TWO_PI`, **`buildCylinderGeometry()` and `makeCylinder()`** |
| `src/main.cpp` | New `CylinderConfig`: `SEGMENTS`, `DIAMETER`, `HEIGHT`, `POSITION`, `BOTTOM_COLOR`, `TOP_COLOR` |
| `src/main.cpp` | `SceneState::cylinderFrame`; built in `updateScene()` as a translation only |
| `src/main.cpp` | `createMeshes()`, `renderScene()` and `main()` take a `cylinderMesh` |
| `shaders/*` | Unchanged |

## The one idea

Every mesh so far has been made of flat pieces whose normals were obvious. A
cube's face points along an axis. A quad points at the camera. A grid points
straight up. In all three, **every vertex of a face shares one normal**, and the
`N` view shows a flat colour.

A cylinder is the first shape where that is not true. Its surface curves, so the
normal is different at every point around it - and the mesh has to say so.

```text
cube     6 distinct normals, one per face
quad     1
grid     1
cylinder 16 at SEGMENTS=16 - one per segment, sweeping right round
```

The shape is still made of flat triangles. Nothing in OpenGL is ever curved. What
makes it *look* curved is that the normals pretend it is.

## The analytic normal

```cpp
const float x = std::sin(angle) * UNIT_HALF_EXTENT;
const float z = std::cos(angle) * UNIT_HALF_EXTENT;
const glm::vec3 normal = glm::normalize(glm::vec3(x, 0.0f, z));
```

For a cylinder standing on the Y axis, the surface normal points straight out
from the axis. It therefore has **no `y` component at all** - verified as exactly
zero - and is simply the point's own `(x, z)` direction made unit length.

**"Analytic" means it comes from knowing what the shape is**, not from measuring
the triangles that approximate it. It is the exact normal of a perfectly smooth
cylinder, handed to a mesh made of flat strips. That mismatch is deliberate and it
is the entire trick: the lighting will behave as though the surface really were
round, even at 6 segments.

The other approach is averaging the faces that meet at a vertex, which is
Phase 23's `computeSmoothNormals()`. That one is for shapes whose normals cannot
just be written down. Where you *can* write them down, as here, analytic is
better: it is exact, it costs nothing, and it does not depend on the
tessellation.

Both vertices of a segment share the normal, because a cylinder's normal does not
depend on height - also verified as exactly equal.

## Why there is no `+ 1`, unlike `makeGrid`

This is the detail most worth noticing, because it looks like an inconsistency
and is not.

```text
makeGrid(N)        ->  (N+1) x (N+1) vertices     a row of posts has two ENDS
makeCylinder(N)    ->  2 x N vertices             a ring has no ends
```

A grid's row of `N` cells needs `N + 1` fence posts along it. A ring of `N`
segments closes on itself, so segment `N - 1` joins straight back to segment `0`:

```cpp
const int next = (i + 1) % segments;
```

That modulo is what closes the tube, and it means the seam vertices are **shared**
rather than duplicated - confirmed above: 32 vertices, 32 distinct positions, every
one used by exactly 3 triangles.

Sharing is correct here because the normal at angle `0` and at angle `2 pi` really
are the same direction. **A textured cylinder would have to duplicate the seam**,
because the texture coordinate there jumps from 1 back to 0 and a single vertex
cannot hold both. This project has no textures, so sharing is both correct and
cheaper.

## Reading the loops

### The vertices: two rings at once

```cpp
for (int i = 0; i < segments; ++i) {
    const float angle = TWO_PI * i / segments;
    ...
    vertices.push_back({ { x, -UNIT_HALF_EXTENT, z }, normal, bottomColor });
    vertices.push_back({ { x,  UNIT_HALF_EXTENT, z }, normal, topColor });
}
```

Both rings are built in one pass, bottom then top, so vertex `2*i` is the bottom
of segment `i` and `2*i + 1` is its top. One multiplication by 2 and the index
loop needs no lookup table.

`sin` for `x` and `cos` for `z` means angle `0` starts on the `+Z` axis and the
angle advances toward `+X`. That is not arbitrary - it is the direction that makes
the winding below come out facing outward.

### The indices: one quad per segment

```text
 t0 --- t1        b0 = 2*i          t0 = 2*i + 1
 |  \    |        b1 = 2*next       t1 = 2*next + 1
 |    \  |
 b0 --- b1
```

`b0 -> b1 -> t1` and `b0 -> t1 -> t0`, both counter-clockwise as seen from
**outside** the tube. Verified against the stored normals at 3, 6, 16 and 64
segments: every triangle faces outward.

## Why it is open, and why that is on purpose

The plan gives the end caps their own phase (21), and leaving the tube open until
then is a feature rather than an omission. A cap needs a different technique - a
centre vertex and a triangle fan - and completely different normals: `(0, +1, 0)`
and `(0, -1, 0)`, flat, where the wall's sweep smoothly round.

The cylinder stands at `y = +0.95` while the camera sits at eye level `y = 0`, so
the default view looks slightly **up into the open bottom**. The far wall's inside
faces away from you and is culled, so you see straight through to the background -
measured above as 27.8% of the shape's area from below.

That is the clearest possible argument for the caps, and you can see it without
touching a key.

## `SEGMENTS`, `DIAMETER` and `HEIGHT` are three different things

```cpp
constexpr int   SEGMENTS = 16;      // how finely the circle is divided
constexpr float DIAMETER = 0.5f;    // how wide it is
constexpr float HEIGHT   = 0.9f;    // how tall it is
```

`SEGMENTS` is the same kind of value as `GridConfig::CELLS`: it changes division,
not size. Checked for every segment count from 3 to 64 - all produce the identical
1 x 1 x 1 footprint.

The draw-time scale is where the other two live, and the order of the three
factors matters:

| mesh | scale | why |
|---|---|---|
| quad | `(SIZE, SIZE, 1)` | flat in `z`, so `z` takes the 1 |
| grid | `(SIZE, 1, SIZE)` | flat in `y`, so `y` takes the 1 |
| cylinder | `(DIAMETER, HEIGHT, DIAMETER)` | round in `x` and `z`, tall in `y` |

Each mesh's own flat or round directions decide which factor is which. Getting it
wrong turns a tube into a disc.

## The scene, counted

| | Phase 19 | Phase 20 |
|---|---:|---:|
| Meshes uploaded | 4 | **5** |
| Generated meshes | 3 | **4** |
| Vertices on the GPU | 112 | **144** |
| Draw calls | 7 | **8** |
| Triangles drawn per frame | 168 | **200** |

## Where each job happens

| Function | Job in this phase |
|---|---|
| `buildCylinderGeometry()` | Builds `2*segments` vertices and `6*segments` indices for a 1 x 1 x 1 open tube, with analytic normals. No OpenGL |
| `makeCylinder()` | Builds it and uploads it once, at startup |
| `createMeshes()` | Calls `makeCylinder()` with `CylinderConfig`'s values |
| `updateScene()` | Builds `cylinderFrame` - a translation, nothing else |
| `renderScene()` | Draws it last, with `scale(DIAMETER, HEIGHT, DIAMETER)` applied at draw time |

## Likely teacher questions

### What makes this the first "curved" surface?

It is the first mesh whose normals differ from vertex to vertex. A cube has 6
distinct normals and the quad and grid have 1 each; this tube has one per
segment, sweeping right round. The triangles are still flat - nothing in OpenGL
is ever curved.

### What is an analytic normal?

A normal written down from knowing what the shape is, rather than measured from
the triangles that approximate it. For a cylinder on the Y axis it points
straight out from the axis, so it is `normalize(vec3(x, 0, z))` - no `y`
component at all.

### Why is that better than averaging the faces?

It is exact, it costs nothing, and it does not depend on how finely the mesh is
divided. Averaging (Phase 23's `computeSmoothNormals()`) is for shapes whose
normals cannot simply be written down.

### Why do both ends of a segment share one normal?

Because a cylinder's normal does not depend on height - only on which way round
the circle you are. Verified as exactly equal.

### Why `2 * segments` vertices and not `2 * (segments + 1)`?

Because a ring has no ends. A grid's row of cells needs `N + 1` posts because it
stops at both sides; a ring closes on itself, so the last segment joins back to
the first with a modulo and the seam vertices are shared.

### When would you have to duplicate the seam?

With textures. The texture coordinate jumps from 1 back to 0 at the seam and one
vertex cannot hold both values. This project has no textures, so sharing is
correct.

### How do you know the winding is right?

Every triangle's geometric normal was computed and compared against its stored
normal at 3, 6, 16 and 64 segments - all agree and all point outward. On screen,
the tube is solid from outside and see-through where a lid is missing.

### Why is the tube open?

The caps need a different technique and different normals - a centre vertex, a
triangle fan, and a flat `(0, +/-1, 0)` - so they get Phase 21. Leaving it open
means you can see straight up into it, which is the plainest argument for why
they are needed: 27.8% of the shape's area from below is see-through.

### How would you show the surface is only an approximation?

The area. At 3 segments the mesh has 82.70% of a true tube's `2 pi r h`; at 6,
95.49%; at 16, 99.36%; at 64, 99.96%. The flat strips always cut inside the
circle, and raising segments closes the gap.

### Does a low segment count show up in the normals view?

Barely, and that is worth understanding. Even at 6 segments the sweep looks
near-continuous (118 distinct colours across a 119-pixel row), because the normal
is **interpolated** across each flat strip. Measured against the ideal, 6 segments
is only about 3 levels out of 255 wrong. Faceting shows in the silhouette and the
wireframe, not here - and a value interpolated across a face looking smoother than
the face really is, is exactly the Gouraud-shading effect of Phase 31.

### Why is the scale `(DIAMETER, HEIGHT, DIAMETER)`?

Because the tube is round in `x` and `z` and tall in `y`. Compare the quad's
`(SIZE, SIZE, 1)` and the grid's `(SIZE, 1, SIZE)`: each mesh's own flat or round
directions decide which factor goes where.

### What happens if you ask for 2 segments?

It clamps to 3. Two segments would be a flat flap and one would be nothing; three
is the smallest ring that encloses any space. Checked for 2, 1, 0 and -5 - all
produce a valid 3-segment tube.

## Simple viva modifications

- **The named change: change the segment count.** Edit `CylinderConfig::SEGMENTS`.
  The startup line's numbers all change together and still match `2 x SEGMENTS`.
- **Make it obviously faceted:** set it to `6` and look at the silhouette and the
  wireframe. Note that the `N` view still looks smooth, and say why.
- **Make it a triangular prism:** set it to `3`, the minimum. Its surface area is
  only 83% of a true tube's.
- **Prove the clamp:** set it to `0`. You still get a valid 3-segment tube.
- **Resize instead of redivide:** edit `DIAMETER` or `HEIGHT`. Same triangle count,
  different shape - the opposite of the above.
- **See the missing lid:** drag the mouse DOWN to drop the camera below the tube
  and look up into it. You can see straight through. Press `W` and the far wall's
  outline appears.
- **Watch the sweep:** press `N` and orbit slowly. The colours rotate round the
  tube because the normals do.
- **Break the axis on purpose:** change the normal to `normalize(vec3(x, 1, z))`.
  Nothing changes in the normal picture, but the `N` view tilts its colours -
  proof the `y` component really is supposed to be zero.
- **Squash it into a disc:** change the scale to `(DIAMETER, DIAMETER, DIAMETER)`
  and then to `(HEIGHT, DIAMETER, HEIGHT)`, and explain which factor is which.

## Checkpoint

Phase 20 passes when:

- Debug and Release builds succeed with no compiler warnings;
- both shaders compile and link, with no missing-uniform warning;
- an **open tube** stands on the left of the scene, clear of everything else;
- the startup output reports `cylinder vertices=32 indices=96 triangles=32` at
  `SEGMENTS = 16`, and follows `2 x SEGMENTS` for any other value
  (**Phase 21 changed these to 66 / 192 / 64** when it added the end caps - check
  against Phase 21's checkpoint, not this one, if you are running current code);
- with `N` pressed, the colours **sweep smoothly round its circumference** rather
  than showing one flat value, which is the thing no earlier mesh could do;
- looking up into it from below, you can see straight through the missing lid;
- changing `SEGMENTS` changes the counts while the tube keeps the same diameter
  and height;
- the grid, triangles, quad, cubes and the `N`, `D`, `W` and `O` keys all still
  behave as before;
- the window remains responsive, reports frame timing, and closes cleanly with
  every GPU object freed.

## What is not part of Phase 20

**No end caps.** That is Phase 21: a centre vertex and a triangle fan at each
end, with flat `(0, +1, 0)` and `(0, -1, 0)` normals. No sphere (Phase 22). No
smooth-normal averaging (Phase 23). No runtime change of `SEGMENTS` - the `+` and
`-` keys are Phase 25. No lighting, so the analytic normals are only being
displayed, not used in a calculation yet.

Phase 21 closes the tube.

---

## Note: the audit of earlier phases

The Phase 19 pulse fix made the test triangle **2.24x larger** at its peak, from
`0.58` to `1.30`. A larger object can reach things it could not before, so the
whole scene was re-checked against it before adding anything new.

### Confirmed good

```text
scale factor spans 0.500000 .. 1.300000, never zero or negative     OK
triangle y range over all time: -1.441 .. +1.441
stays inside the default view (which covers y +-1.657)              OK
gap to the quad          : +0.4510                                  OK
gap to cube 1            : +0.8491                                  OK
gap to cube 2            : +0.0977                                  OK
```

### The grid floor: a false alarm I had to chase down

A first pass using bounding **spheres** reported the triangle dipping `0.0412`
below the grid. That would have been a real defect - an object poking through the
floor. An exact test using the triangle's actual rotated vertices found:

```text
instants with a vertex below the floor: 0  (of 1 006 783 instants sampled over 400 s)
verdict: clear
```

The sphere test was pessimistic: its radius assumes the farthest vertex points
straight down, which only happens at particular spin angles - and at those angles
the slide is not at its lowest. **No change was needed**, and it is worth
recording that the conservative test would have led to an unnecessary edit to
`GridConfig::POSITION`.

### Cube 0: a real overlap, deliberately left alone

```text
cube 0 (scale 0.75 at +1.10,-0.50): 16 310 overlapping instants
                                    (0.81% of the time, first at t = 1.33 s)
cube 1, cube 2                    : 0
```

This is an exact separating-axis test, so it is real: at its largest the spinning
triangle's corner genuinely passes through the big cube, about 0.8% of the time.
Before the pulse fix it could not reach that far.

**I have not changed it**, for these reasons:

- It renders **correctly**. The depth buffer resolves it per pixel, and what you
  see is two shapes genuinely intersecting - which is what intersecting shapes
  look like.
- The triangle is explicitly the temporary Phase 2 test object, and it already
  sweeps `+/-1.5` in depth, passing in front of and behind everything on screen
  constantly.
- The cost of "fixing" it is high and the benefit is cosmetic. Cube positions
  carry measured pixel evidence in three phase documents (Phase 16's
  `216 x 260 px` silhouettes and spacing margins, Phase 17's and Phase 18's
  comparisons). Moving one would invalidate those numbers, and a stale number in
  a document is a worse defect than a brief interpenetration.

If you would rather the shapes never touched, the smallest change is to move
cube 0 further right - say `{ 1.35f, -0.50f, 0.0f }` - and I would then re-measure
and update the three documents that quote its position.

---

## Note: a flaw in my own verification

While finishing this phase the strict-warning build suddenly failed to configure.
The cause turned out to be in the **way I had been running it**, not in the
project:

```text
CL.exe ... /permissive- ... C:/Program Files/Git/W4 testCXXCompiler.cxx
                           ^^^^^^^^^^^^^^^^^^^^^^^^
```

Git Bash (MSYS) rewrites an argument that looks like a Unix path, so
`-DCMAKE_CXX_FLAGS="/W4 ..."` had `/W4` converted into `C:/Program Files/Git/W4`
and handed to the compiler as a **source file** instead of a warning flag.

What makes this worth writing down is how it hid. When the flag was mangled,
CMake's configure step failed, and the command I used was shaped like this:

```bash
cmake -S . -B dir -DCMAKE_CXX_FLAGS="/W4 ..." > /dev/null 2>&1 && \
  cmake --build dir --config Release 2>&1 | grep -E "error|warning C"
```

A failed configure short-circuits the `&&`, so the build never runs, so `grep`
prints nothing - **which looks exactly like a clean build**. "No warnings
found" and "no build happened" were indistinguishable under that filter.

So the `/W4` results reported in some earlier phases were unreliable. The Debug
and Release results were never affected (no `/W4` argument to mangle, and those
builds produced executables that were then run), and neither were any of the
geometry or pixel measurements, which were separate programs.

**Resolved.** The strict build was re-run from PowerShell, which does not rewrite
arguments, and checked three ways - the cache value, the generated project's
`WarningLevel`, and the real `cl` command line - before trusting it. The code is
clean at `/W4 /permissive-`, and because the source is cumulative that covers
everything added in Phases 14 to 20.

Two lessons, both general:

- **A test that cannot fail is worse than no test.** Always check for the
  positive artefact - here, that the executable exists and that the flag reached
  the compiler - not merely for the absence of complaints.
- **Never silence the step before the step you are measuring.** The `> /dev/null`
  on the configure line is what made the failure invisible.
