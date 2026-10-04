# Phase 23 - `computeSmoothNormals()`: the Other Way of Deciding a Normal

## Status

Phase 23 is complete and verified. Debug and Release builds succeeded with no
compiler warnings, both shaders linked with no missing-uniform warning, and a
live Release run showed a second cube beside the first. Nothing was printed to
the error output, and the program closed cleanly.

This is a graded milestone (`*` in the plan). It adds **no new shape** - Stage B's
five meshes were finished in Phase 22. It adds a second *method*.

```text
[mesh] cube       vertices=24 indices=36 triangles=12
[mesh] smoothcube vertices=8  indices=36 triangles=12
```

Same 12 triangles. A third of the vertices. Completely different in the `N` view.

### 1. THE CHECKPOINT: hard face colours against blended corners

Both cubes rendered alone, at the same scale, in the normals debug view:

| | flat cube (24 vertices) | smooth cube (8 vertices) |
|---|---:|---:|
| distinct colours | **3** | **15 952** |
| biggest single flat patch | 12 177 px = **69.5%** of it | 1 px = **0.0%** |
| silhouette | 17 516 px | 15 952 px |

The flat cube shows **three** colours - one per visible face - and a single one of
them covers 69.5% of the shape, because a whole face has one unchanging normal.

The smooth cube shows **15 952** distinct colours over 15 952 pixels. Every single
pixel is a different value, and the largest run of any one colour is one pixel.
There are no flat patches at all; it is gradient everywhere.

Three against fifteen thousand. That is "obviously different".

### 2. The geometry is identical - only the shading changed

```text
wireframe edge px: flat 1620, smooth 1607   (ratio 0.992)
the 12 triangles occupy the same 12 places in space: yes
surface area: exactly 6.000000 (a unit cube is 6.0)
```

The two cubes are the same shape, the same size, in the same rotation, side by
side. Verified by comparing the triangles as unordered sets of positions: the same
twelve triangles in the same twelve places. **The only difference is how their
normals were decided.**

### 3. THE VIVA QUESTION: why the flat 24-vertex cube cannot be smoothed

```text
smoothing the 24-vertex cube changes its normals by at most 0
most faces any one of its 24 vertices belongs to: 1
by contrast each of the 8 shared corners belongs to 3 faces
```

`computeSmoothNormals()` was run on the flat cube and **changed nothing at all** -
not approximately, exactly zero.

The reason is counted rather than asserted: each of the flat cube's 24 vertices
belongs to exactly **one** face. Averaging the faces that meet at a vertex, when
only one face meets there, gives that face's own normal back. There is nothing to
average. By contrast each of the 8 shared corners touches **three** faces, so
there are three normals to combine.

So "smoothing" is not something you do to a mesh. It is something that is only
*possible* if the mesh shares its vertices in the first place - and Phase 10 chose
not to, deliberately, so that each face could have its own flat colour.

### 4. `computeSmoothNormals()` really is the slide formula

```text
worst difference from an independent sum-then-normalize : 0
worst |normal| - 1                                      : 5.96e-08
running it a second time changes nothing: worst drift   : 0
```

The function's output was compared against the formula recomputed independently:
identical. Every result is unit length. And it is safe to run twice, because it
depends only on positions - it overwrites whatever normals were there rather than
accumulating on top of them.

### 5. The smooth cube is still a valid solid

```text
degenerate 0 | wound outward yes | surface area 6.000000
18 edges, 0 used once, 0 used more than twice
all 8 normals still point outward, away from the centre
```

## What changed

| File | Change |
|---|---|
| `src/Mesh.h` | **`computeSmoothNormals()`** - the L9 slide 20 formula |
| `src/Mesh.h` | **`buildSharedCubeGeometry()` and `makeSmoothCube()`** - a cube from 8 shared corners |
| `src/Mesh.h` | The `[mesh]` report's name column widened from 8 to 10, so `smoothcube` lines up |
| `src/main.cpp` | New `SmoothCubeConfig`: `SCALE`, `POSITION`, `BOTTOM_COLOR`, `TOP_COLOR` |
| `src/main.cpp` | `SceneState::smoothCubeFrame`, sharing the flat cubes' rotation |
| `src/main.cpp` | `createMeshes()`, `renderScene()` and `main()` take a `smoothCubeMesh`; Draw 10 added |
| `shaders/*` | Unchanged |

## The one idea

Every normal in the project so far has been **analytic** - written down because
the shape was known:

| mesh | normal | how it was known |
|---|---|---|
| cube | one of 6 axis directions | a table, per face |
| quad | `(0, 0, 1)` | a constant |
| grid | `(0, 1, 0)` | a constant |
| cylinder wall | `normalize(vec3(x, 0, z))` | derived from the axis |
| cylinder caps | `(0, +/-1, 0)` | a constant |
| sphere | `normalize(position)` | nothing to derive |

Analytic is always the better answer **when it is available**, because it is exact
and does not depend on how finely the mesh is divided.

It is not always available. For a shape with no formula - a hull hand-built from
arbitrary points, or a surface that has been deformed - the only information left
is the triangles themselves. So take the normal of every triangle that meets a
vertex and average them:

```text
N_v = (sum of N_i) / || sum of N_i ||          - L9 slide 20
```

```cpp
inline void computeSmoothNormals(std::vector<Vertex>& vertices,
                                const std::vector<unsigned int>& indices)
{
    for (Vertex& vert : vertices)
        vert.normal = glm::vec3(0.0f);

    for (std::size_t i = 0; i + 2 < indices.size(); i += 3) {
        const glm::vec3& a = vertices[indices[i]].position;
        const glm::vec3& b = vertices[indices[i + 1]].position;
        const glm::vec3& c = vertices[indices[i + 2]].position;

        const glm::vec3 faceNormal = glm::normalize(glm::cross(b - a, c - a));

        vertices[indices[i]].normal += faceNormal;          // accumulate sum N_i
        vertices[indices[i + 1]].normal += faceNormal;
        vertices[indices[i + 2]].normal += faceNormal;
    }

    for (Vertex& vert : vertices)
        vert.normal = glm::normalize(vert.normal);           // divide by || sum ||
}
```

**This function belongs in the report.** It is a direct implementation of a
lecture slide, and the `cross(b - a, c - a)` in the middle is exactly the
expression the winding checks in Phases 9 to 22 have been computing all along.

Dividing by the *length* of the sum is the same as dividing by the count and then
normalizing, which is why the slide writes it in the compact form.

## What the averaged corner normals actually are - a finding

The ideal normal at a cube's corner is its body diagonal, `(+/-1, +/-1, +/-1)`
normalized. The formula does **not** produce that:

| corner | triangles using it | averaged normal | ideal diagonal | angle off |
|---:|---:|---|---|---:|
| 0 | 5 | `(-0.667, -0.667, -0.333)` | `(-0.577, -0.577, -0.577)` | 15.79 deg |
| 1 | 4 | `(+0.408, -0.408, -0.816)` | `(+0.577, -0.577, -0.577)` | 19.47 deg |
| 2 | 5 | `(+0.667, +0.667, -0.333)` | `(+0.577, +0.577, -0.577)` | 15.79 deg |
| 4 | 4 | `(-0.408, -0.408, +0.816)` | `(-0.577, -0.577, +0.577)` | 19.47 deg |

Every corner is 15.8 to 19.5 degrees off the diagonal, and they are not even all
wrong by the same amount. Why?

**The formula weights by TRIANGLE, not by FACE.** Each face is two triangles, and
a quad split along a diagonal gives two of its corners two triangles and the other
two only one. So a corner's three faces do not contribute equally: corner 0 gets
the `-X` and `-Y` face normals twice each and `-Z` only once, which pulls it away
from the diagonal and toward those two faces.

And this is **not fixable** within the formula, because of a counting argument:

```text
12 triangles x 3 corners = 36 triangle-corner slots
36 / 8 corners = 4.5
```

4.5 is not an integer, so the 8 corners *cannot* all be used by the same number of
triangles. Measured: 4 corners are used by 5 triangles and 4 by 4
(`5x4 + 4x4 = 36`). Those two groups are the cube's two tetrahedral sets, which
is why the skew comes in exactly two sizes.

Summing per **unique face** instead of per triangle would give the exact body
diagonal. The slide's formula sums per triangle, and that is what is implemented,
because it is the form that works on any mesh without first having to work out
what its "faces" are. It is a good viva answer to know both.

## The shared cube cannot have per-face colours either

This is the Phase 10 lesson seen from the other side, and it is worth stating
plainly because it caught me while writing the config.

Phase 10 needed 24 vertices because it wanted one flat colour per face, and **a
vertex carries one colour**. The shared cube has the same problem with the same
cause: its 8 corners cannot give 6 faces 6 colours.

So the smooth cube is coloured by corner *height* instead - the four lower corners
one colour, the four upper another - and each side face shows a vertical gradient.
That limitation is not a shortcut; it is the same single fact (*one vertex, one
value*) that has shaped every mesh in Stage B:

| | needs separate vertices? | why |
|---|---|---|
| cube faces (Phase 10) | yes | different colour AND different normal per face |
| cylinder cap rim (Phase 21) | yes | wall normal sideways, cap normal axial |
| sphere pole (Phase 22) | no | every triangle there agrees the normal is `(0, 1, 0)` |
| smooth cube corner (Phase 23) | no - sharing is the point | averaging needs them shared |

## Why the two cubes share one rotation

```cpp
scene.smoothCubeFrame =
    glm::translate(glm::mat4(1.0f), SmoothCubeConfig::POSITION) * cubeSpin;
```

`cubeSpin` is the same matrix the three flat cubes use. The smooth cube turns in
perfect step with them, so at every instant the two cubes differ **only** in their
normals. A different rotation would leave room to wonder whether the difference
you were seeing was just a different viewing angle.

## Where it sits, and how that was chosen

```cpp
constexpr float SCALE = 0.48f;                   // matches CubeConfig::SCALES[1] exactly
const glm::vec3 POSITION(2.00f, 1.00f, 0.0f);    // just above flat cube 1
```

The scene was already full, so the position came from a search over candidate
spots rather than a guess: of the scales tried, `0.75`, `0.65` and `0.55` had
nowhere to fit at all, and `0.48` - the one that matches flat cube 1 exactly - fits
at `(2.00, 1.00)` with room to spare.

All four cubes share one rotation, which makes the exact overlap test simple.
Two identically-oriented cubes are separated unless **every** one of the three
projections of the centre-to-centre vector is shorter than the sum of their
half-extents; if all three were, then `|d|^2 < 3(h1+h2)^2`. So the condition is
just `distance > sqrt(3) * (h1 + h2)`:

| pair | distance | needs > | margin |
|---|---:|---:|---:|
| smooth (0.48) vs flat cube 0 (0.75) | 1.749 | 1.065 | +0.684 |
| smooth (0.48) vs flat cube 1 (0.48) | 0.922 | 0.831 | +0.091 |
| smooth (0.48) vs flat cube 2 (0.30) | 0.765 | 0.675 | +0.089 |

Confirmed on screen too: the two bounding boxes do not overlap, the smooth cube is
fully inside the window, and it covers only background - it hides nothing that was
already there.

## The scene, counted

| | Phase 22 | Phase 23 |
|---|---:|---:|
| Meshes uploaded | 6 | **7** |
| Distinct shapes | 5 | 5 (**no new shape**) |
| Vertices on the GPU | 378 | **386** |
| Draw calls | 9 | **10** |
| Triangles drawn per frame | 628 | **640** |

Eight more vertices and twelve more triangles for an entire extra demonstration.

## Where each job happens

| Function | Job in this phase |
|---|---|
| `computeSmoothNormals()` | Overwrites every normal with the average of the triangles meeting it. No OpenGL, no knowledge of what the shape is |
| `buildSharedCubeGeometry()` | 8 corners, 36 indices, normals left at zero for the above to fill in |
| `makeSmoothCube()` | Builds, smooths, uploads |
| `updateScene()` | Builds `smoothCubeFrame` from the flat cubes' own `cubeSpin` |
| `renderScene()` | Draw 10, same scale as flat cube 1 |

## Likely teacher questions

### What is the formula?

`N_v = (sum of N_i) / || sum of N_i ||`, L9 slide 20: the normal at a vertex is
the normalized sum of the normals of the faces that meet it.

### Why do you need it, when the sphere's normal was free?

Because `normalize(position)` only works for a sphere centred on its origin. For a
shape with no formula - a hand-built hull, or a deformed surface - the triangles
are the only information left. Analytic is better when available; averaging is
what you use when it is not.

### Why can the flat 24-vertex cube not be smoothed?

Because each of its 24 vertices belongs to exactly one face, so "the average of
the faces meeting here" is just that one face's normal. Running
`computeSmoothNormals()` on it changes its normals by exactly zero - measured, not
assumed.

### Then what makes the other cube smoothable?

Its 8 corners are each shared by three faces, so there are three different normals
to average at every vertex.

### Why does the flat cube have 24 vertices in the first place?

Phase 10's reason: a vertex carries one colour and one normal, and each face wanted
its own. That choice is what makes it un-smoothable, which is the trade.

### What do the two cubes look like with `N` pressed?

The flat one shows three colours - one per visible face - with a single colour
covering 69.5% of it. The smooth one shows 15 952 distinct colours over 15 952
pixels, with no flat patch bigger than one pixel.

### Are the two cubes actually the same shape?

Yes - verified by comparing their triangles as sets of positions: the same twelve
triangles in the same twelve places, both with surface area exactly 6.0, and
wireframes within 1% of each other. Only the normals differ.

### Are the smoothed corner normals the body diagonals?

No, and that is worth knowing. They are 15.8 to 19.5 degrees off, because the
formula weights by triangle and a quad's two triangles do not use its four corners
equally. With 36 triangle-corner slots and 8 corners, `36/8 = 4.5` is not an
integer, so equal weighting is impossible: 4 corners are used 5 times and 4 are
used 4 times.

### Could you make them exact?

Yes - sum each **unique face** normal once instead of once per triangle. The
slide's formula sums per triangle because that works on any mesh without first
having to decide what counts as a "face".

### Is it safe to call `computeSmoothNormals()` twice?

Yes. It zeroes the normals first and recomputes from positions only, so a second
run changes nothing - measured drift 0.

### Why do both cubes spin together?

So that the only difference between them at any instant is the normals. A
different rotation would confuse "different shading" with "different angle".

## Simple viva modifications

- **The named exercise: explain why the flat cube cannot be smoothed.** Call
  `computeSmoothNormals(vertices, indices)` at the end of `buildCubeGeometry()`
  too, rebuild, and press `N`. The flat cube is **unchanged** - which is the
  whole answer.
- **Compare them:** press `N` and look at the two cubes on the right. Hard faces
  against gradient.
- **Prove the shape is the same:** press `W`. The two wireframes are the same 12
  triangles.
- **Break the sharing:** in `buildSharedCubeGeometry()`, duplicate one corner so
  two faces stop sharing it, and watch that corner's shading turn hard again.
- **Make the normals exact:** change the accumulation to add each face normal once
  rather than once per triangle, and check the corner normals become the body
  diagonals.
- **Recolour it:** edit `SmoothCubeConfig::BOTTOM_COLOR` or `TOP_COLOR`, then try
  to give its top face its own colour and explain why you cannot.
- **Move it:** edit `SmoothCubeConfig::POSITION`. Keep `distance > sqrt(3)*(h1+h2)`
  from the other cubes - 0.831 from flat cube 1.
- **Resize it:** edit `SCALE`, then put it back to `0.48` and say why matching flat
  cube 1 matters for the comparison.

## Checkpoint

Phase 23 passes when:

- Debug and Release builds succeed with no compiler warnings;
- both shaders compile and link, with no missing-uniform warning;
- a **second cube** sits beside the first, the same size, turning in step;
- the startup output reports `smoothcube vertices=8 indices=36 triangles=12` -
  8 vertices against the flat cube's 24, with the same 12 triangles;
- with `N` pressed the two are **obviously different**: hard face colours against
  blended corners;
- with `W` pressed they are obviously the **same** 12 triangles;
- you can explain, and demonstrate, why the 24-vertex cube cannot be smoothed;
- every earlier object and key still behaves as before;
- the window remains responsive, reports frame timing, and closes cleanly with
  every GPU object freed.

## What is not part of Phase 23

No lighting - every normal in the project is still only being *displayed*, never
used in a calculation. That is the whole of Stage C, starting at Phase 26 with the
normal matrix. No live counters in the window title (Phase 24). No runtime
tessellation control (Phase 25). No new shape, and there will not be another:
Stage B's five meshes are final.

Phase 24 adds live counters - draw calls, vertices and triangles - to the window
title.

---

## Note: the audit of earlier phases

### One cosmetic fix

The `[mesh]` report's name column was 8 characters wide, which `smoothcube` (10)
overflowed, knocking the startup block out of alignment. Widened to 10.

### A policy, stated once

Each phase document records **that phase's** output, so the startup blocks quoted
in Phases 16, 18, 20 and 22 list fewer meshes than the program prints today. That
is correct for a historical record and is not drift.

What does need flagging is a **checkpoint** that would now fail if someone ticked
it against current code. There is exactly one: Phase 20's cylinder counts
(`32 / 96 / 32`), superseded when Phase 21 added the end caps. Both its status
block and its checkpoint list carry a note saying so.

### No new defects

The smooth cube covers only background, hides nothing, and is clear of all three
flat cubes by the exact same-orientation test.

### Still open, and still deliberate

The test triangle passes through the largest cube about 0.81% of the time, a
consequence of the Phase 19 pulse fix. It renders correctly and moving a cube
would invalidate measured evidence in three documents. Recorded in full at the end
of [PHASE_20_EXPLANATION.md](PHASE_20_EXPLANATION.md).
