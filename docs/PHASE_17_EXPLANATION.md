# Phase 17 - `makeQuad()`: the Simplest Generator

## Status

Phase 17 is complete and verified. Debug and Release builds succeeded with no
compiler warnings, both shaders linked with no missing-uniform warning, and a
live Release run drew the same scene as Phase 16. Nothing was printed to the
error output, the loop held about `120 FPS`, and the program closed cleanly.

Like Phase 14, this is a **refactor**: the quad was hand-written data, and now
it is generated. The picture is meant not to change, so the most useful thing to
measure is whether it did. It did not - not by a single pixel.

### 1. The picture is byte-for-byte identical to Phase 16

The Phase 16 sources and the Phase 17 sources were both built, both had their
clock pinned to `t = 2.0 s`, and both had their finished frame read back from the
framebuffer with `glReadPixels`. Both ran on the same GPU (the NVIDIA RTX 5050,
confirmed from the `Renderer` line each program printed).

```text
whole frame (1280 x 720)           : 0 of 2 764 800 colour bytes differ
quad on its own                    : 174 x 174 px at (162, 273), 30 276 px - in BOTH
quad's colour bytes                : 0 differ
triangles + cubes, quad hidden     : 0 differ
wireframe, front view              : 0 differ (866 px of edges, diagonal included)
wireframe, seen from behind        : 0 differ
```

I had predicted "within 1/255", because the GPU multiplies the matrices together
in a different order than it adds up the numbers, and that can nudge the last
bit. The measured result is better than that: **zero**. The generated quad,
scaled by `0.8`, lands every corner on exactly the float the hand-written quad
had.

### 2. The generator really produces Phase 9's quad

This was checked on the numbers, not just the pixels, by compiling the real
`Mesh.h` and comparing it against Phase 9's hand-written arrays, copied in
verbatim:

```text
4 vertices, 6 indices; every index below 4; every vertex used
extents x[-0.500,+0.500] y[-0.500,+0.500] z[0.000, 0.000]
the two triangles, scaled by 0.8, have BIT-IDENTICAL positions and colours
  to Phase 9's two triangles (compared as unordered vertex sets)
shared diagonal: (-0.4, +0.4) to (+0.4, -0.4)  - top-left to bottom-right,
  the SAME diagonal Phase 9 documented
```

### 3. The normals view confirms it faces `+Z` - the phase's checkpoint

With the Phase 15 `N` view on and everything else hidden, the frame contains
**exactly two colours**: the background and one flat quad colour,
`(127, 127, 255)` on this GPU. That is the `+Z` colour from Phase 15's table -
red and green at half, blue at full, light blue - and it covers exactly the
30 276 pixels the quad covers. No gradient, no second colour. It is also
identical to what the hand-written quad showed in the same view.

(`127` rather than `128` is the driver rounding `127.5`, covered in Phase 15's
document: predict the float values, and expect the half-grey channel to read as
either.)

### 4. Seen from behind, it disappears - which proves the winding agrees with the normal

A normal and a winding order are two separate pieces of data that are supposed
to agree. The normal says "I face `+Z`". The winding says it too, to
`GL_CULL_FACE`. Nothing in the program forces them to agree - which is exactly
the kind of mistake a generator can make and a picture cannot show. So both
were tested:

| Camera | Solid | Wireframe (culling off) |
|---|---:|---:|
| From the front (`yaw 0`) | 30 276 px | 866 px |
| From behind (`yaw 180`) | **0 px** | **866 px** |

From the front it is drawn. From behind, in solid mode, it is **gone** - culled,
exactly as a `+Z`-facing quad must be. With culling switched off, the outline
reappears, proving it was culled rather than missing. This is Phase 11's lesson
applied to a generator, and the numbers are identical for the hand-written quad
and the generated one.

The geometry agrees too: both triangles' computed normals (the cross product of
their edges) are `(0, 0, +1)`, the same as every stored normal, and each
triangle has exactly half the quad's area.

### 5. The viva change, performed and measured

The plan's modification is *change the quad's size*. `QuadConfig::SIZE` was
changed from `0.8` to `1.2`, one number, nothing else:

```text
SIZE 0.8 : 174 x 174 px, 30 276 px
SIZE 1.2 : 260 x 260 px, 67 600 px     width ratio 1.4943   (1.2 / 0.8 = 1.5)
                                       area  ratio 2.2328   (1.5 squared = 2.25)
with the quad hidden, SIZE 0.8 vs 1.2  : 0 differing bytes
every changed pixel in the whole frame : inside the larger quad's own box
```

The quad grew by one and a half times in both directions, to within a pixel
(`174 x 1.5 = 261`, measured `260`). The triangles and all three cubes are
byte-for-byte untouched, and every pixel that did change lies inside the larger
quad's footprint. The startup line still reads `vertices=4 indices=6
triangles=2` - the mesh was never touched, only the matrix.

### 6. The three "break it on purpose" exercises, actually performed

The viva exercises at the end of this document claim specific things will happen
when the generator is deliberately broken. Rather than trust those, each was
done and measured, on the same GPU:

| Deliberate mistake | Normal picture | `N` view | Seen from behind |
|---|---|---|---|
| **Wrong normal**: `(0, 0, -1)`, winding untouched | **0 bytes differ** from the correct quad - looks perfect | `(127, 127, 0)` olive instead of `(127, 127, 255)` light blue | n/a |
| **Reversed winding**: indices `0,3,1 / 1,3,2`, normal untouched | quad **gone**: 0 px | n/a | quad **appears**: 30 276 px |
| **The other diagonal**: the cube's `0,1,2 / 2,3,0` | 87 914 colour bytes differ, up to 165 out of 255 | n/a | n/a |

That table is the whole argument for this phase's checkpoint in three rows:

- A wrong **normal** is completely invisible in the normal picture - not "hard to
  spot", **zero** differing bytes - and obvious the instant `N` is pressed.
- A wrong **winding** is the opposite: it is not subtle at all, the quad simply
  vanishes, and the debug view is no help because the normal data is fine.
- The **diagonal** really does matter: the silhouette is the same 30 276 pixels,
  but the colours inside it change by as much as 165 levels.

## What changed

| File | Change |
|---|---|
| `src/Mesh.h` | **`buildQuadGeometry()` and `makeQuad()` added** - the second generator |
| `src/main.cpp` | `QuadConfig`'s hand-written `VERTICES`, `INDICES`, `FLAT_NORMAL`, `VERTEX_COUNT`, and `INDEX_COUNT` deleted |
| `src/main.cpp` | `QuadConfig::CORNER_COLORS[4]` and `QuadConfig::SIZE` added |
| `src/main.cpp` | `SceneState::quadModel` renamed `quadFrame` - a translation only, no scale |
| `src/main.cpp` | `createMeshes()` calls `makeQuad()`; `renderScene()` multiplies the frame by `QuadConfig::SIZE`'s scale at draw time |
| `shaders/*` | Unchanged |

`createMeshes()` also got shorter: the quad's two `std::vector` copies and its
`upload()` call became one line, `makeQuad(quadMesh, QuadConfig::CORNER_COLORS)`.

## The one idea

Phase 14's `makeCube()` introduced the generator idea, but a cube is a poor way
to learn it: six faces, a table of directions, a loop, and a clever rule about
`right x up = normal`. There is a lot going on at once. **A quad is the same
idea with nothing in the way.**

```text
makeCube   -> 6 faces x 4 corners = 24 vertices, 36 indices, a table, a loop
makeQuad   -> 4 corners           =  4 vertices,  6 indices, no table, no loop
```

The recipe a generator follows, small enough now to see whole:

1. **Corners** - where the vertices are, for a shape of size 1 centred on the origin.
2. **Normal and colour** - what each vertex also carries.
3. **Indices** - which corners make up which triangles.
4. **Upload** - hand all of it to `Mesh::upload()`.

Every generator from here to Phase 22 - grid, cylinder, sphere - is this recipe
with a more interesting step 1.

## The unit quad

```cpp
const glm::vec3 corners[4] = {
    { -UNIT_HALF_EXTENT, -UNIT_HALF_EXTENT, 0.0f },   // 0: bottom-left
    {  UNIT_HALF_EXTENT, -UNIT_HALF_EXTENT, 0.0f },   // 1: bottom-right
    {  UNIT_HALF_EXTENT,  UNIT_HALF_EXTENT, 0.0f },   // 2: top-right
    { -UNIT_HALF_EXTENT,  UNIT_HALF_EXTENT, 0.0f },   // 3: top-left
};
```

It follows the unit-mesh rule from Phase 16 exactly: **no size parameter**, one
unit across, centred on its own origin, and the same `UNIT_HALF_EXTENT` the cube
uses. A quad has no thickness, so `1 x 1` means width and height; its depth is
exactly `0`, and it lies in the XY plane.

The corners are walked **counter-clockwise from the bottom-left**, the same walk
the cube takes on each of its faces. That consistency is deliberate: it means
"which way round is the winding?" has one answer across the whole project.

## Why the quad is scaled by `(SIZE, SIZE, 1)` and not `(SIZE, SIZE, SIZE)`

```cpp
const glm::mat4 quadScale = glm::scale(
    glm::mat4(1.0f), glm::vec3(QuadConfig::SIZE, QuadConfig::SIZE, 1.0f));
drawMesh(shader, quadMesh, scene.quadFrame * quadScale, NO_TINT);
```

The `z` factor is `1`. That is partly honesty - a quad has no depth, so there is
nothing to scale - and it was checked that it makes no practical difference:
every vertex has `z = 0`, and zero times anything is still zero, so a quad scaled
by `(s, s, s)` stays exactly as flat as one scaled by `(s, s, 1)`.

Writing `1` says what is meant: *size this in width and height only*. It also
keeps the door open, because a flat quad facing `+Z` can safely be given separate
width and height. That was checked too - with the non-uniform scale
`(0.8, 1.7, 1)`, the normal matrix `(M^-1)^T` applied to `(0, 0, 1)` still gives
exactly `(0, 0, 1)`. The reason is that the normal lies along the one axis that
is **not** being scaled. This is why the quad is a safe place to see a
non-uniform scale before Phase 26's normal matrix - but it is not needed yet, so
`SIZE` is a single number.

## Why `SIZE` is `0.8`

Not a design choice - a promise. Phase 9 drew the quad with its corners at
`+-0.4`, which is `0.8` across. A unit quad has corners at `+-0.5`, so scaling by
`0.8` puts each corner at `0.5 x 0.8 = 0.4` - exactly where it already was. (And
`0.4` is the float exactly half of `0.8`, because halving is exact in binary,
which is why the comparison above found zero differences rather than nearly
zero.)

If the size had been `1.0`, the quad would have grown by a quarter and the
refactor would have changed the picture. Choosing `0.8` is what makes this phase
a pure refactor.

## Which way it faces

Two facts are written down once, in `buildQuadGeometry()`:

| Fact | Where | Who reads it |
|---|---|---|
| **Normal** = `(0, 0, 1)` on all four vertices | the `Vertex` data | the Phase 15 debug view, and every lighting phase from 27 |
| **Winding** = counter-clockwise as seen from `+Z` | the order of the indices | `GL_CULL_FACE` |

They are independent. You can get one right and the other wrong, and the quad
will still draw. That is the reason both are tested - the `N` view reads the
first, and looking at the quad from behind reads the second:

- **Wrong normal, right winding** (for example, normal `(0, 0, -1)`): the quad
  looks exactly as before, but the `N` view paints it olive instead of light
  blue. The picture alone would never have shown it.
- **Right normal, wrong winding** (for example, reversed indices): the `N` view
  would look right in theory, but the quad would vanish from the front and
  appear from behind.

## Why the indices are `{0,1,3, 1,2,3}` and not the cube's `{0,1,2, 2,3,0}`

This is the one place the quad does not copy the cube, and it is on purpose.

A square can be split into two triangles along either of its two diagonals:

```text
the cube's pattern (0,1,2 / 2,3,0)        the quad's pattern (0,1,3 / 1,2,3)
splits bottom-left to top-right           splits bottom-right to top-left

 3 ------- 2                               3 ------- 2
 |       / |                               | \       |
 |     /   |                               |   \     |
 |   /     |                               |     \   |
 | /       |                               |       \ |
 0 ------- 1                               0 ------- 1
```

The four corner colours are different, so the diagonal matters: each triangle
blends only its **own** three corner colours, so cutting along the other diagonal
makes a visibly different gradient. The quad has been cut bottom-right to
top-left since Phase 9, and Phase 9's document describes exactly that diagonal.
Using the cube's pattern would have changed the picture.

The pixel comparison confirms the choice: the wireframe is identical, with the
same diagonal in the same place, and so is every coloured pixel.

(The cube's faces all have a single flat colour per face, so which diagonal they
use makes no visible difference, which is why the cube was free to use the
simpler pattern.)

## The corner colours moved, but not their meaning

Phase 9 listed the corners top-left, top-right, bottom-right, bottom-left - a
**clockwise** walk, which is why its indices had to be the odd-looking
`{0, 3, 2, 2, 1, 0}`. A generator walks counter-clockwise from the bottom-left
like the cube, so `QuadConfig::CORNER_COLORS` is in that order instead:

| Index | Corner | Colour |
|---:|---|---|
| 0 | bottom-left | yellow `(1.0, 1.0, 0.9)` |
| 1 | bottom-right | blue `(0.1, 0.7, 1.0)` |
| 2 | top-right | green `(0.5, 1.0, 0.0)` |
| 3 | top-left | red `(1.0, 0.0, 0.0)` |

The same four colours on the same four corners as always. Only the order in which
they are listed changed, and it was checked that each colour really did land on
the corner it names.

## A frame, not a model matrix

`SceneState::quadModel` was renamed `quadFrame`, and its `updateScene()` line
gained a comment, but its value did not change: it is still just a translation to
`QuadConfig::POSITION`. The rename is about what the matrix *is*. Phase 16
established that a stored matrix is a **frame** - a place and a rotation, never a
scale - and that size is multiplied on at the last moment inside the draw. The
quad now follows that rule too, so both generated meshes are drawn the same way.

The triangle is the only object that does not: its model matrix still contains
Phase 6's pulsing scale, which is that phase's demonstration and has no children
to inherit it, as Phase 16 explained.

## What is gone, and where to find it

Phase 9's viva change was *change one index and see the tear*. The indices no
longer live in `main.cpp`, so the same exercise now happens inside
`buildQuadGeometry()` in `src/Mesh.h`: change one of the six `indices.push_back`
values and half the quad vanishes, exactly as before.

| Deleted from `main.cpp` | Now |
|---|---|
| `QuadConfig::VERTICES` (4 rows of position, normal, colour) | `corners[4]` in `buildQuadGeometry()`, plus `CORNER_COLORS` |
| `QuadConfig::INDICES` (6 numbers) | six `indices.push_back` calls in `buildQuadGeometry()` |
| `QuadConfig::FLAT_NORMAL` | `normal` in `buildQuadGeometry()` |
| `QuadConfig::VERTEX_COUNT`, `INDEX_COUNT` | the vectors know their own size |

## The scene, counted

| | Phase 16 | Phase 17 |
|---|---:|---:|
| Meshes uploaded | 3 | 3 |
| Hand-written meshes | 2 (triangle, quad) | **1** (triangle) |
| Generated meshes | 1 (cube) | **2** (cube, quad) |
| Draw calls | 6 | 6 |
| Triangles drawn per frame | 40 | 40 |
| Lines in `main.cpp` | 1019 | 1029 |

`main.cpp` grew by ten lines, which is worth explaining rather than hiding: the
data it lost was small (a handful of rows), and the comments explaining the new
rule, the new colours, and the new size are longer than the rows they replaced.
`Mesh.h` grew by 72 lines for the generator. Nothing about this phase is a
saving of code; it is a saving of *places to make a mistake*, and the first step
toward the quad becoming a real, reusable shape in Phase 18's grid.

## Where each job happens

| Function | Job in this phase |
|---|---|
| `buildQuadGeometry()` | Builds 4 vertices and 6 indices for a 1 x 1 quad facing `+Z`. No OpenGL |
| `makeQuad()` | Builds the quad and uploads it, once, at startup |
| `createMeshes()` | Calls `makeQuad()` with `QuadConfig::CORNER_COLORS` |
| `updateScene()` | Builds `quadFrame` - a translation, nothing else |
| `renderScene()` | Multiplies the frame by `scale(SIZE, SIZE, 1)` and calls `drawMesh` |

## Likely teacher questions

### What does `makeQuad()` do that Phase 9's hand-written quad did not?

Nothing visible - the picture is byte-for-byte identical. It produces the same
four vertices and six indices from a short recipe instead of a typed-out table,
as a unit mesh, so its size becomes a `glm::scale` at draw time like every other
object's.

### Why is the quad a unit quad, 1 x 1?

The unit-mesh rule from Phase 16: a generator makes a shape of size 1 centred on
its own origin and takes no size parameter, so one mesh can be drawn at any size.
A quad's `1 x 1` is its width and height; it has no depth.

### Which way does the quad face, and how do you know?

`+Z`, straight out of the screen. Every vertex stores the normal `(0, 0, 1)`, and
the Phase 15 debug view paints it the `+Z` colour, light blue, with nothing else
in the frame. The winding agrees: from behind, the quad is culled and vanishes.

### What is the difference between the normal and the winding?

The normal is data stored on each vertex, read by the debug view and, later, by
lighting. The winding is the order the corners are listed in, read by
`GL_CULL_FACE` to decide which side is the front. They are independent - a
generator can get one right and the other wrong - so both are checked.

### Why is the scale `(SIZE, SIZE, 1)` instead of `(SIZE, SIZE, SIZE)`?

A quad has no depth, so `z` is `0` and scaling it changes nothing; writing `1`
just says what is meant. It also means width and height could later be given
separate values safely, because the normal lies along the untouched axis.

### Why is `SIZE` exactly `0.8`?

Phase 9's quad had corners at `+-0.4`, which is `0.8` across. A unit quad has
corners at `+-0.5`, and `0.5 x 0.8 = 0.4`, so a scale of `0.8` puts every corner
exactly where it already was and the refactor changes nothing on screen.

### Why are the indices `{0,1,3, 1,2,3}` instead of the cube's pattern?

The two patterns cut the square along different diagonals, and with four
different corner colours that changes how the colours blend. The quad has been
cut from bottom-right to top-left since Phase 9, so its indices keep that
diagonal and its picture stays the same.

### Why did the corner order change from Phase 9?

Phase 9 listed the corners clockwise and fixed the winding with unusual indices.
A generator walks counter-clockwise from the bottom-left, as the cube does, so
the winding has one rule across the whole project. The colours are the same four
on the same four corners, only listed in the new order.

### How would you prove the generated quad is the same as the old one?

By measurement: the generated vertices, scaled by `0.8`, have bit-identical
positions and colours to Phase 9's as sets, and the rendered frames from the two
versions have zero differing bytes, in solid, wireframe, and normals views.

### What happens if you give the quad a wrong normal?

Nothing you can see. It still draws and still looks the same, because nothing
reads the normal until lighting. The `N` view shows it at once - the quad turns a
different colour - which is why Phase 15 put that view in before any lighting.

### What happens if you reverse the winding?

The quad vanishes when seen from the front and appears when seen from behind,
because `GL_CULL_FACE` now treats the old back as the front. Wireframe with
culling off shows it is still there.

### Where did Phase 9's "change one index" exercise go?

Into `buildQuadGeometry()` in `src/Mesh.h`. Change any of the six index values
there and half the quad vanishes exactly as it did before.

## Simple viva modifications

- **The named change: resize the quad.** Edit `QuadConfig::SIZE`. Point out that
  the startup line still says `vertices=4 indices=6 triangles=2` and that the
  triangles and cubes are untouched.
- **Make it wider than it is tall:** change the scale in `renderScene()` to
  `glm::vec3(QuadConfig::SIZE * 2.0f, QuadConfig::SIZE, 1.0f)`. It stretches
  sideways, and the `N` view still shows the same light blue. Two separate
  reasons, worth keeping apart: right now the shader never transforms the
  normal at all (`vNormal = aNormal`), so no scale could change what `N` shows;
  and even once Phase 26's normal matrix is applied, a quad's `(0, 0, 1)` normal
  survives a stretch in its own plane, because it lies along the untouched axis.
- **Recolour one corner:** edit a row of `CORNER_COLORS`. Corner 3 is top-left.
- **Break the normal on purpose:** change `normal` in `buildQuadGeometry()` to
  `(0, 0, -1)`. Nothing changes in the normal picture. Press `N`: the quad turns
  olive. Put it back.
- **Break the winding on purpose:** swap two numbers inside one triangle's
  indices. Half the quad vanishes. Press `W`: the missing half's outline is
  still there.
- **Look at it from behind:** orbit the camera around to the far side. The quad
  disappears. Press `W` and it comes back as an outline.
- **Tear it:** change one of the six `indices.push_back` values to a repeated
  corner. A zero-area triangle is drawn and half the quad is missing, with no
  error message - Phase 9's lesson, in its new home.
- **Cut the other diagonal:** replace the indices with the cube's pattern
  `0,1,2, 2,3,0`. The picture changes - the colours now blend along the opposite
  diagonal - which is the proof that the diagonal matters.

## Checkpoint

Phase 17 passes when:

- Debug and Release builds succeed with no compiler warnings;
- both shaders compile and link, with no missing-uniform warning;
- the picture is **identical to Phase 16's**: the same quad, in the same place,
  at the same size, with the same blended corner colours;
- the startup output still reports `quad vertices=4 indices=6 triangles=2`;
- `main.cpp` holds no vertex or index data for the quad;
- with `N` pressed, the quad is one flat light blue - the `+Z` colour - matching
  Phase 15's table;
- seen from behind, the quad is not drawn in solid mode and is drawn in wireframe;
- changing `QuadConfig::SIZE` resizes the quad and nothing else;
- the `N`, `D`, `W`, and `O` keys and the mouse orbit and zoom all still work;
- the window remains responsive, reports frame timing, and closes cleanly.

## What is not part of Phase 17

No grid: `makeGrid(N)`, the first parameterised generator, is Phase 18. The quad
takes no parameters at all - it is always 4 vertices - which is exactly why it
comes first. No new files and no shader changes. No non-uniform scale in the
running program, and no normal matrix; that pairing is Phase 26. No lighting.

The triangle is still hand-written, and there is no plan to generate it: it is
Phase 2's temporary pipeline test, kept because the earlier phases' demonstrations
(`O`, `D`, the pulse) are built on it.

**Still deliberately unfixed.** The Stage A review's finding on the Phase 6 pulse
maths (`* 0.1f` where `* 0.5f` was meant, in `src/main.cpp`) is untouched for a
fourth phase, so that this phase's frame-by-frame comparison against Phase 16
could be exact. It was exact. That finding is now the oldest open item in the
project, and is worth fixing as its own small change before Phase 18.

Phase 18 adds `makeGrid(N)`: the first **parameterised** generator, built with
nested loops, a flat surface that is `N` quads wide and `N` quads deep.
