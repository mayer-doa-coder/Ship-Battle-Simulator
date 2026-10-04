# Phase 21 - The Cylinder Caps: Closing the Solid

## Status

Phase 21 is complete and verified. Debug and Release builds succeeded with no
compiler warnings, both shaders linked with no missing-uniform warning, and a
live Release run showed a closed tube standing on the left of the scene. Nothing
was printed to the error output, and the program closed cleanly.

```text
[mesh] cylinder vertices=66 indices=192 triangles=64
```

At `SEGMENTS = 16`: `4 x 16 + 2 = 66` vertices, `4 x 16 = 64` triangles,
`12 x 16 = 192` indices. The wall alone was 32 / 32 / 96.

### 1. The checkpoint: the hole is gone

Turning back-face culling off can only *add* pixels if there is an inside surface
to reveal. So: render the shape with culling on and off, and compare.

| camera | culling ON | culling OFF | difference |
|---|---:|---:|---:|
| looking up from below | 18 517 px | 18 517 px | **0** |
| looking down from above | 38 601 px | 38 601 px | **0** |
| level with the middle | 28 451 px | 28 451 px | **0** |

Zero from every angle. Compare Phase 20, the same measurement on the wall alone:

```text
looking up from below    culling ON 13 378 px, OFF 18 517 px, difference 5 139
```

That 5 139-pixel gap - 27.8% of the shape - was the missing lid. **It is now 0.**

Two cross-checks fall out of this, and both are exact:

- From below the closed solid covers **18 517 px**, which is *precisely* the
  number Phase 20 measured with culling switched off. Phase 20's "culling off"
  picture was an accurate prediction of what Phase 21 would look like.
- In the default view the silhouette grew from 24 375 px to 25 514 px, a gain of
  **1 139 px** - and the bottom cap measures **exactly 1 139 px** in the normals
  view. The lid filled the hole and changed nothing else.

### 2. The caps are two flat colours against the sweeping wall

| view | measurement |
|---|---|
| from above | 7 604 px of the exact `+Y` colour `(127,255,127)` - 19.7% of the visible shape |
| from below | 5 139 px of the exact `-Y` colour `(127,0,127)` |
| edge-on (camera level with the middle) | **0 px** of either cap colour |
| default view | 1 139 px of the `-Y` colour - the eye is below the tube, so the underside shows |

From above, the single commonest colour in the whole frame **is** the `+Y` cap
colour. One flat value over a big area, against a wall whose colour changes every
pixel. Edge-on, neither appears at all, because you are looking along the rims.

### 3. The counts, for every segment count from 3 to 64

```text
vertices = 4*segments + 2, indices = 12*segments, for all 62 values   OK
every index is inside the vertex array                               OK
every vertex is referenced                                           OK
```

| segments | vertices | triangles | indices | (wall alone was) |
|---:|---:|---:|---:|---:|
| 3 | 14 | 12 | 36 | 6 |
| 6 | 26 | 24 | 72 | 12 |
| 16 | 66 | 64 | 192 | 32 |
| 64 | 258 | 256 | 768 | 128 |

### 4. The normals

```text
of 66 vertices: 32 sideways (wall), 17 exactly (0,+1,0), 17 exactly (0,-1,0), 0 other
distinct normals in the whole mesh: 18   (16 wall sweep + 2 caps)
```

Each cap has 17 vertices - one centre and 16 rim - all carrying **one** flat
normal. The wall's 32 still sweep. No normal in the mesh is anything else.

### 5. The caps sit exactly on the wall - no crack

```text
cap rim vertices bit-identical to the wall's: bottom 16/16, top 16/16
centres: bottom (0.0,-0.5,0.0), top (0.0,+0.5,0.0)
```

Not "within a tolerance" - **bit-identical**, because both are built from the
same precomputed ring of positions.

### 6. Winding, with no degenerate triangles

| segments | agrees with stored normal | points away from the centre | degenerate |
|---:|---|---|---:|
| 3 | yes | yes | 0 |
| 6 | yes | yes | 0 |
| 16 | yes | yes | 0 |
| 64 | yes | yes | 0 |

### 7. It is watertight

This is the strongest statement available about a closed surface: **every edge
must be shared by exactly two triangles.** An edge used once is a hole; an edge
used three or more times is a fold.

| segments | edges | used twice | used once (a hole) | used more |
|---:|---:|---:|---:|---:|
| 3 | 18 | 18 | **0** | 0 |
| 6 | 36 | 36 | **0** | 0 |
| 16 | 96 | 96 | **0** | 0 |
| 64 | 384 | 384 | **0** | 0 |

Edges are compared by **position**, not by vertex index, so a wall rim edge and
the cap rim edge lying on top of it count as the same edge - which is exactly the
question being asked. The mesh is a closed 2-manifold at every segment count.

### 8. The area now converges on a closed cylinder

A true closed cylinder of diameter 1 and height 1 has area
`2 pi r h + 2 pi r^2 = 1.5 pi = 4.712389`.

| segments | total area | % of true | of which caps | exact regular n-gons |
|---:|---:|---:|---:|---:|
| 3 | 3.247595 | 68.92% | 0.649519 | 0.649519 |
| 6 | 4.299038 | 91.23% | 1.299038 | 1.299038 |
| 16 | 4.652179 | 98.72% | 1.530734 | 1.530734 |
| 64 | 4.708605 | 99.92% | 1.568274 | 1.568274 |

The caps' area matches the exact formula for two regular polygons
(`0.5 * n * r^2 * sin(2 pi / n)`) to six decimal places at every segment count.

### 9. The viva change: `HEIGHT`

| `HEIGHT` | y spans | tall | diameter |
|---:|---|---:|---:|
| 0.3 | -0.150 .. +0.150 | 0.300 | 0.500 |
| 0.9 | -0.450 .. +0.450 | 0.900 | 0.500 |
| 2.5 | -1.250 .. +1.250 | 2.500 | 0.500 |

The caps follow the height automatically, because they are part of the same unit
mesh and the same draw-time scale. The diameter does not move.

## What changed

| File | Change |
|---|---|
| `src/Mesh.h` | `buildCylinderGeometry()` now also builds **two end caps**; a shared `ring` of positions was factored out |
| `src/main.cpp` | Header comment, window title and startup message only |
| `src/main.cpp` | Draw 8's comment: one mesh, one draw call, now wall **and** caps |

No new config values. `CylinderConfig` is untouched - the caps take their colours
from the existing `BOTTOM_COLOR` and `TOP_COLOR`, and their size from the existing
`DIAMETER` and `HEIGHT`.

## The one idea

**A cap cannot reuse the wall's rim vertices.**

The wall's bottom rim vertex at segment `i` already sits in exactly the right
place for the bottom cap's rim. It is tempting - and wrong - to just index it
again. A vertex carries **one** normal, and these two surfaces need different
ones:

```text
wall at that point   normal points SIDEWAYS, out from the axis
cap  at that point   normal points STRAIGHT DOWN
```

Sharing would force one of them to be wrong. So every cap rim vertex is a
**duplicate in position and a different vertex in meaning**:

```text
wall      2 * segments        vertices
each cap  1 + segments        vertices   (a centre, and its own copy of the rim)
-----------------------------------------
total     4 * segments + 2    vertices
```

This is the Phase 10 cube lesson exactly - *a cube needs 24 vertices, not 8,
because a corner shared by three faces cannot carry three different normals* -
and it is why the caps get their own phase rather than four more lines in the
previous one.

### I predicted this wrong, and the mistake is instructive

At the end of Phase 20 I wrote in `AGENT.md` that the counts would become
`2*segments + 2` vertices. That number assumes the caps reuse the wall's rim and
only add two centres. It would have produced a cylinder whose end caps were
shaded as though they curved sideways - the lighting would have been wrong at
both ends and the picture would have looked almost right.

The correct count is `4*segments + 2`. `AGENT.md` has been corrected.

## The triangle fan

Each cap is a **fan**: one centre vertex, and one triangle from it out to each
neighbouring pair of rim vertices.

```text
        rim2 ___ rim1
         / \      / \         triangle 0: centre, rim0, rim1
        /   \    /   \        triangle 1: centre, rim1, rim2
    rim3 --- C ---- rim0      ...
        \   /    \   /        triangle n-1: centre, rim(n-1), rim0
         \ /      \ /
        rim4 ---- rim5
```

It is written as ordinary indexed triangles rather than with `GL_TRIANGLE_FAN`,
for two reasons: the whole cylinder is **one** `glDrawElements` call, and a fan
primitive would need a call of its own; and `Mesh` only knows how to draw
`GL_TRIANGLES`, which is a simplification worth keeping.

## The two caps wind in opposite directions

Counter-clockwise *as seen from outside* means counter-clockwise seen from
**above** for the top cap and from **below** for the bottom one. So the two fans
list their corners in reverse order:

```cpp
indices.push_back(centre);
indices.push_back(top ? a : b);   // the only difference between the two fans
indices.push_back(top ? b : a);
```

Getting this backwards is a nasty bug because it is **invisible from one side**.
A reversed top cap looks perfect from below and leaves the top of the tube
see-through - which is exactly the hole Phase 20 measured, now in only one place
instead of two. It is the kind of mistake the watertight test in section 7 catches
and the eye does not.

## One shared ring, three sets of vertices

```cpp
std::vector<glm::vec2> ring;
for (int i = 0; i < segments; ++i) {
    const float angle = TWO_PI * i / segments;
    ring.push_back(glm::vec2(std::sin(angle) * UNIT_HALF_EXTENT,
                             std::cos(angle) * UNIT_HALF_EXTENT));
}
```

The `(x, z)` positions are worked out **once** and then used three times: for the
wall, for the bottom cap's rim, and for the top cap's rim. The three sets of
vertices are separate - they must be, for the normals - but they are built from
the same numbers.

That is what guarantees a cap's edge lands exactly on the wall's edge. Calling
`sin` and `cos` again for each would almost certainly give the same answers, but
"almost certainly" is a poor foundation for a seam: a one-bit difference would
open a hairline crack that the watertight test would catch and that would be
nearly impossible to see. Measured: 16 of 16 cap rim vertices are **bit-identical**
to their wall counterparts at both ends.

## The default view already shows a cap

The tube stands at `y = +0.95` while the eye is at `y = 0`, so the default view
looks slightly **up** at it and the new bottom cap is partly in view - measured at
1 139 pixels. That is the same geometry that made Phase 20's hole visible without
touching a key, and it means the fix is visible without touching one either.

## The scene, counted

| | Phase 20 | Phase 21 |
|---|---:|---:|
| Meshes uploaded | 5 | 5 |
| Vertices on the GPU | 144 | **178** |
| Draw calls | 8 | 8 |
| Triangles drawn per frame | 200 | **232** |

No new mesh, no new draw call - the caps went into the mesh that was already
there.

## Likely teacher questions

### Why can't the caps reuse the wall's rim vertices?

Because a vertex carries one normal, and the two surfaces need different ones:
the wall's points sideways out from the axis, the cap's points straight up or
down. Sharing would force one of them to be wrong. It is the same reason the cube
needs 24 vertices instead of 8.

### How many vertices does the closed cylinder have?

`4 * segments + 2`: `2 * segments` for the wall, plus `1 + segments` for each cap.
66 at `SEGMENTS = 16`. The `+ 2` is the two cap centres.

### What is a triangle fan?

One centre vertex with a triangle running from it to each neighbouring pair of
rim vertices, so `segments` triangles cover the disc. It is written here as plain
indexed triangles so the whole cylinder stays one draw call.

### Why do the two caps wind in opposite directions?

Because "counter-clockwise as seen from outside" means from above for the top cap
and from below for the bottom one. The two fans therefore list their corners in
reverse order.

### What would a reversed cap look like?

Perfect from one side and see-through from the other - the hole Phase 20
measured. That is why the watertight test matters: the eye cannot be relied on to
spot it.

### How do you know the cylinder is really closed?

Every edge is shared by exactly two triangles, checked at 3, 6, 16 and 64
segments: 96 edges at 16 segments, all used twice, none used once. And on screen,
turning culling off adds **zero** pixels from any angle, where the open tube
gained 5 139.

### Why build the ring of positions once instead of twice?

So the cap rim and the wall rim are built from identical numbers and land in
exactly the same place. Recomputing `sin` and `cos` would almost certainly agree,
but a one-bit difference would open a crack that is nearly invisible and hard to
diagnose. Measured bit-identical, 16 of 16, at both ends.

### What do the caps look like in the normals view?

Two flat colours - `(0, +1, 0)` light green on top and `(0, -1, 0)` purple
underneath - against the wall's smooth sweep. From above the cap colour is the
commonest single colour in the frame, covering 19.7% of the shape; edge-on,
neither appears at all.

### Does the cap change the silhouette?

Only where you could previously see through. In the default view the silhouette
grew by exactly 1 139 pixels, which is exactly the number of bottom-cap pixels -
the lid filled the hole and nothing else moved.

### Does changing `HEIGHT` still work?

Yes, and the caps follow automatically, because they are part of the same unit
mesh and the same draw-time scale. Checked at 0.3, 0.9 and 2.5: the height
changes and the diameter does not.

## Simple viva modifications

- **The named change: change the cylinder height.** Edit `CylinderConfig::HEIGHT`.
  The tube gets taller or shorter, the caps move with it, and the diameter does
  not change.
- **Reverse one cap on purpose:** swap the `top ? a : b` and `top ? b : a` lines.
  One end of the tube becomes see-through again while the other stays solid - and
  the normal picture barely changes, which is the point.
- **See the caps as flat colours:** press `N` and orbit from below to above. The
  wall's colours sweep; each cap is a single flat value.
- **Count the triangles:** press `W` at `SEGMENTS = 6`. 24 triangles - 12 on the
  wall and 6 on each cap - against `4 * segments`.
- **Make it a triangular prism:** set `SEGMENTS` to `3`. The caps become
  triangles, and the whole solid has 68.92% of a true cylinder's area.
- **Prove the caps are separate vertices:** set `SEGMENTS` to 16 and check the
  startup line says 66 vertices, not 34. Explain the difference in terms of
  normals.
- **Prove there is no crack:** there is nothing to see, which is the point - but
  the wireframe view shows the cap rim and the wall rim drawn as one line, not
  two.

## Checkpoint

Phase 21 passes when:

- Debug and Release builds succeed with no compiler warnings;
- both shaders compile and link, with no missing-uniform warning;
- the cylinder is a **closed solid** - you cannot see through it from any angle,
  including looking straight up into what used to be its open bottom;
- the startup output reports `cylinder vertices=66 indices=192 triangles=64` at
  `SEGMENTS = 16`, and follows `4 * SEGMENTS + 2` and `4 * SEGMENTS` otherwise;
- with `N` pressed, each cap is **one flat colour** against the wall's sweep;
- changing `HEIGHT` resizes the tube with the caps following, leaving the
  diameter alone;
- the grid, triangles, quad, cubes and the `N`, `D`, `W` and `O` keys all still
  behave as before;
- the window remains responsive, reports frame timing, and closes cleanly with
  every GPU object freed.

## What is not part of Phase 21

No sphere - `makeSphere(stacks, slices)` is Phase 22, and it is the first
two-parameter surface. No smooth-normal averaging (Phase 23). No runtime change
of `SEGMENTS` (Phase 25). No lighting, so the caps' flat normals and the wall's
analytic ones are still only being displayed, not used in a calculation.

Phase 22 adds the sphere, with `normal = normalize(position)`.

---

## Note: the audit of earlier phases

### The `AGENT.md` prediction, corrected

The one defect found was in my own forward-looking note at the end of Phase 20,
which predicted `2*segments + 2` vertices for this phase. It was wrong for a
reason worth understanding - it assumed the caps could share the wall's rim - and
has been corrected to `4*segments + 2`. Details above under "I predicted this
wrong".

### The strict-warning build

Phase 20 uncovered a flaw in how I had been running the `/W4` build: Git Bash was
rewriting `/W4` into a file path, and a failed configure looked identical to a
clean build under the filter I was using. That is now run from PowerShell and
checked three ways. Re-verified for this phase:

```text
CMakeCache.txt : CMAKE_CXX_FLAGS:STRING=/W4 /permissive- /EHsc
vcxproj        : <WarningLevel>Level4</WarningLevel>
actual cl line : /W4 /WX-
full clean rebuild, warnings in project sources: none
```

### Still open, and still deliberate

The test triangle passes through the largest cube about 0.81% of the time - a
consequence of the Phase 19 pulse fix making it 2.24x bigger. It renders
correctly, the triangle is the temporary Phase 2 test object, and moving a cube
would invalidate measured pixel evidence in three documents. Recorded in full at
the end of [PHASE_20_EXPLANATION.md](PHASE_20_EXPLANATION.md), with the one-line
change if you would rather the shapes never touched.
