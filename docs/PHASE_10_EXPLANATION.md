# Phase 10 - The Cube: the First Real 3D Object

## Status

Phase 10 is complete and verified. Debug and Release builds succeeded with no
compiler warnings, both shaders linked, and a live Release run showed a solid,
correctly coloured cube spinning next to the triangles and quad from earlier
phases. Nothing was printed to the error output, the loop stayed near
`120 FPS`, and the program exited with code `0`.

The cube was watched turn, not just drawn once and assumed correct:

| Check | Result |
|---|---|
| First look | Cyan and blue faces visible, with a sliver of green along the top edge |
| Same cube, about 2.8 seconds later | Completely different faces now facing the camera: yellow, green, and a sliver of cyan |
| Changing `HALF_SIZE` from `0.5` to `0.9` | The cube visibly grew, large enough to run off the edge of the window |
| Changing one face's colour | That one face, and only that face, changed colour |
| Pressing `D` (Phase 8's depth-test key) | The two triangles broke exactly as before; the cube kept looking correct on its own |

## What changed

| File | Change |
|---|---|
| `src/main.cpp` | New `CubeConfig` namespace: 24 vertices, 36 indices, a size, a position, a spin |
| `src/main.cpp` | New `CubeGpu` struct - the same VAO/VBO/EBO shape as `QuadGpu` |
| `src/main.cpp` | `createCube()` / `destroyCube()`, copied almost line for line from the quad's versions |
| `src/main.cpp` | `SceneState` gained `cubeModel` |
| `src/main.cpp` | `renderScene()` draws a fourth object |

No shader changed again. Every object so far - two triangles, a quad, now a
cube - uses the exact same `basic.vert` / `basic.frag`.

## Why a cube needs 24 vertices, not 8

A real cube has 8 corners. Naming each corner once, the obvious way, uses only
8 rows of vertex data. That was tried first here and rejected, for one
reason: **a vertex can only carry one colour**, and every corner of a cube is
shared by **three** faces. If corner data were shared between faces the way
the quad's diagonal corners were shared between its two triangles, every face
touching that corner would be forced to use the exact same colour - the whole
"one face colour each" idea from the plan would be impossible.

The fix is to give each face its **own** 4 corners, never shared with any
other face, even though two faces' corners sit at the exact same point in
space. `6 faces x 4 vertices = 24 vertices`. Two of those 24 might occupy
identical positions, but each one carries only its own face's colour, so
there is no conflict.

## The pattern, repeated six times

Every face uses **exactly** the same two-triangle, six-index pattern the quad
introduced in Phase 9:

```text
corner 0, corner 1, corner 2,
corner 2, corner 3, corner 0.
```

The cube's `INDICES` array is nothing more than that same six-number pattern,
once per face, with the starting corner number moved along by 4 each time
(`0`, then `4`, then `8`, and so on). Nothing new had to be invented for this
phase - Phase 9's one idea, indexing, was simply used six times.

## Getting the winding right, on all six sides

Phase 9 found out the hard way that a face's corners must be listed
**counter-clockwise, as seen from outside**, or `GL_CULL_FACE` throws it away.
A cube has six faces, each one facing a different direction, so "outside" is a
different direction every time.

Every face here was checked with the same rule used to fix the quad: take the
first two edges of the face's first triangle, and confirm they turn the
correct way to produce that face's outward direction. This was done for all
six faces before typing any of them in, not guessed and fixed by trial and
error. The result rendered correctly on the very first try - a solid, complete
cube with all six colours present at different moments as it turned, no
missing faces, no holes.

## Spinning on a tilted axis, on purpose

```cpp
const glm::vec3 SPIN_AXIS = glm::normalize(glm::vec3(0.4f, 1.0f, 0.3f));
```

The cube does not spin around a single flat axis such as straight up (`Y`) or
straight through the screen (`Z`). If it did, two of its faces (whichever ones
sit at the "poles" of that axis) would never turn to face the camera at all -
the viewer would only ever see four of the six faces, forever.

Tilting the axis slightly on two other directions means the spin eventually
carries **every** face past the camera. This was checked directly: the first
screenshot showed the cyan, blue, and green faces; a second screenshot taken
about `2.8` seconds later showed yellow, green, and cyan instead - proving the
cube had turned to a genuinely different orientation, not just sat there.

## `T * R`, the pattern from Phase 5, again

```cpp
scene.cubeModel = cubeSlide * cubeSpin;
```

This is exactly Phase 5's lesson, at cube scale. `cubeSpin` turns the cube
around its own centre first - the origin, which is where all 24 of its
vertices are measured from - and `cubeSlide` then carries the already-turning
cube out to `CubeConfig::POSITION`. Doing it the other way round would make
the cube **orbit** its resting place instead of spinning on the spot, the same
wobble-versus-orbit lesson the very first spinning triangle taught.

## `uTint` stays neutral again

```cpp
shader.setVec3("uTint", glm::vec3(1.0f, 1.0f, 1.0f));
```

Just like the quad, the cube's colour comes entirely from its own vertex
data - six faces, six colours, baked in once at creation. `uTint` is left at
`(1, 1, 1)` so it multiplies every colour by exactly `1` and changes nothing,
the same "no filter" behaviour Phase 3 documented as the neutral tint value.

## `D` still works, and reveals something new

Pressing `D` disables `GL_DEPTH_TEST` for **every** draw call this frame, not
just the two triangles - it is a single global switch, not something that can
be aimed at one object. This was tested with the cube on screen: the two
triangles broke exactly as they did in Phase 8, and the cube kept rendering
correctly, with no visible fault at all.

The reason is that a solid, convex shape like a cube never needs depth testing
against **itself**. At any pixel belonging to the cube, only one of its own
faces can ever be the front-most one, and `GL_CULL_FACE` has already thrown
away every face pointing away from the camera before drawing even starts.
Depth testing only becomes necessary the moment two **separate** objects
overlap on screen, which the cube currently does not do with anything else in
the scene.

## Where each job happens

| Function | Job in this phase |
|---|---|
| `createCube()` | Uploads the 24 vertices and 36 indices once, at startup |
| `updateScene()` | Rebuilds `cubeModel` (spin, then move) every frame from the clock |
| `renderScene()` | Binds the cube's own VAO and calls `glDrawElements` |
| `destroyCube()` | Frees the VAO, VBO, and EBO when the program closes |

## Likely teacher questions

### Why does this cube need 24 vertices instead of 8?

Because each of a cube's 8 real corners is shared by three faces, and a
vertex can only hold one colour. Giving every face its own 4 vertices lets
each face have a colour that belongs to it alone.

### Isn't that wasteful?

A little - three times as much vertex data as the minimum possible. It is a
deliberate, small cost accepted here because it is the simplest way to give
each face a flat, distinct colour. Later phases that light objects properly
will have an even stronger reason for this same choice: each face also needs
its own **normal** direction, which a shared corner could not hold either.

### Why does the cube use the same six-index pattern as the quad, six times?

Because a cube's face is just a quad. Nothing about indexing needed to change;
the same idea was reused once per face.

### How was the correct winding order worked out for six different directions?

By checking, for each face, whether the first two edges of its first triangle
turn the right way to point outward - the same check that fixed the quad's
winding in Phase 9, just repeated once per face before any of it was typed in.

### Why does the cube spin around a tilted axis instead of straight up or forward?

Spinning around a single flat axis like `Y` never turns the two faces at that
axis's own poles to face the camera - they would stay hidden forever. A
tilted axis eventually carries every face past the camera as the cube turns,
which is the real proof that all six exist and are attached correctly.

### Does pressing `D` break the cube the way it breaks the triangles?

No, and that is worth understanding. `GL_DEPTH_TEST` only matters where two
things could be drawn on top of each other. A single convex shape drawn by
itself never has that problem: `GL_CULL_FACE` already removes its hidden
faces before drawing starts, so there is nothing left to sort.

### Why is `uTint` left at `(1, 1, 1)` for the cube?

Because, like the quad, the cube's colours already come from its own vertex
data. A tint of `(1, 1, 1)` multiplies every colour by `1`, leaving it
unchanged.

## Simple viva modifications

- Resize the cube: change `HALF_SIZE`.
- Recolour one face: edit the three colour numbers on any one of its four
  vertex rows - only that face changes.
- Spin faster or slower: change `SPIN_SPEED`.
- Spin around a single flat axis on purpose: set `SPIN_AXIS` to
  `glm::vec3(0.0f, 1.0f, 0.0f)` and watch the top and bottom faces never
  appear.
- Move the cube: change `POSITION`.
- Prove culling alone keeps the cube correct: press `D` and confirm the cube
  still looks solid while the two triangles behind it visibly break.

## Checkpoint

Phase 10 passes when:

- Debug and Release builds succeed with no compiler warnings;
- all shaders compile and link, with no missing-uniform warning;
- a solid, correctly coloured cube is visible and turning;
- watching it over a few seconds shows genuinely different faces, not the
  same ones repeating;
- changing `HALF_SIZE` or a face colour produces the expected visible change;
- the earlier triangles, quad, and the `D` and `O` keys still behave exactly
  as before;
- the window remains responsive, reports frame timing, and closes cleanly.

## What is not part of Phase 10

No wireframe key yet, and no explicit discussion of why culling matters -
that is Phase 11. No camera control - the fixed view from Phase 7 is still in
use. No reusable mesh functions - the cube's data, like the triangle's and the
quad's, is still written out by hand.

Phase 11 adds a wireframe toggle and deliberately breaks one face's winding on
purpose, to show the hole culling leaves behind and prove the face was always
there, just discarded.
