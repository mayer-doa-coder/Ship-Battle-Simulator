# Phase 22 - `makeSphere(stacks, slices)`: the Two-Parameter Surface

## Status

Phase 22 is complete and verified. Debug and Release builds succeeded with no
compiler warnings, both shaders linked with no missing-uniform warning, and a
live Release run showed a ball at the bottom of the left-hand column. Nothing was
printed to the error output, and the program closed cleanly.

```text
[mesh] triangle vertices=3   indices=3    triangles=1
[mesh] quad     vertices=4   indices=6    triangles=2
[mesh] grid     vertices=81  indices=384  triangles=128
[mesh] cylinder vertices=66  indices=192  triangles=64
[mesh] sphere   vertices=200 indices=1188 triangles=396
[mesh] cube     vertices=24  indices=36   triangles=12
```

At `STACKS = 12`, `SLICES = 18`: `2 + 11 x 18 = 200` vertices,
`2 x 18 x 11 = 396` triangles, `6 x 18 x 11 = 1188` indices.

**This is the fifth and last of the five meshes the whole project is built
from.** Everything after this - the ship, the cannon, the cannonball, the sea,
the crew, the rain, the snow - is one of these five, scaled and placed by a
matrix.

### 1. The formulas, over 1 482 combinations

Every `stacks` from 2 to 40 against every `slices` from 3 to 40:

```text
1482 combinations tested
vertices = 2 + (stacks-1)*slices and indices = 6*slices*(stacks-1), every time   OK
every index inside the vertex array                                             OK
every vertex referenced                                                         OK
no NaN or infinity anywhere                                                      OK
```

| stacks x slices | vertices | triangles | indices |
|---|---:|---:|---:|
| 2 x 18 | 20 | 36 | 108 |
| 3 x 18 | 38 | 72 | 216 |
| 12 x 3 | 35 | 66 | 198 |
| 12 x 18 | 200 | 396 | 1 188 |
| 24 x 18 | 416 | 828 | 2 484 |

### 2. The normal *is* the position

```text
worst |normal| - 1              : 1.19e-07
worst |position| - 0.5          : 2.98e-08
worst |normal - normalize(pos)| : 0
distinct normals: 200 of 200 vertices
```

The middle figure says every vertex lies exactly on a radius-0.5 sphere. The
third says the normal and `normalize(position)` differ by **exactly zero**.

And the last line is the real novelty: **200 distinct normals out of 200
vertices.** No two alike. Compare every earlier mesh:

| mesh | distinct normals |
|---|---:|
| quad | 1 |
| grid | 1 |
| cube | 6 |
| cylinder | 18 (16 wall + 2 caps) |
| **sphere** | **one per vertex** |

### 3. The poles

```text
vertex 0   : pos (+0.0,+0.5,+0.0) normal (+0.0,+1.0,+0.0)
vertex 199 : pos (+0.0,-0.5,+0.0) normal (+0.0,-1.0,+0.0)
the north pole is ONE vertex serving 18 triangles (slices = 18)
exactly two vertices sit on the axis - no duplicated pole rows
```

### 4. No degenerate triangles - the naive sphere's classic flaw

| stacks x slices | degenerate | outward | agrees with stored normals |
|---|---:|---|---|
| 2 x 3 | 0 | yes | yes |
| 3 x 18 | 0 | yes | yes |
| 12 x 18 | 0 | yes | yes |
| 24 x 18 | 0 | yes | yes |

Zero zero-area triangles at every combination tested. That is not automatic - see
"Why the poles are fans" below.

### 5. It is a closed solid

Every edge shared by exactly two triangles, at every combination:

| stacks x slices | edges | used once (a hole) | used more than twice |
|---|---:|---:|---:|
| 2 x 3 | 9 | 0 | 0 |
| 12 x 18 | 594 | 0 | 0 |
| 24 x 18 | 1 242 | 0 | 0 |

On screen, turning culling off adds **0** pixels (21 340 either way).

### 6. The area converges on a true sphere

A true sphere of diameter 1 has area `4 pi r^2 = 3.141593`.

| stacks x slices | area | % of a true sphere |
|---|---:|---:|
| 3 x 6 | 2.469975 | 78.62% |
| 6 x 12 | 2.965322 | 94.39% |
| 12 x 24 | 3.096935 | 98.58% |
| 24 x 48 | 3.130391 | 99.64% |
| 48 x 96 | 3.138790 | 99.91% |

### 7. THE CHECKPOINT: the classic RGB ball

This is the phase's promise, and it can be stated precisely. In the `N` debug
view each channel encodes one component of the normal, so a ball must show all
three sweeping at once, in three different directions:

```text
across the middle (169 px): RED   spans  22..253, went backwards 1 time,  max step 5
down the middle   (160 px): GREEN spans   6..254, went backwards 0 times, max step 5
BLUE at the centre of the ball: 242, spanning 108..252 across it
distinct colours on the ball: 21340
```

- **Red** (`normal.x`) climbs left to right across nearly the whole range, in
  steps of at most 5 levels.
- **Green** (`normal.y`) sweeps top to bottom, monotonically, with zero
  reversals.
- **Blue** (`normal.z`) is brightest at the centre, where the surface faces you,
  and falls toward the rim.

Red one way, green at right angles to it, blue peaking in the middle. **That
three-way sweep is exactly what makes the classic RGB normal ball**, and it is
the first mesh in the project that can produce it.

### 8. The two parameters are independent

```text
stacks=6   slices=18 ->  7 distinct latitudes (expect  7)
stacks=12  slices=18 -> 13 distinct latitudes (expect 13)
stacks=24  slices=18 -> 25 distinct latitudes (expect 25)
stacks=12  slices=6  ->  6 vertices per ring  (expect  6)
stacks=12  slices=18 -> 18 vertices per ring  (expect 18)
stacks=12  slices=36 -> 36 vertices per ring  (expect 36)
```

`stacks` changes the latitudes and nothing else; `slices` changes the vertices
per ring and nothing else. And they look different on screen, which is the point:

```text
wireframe ink: 12x18 (shipped) 7595 px | 4 stacks x 18: 5318 px | 12 x 5 slices: 5165 px
the two coarse wireframes differ in 26898 bytes - they are not the same picture
```

Both cuts remove a similar amount of ink, yet produce **completely different**
pictures - one coarse in bands, one coarse in wedges.

### 9. Detail changes smoothness, not size

| | coarse (4 x 5) | fine (32 x 48) |
|---|---:|---:|
| silhouette | 17 845 px | 21 632 px |
| distinct normal colours | 17 845 | 21 632 |

The fine ball is **21.2% bigger in area**, because flat triangles always cut
inside the true sphere. That is a much larger effect than the cylinder's 1.58%,
because a sphere is curved in two directions at once.

## What changed

| File | Change |
|---|---|
| `src/Mesh.h` | **`buildSphereGeometry()` and `makeSphere()` added** - the fifth and last generator |
| `src/main.cpp` | New `SphereConfig`: `STACKS`, `SLICES`, `DIAMETER`, `POSITION`, `BOTTOM_COLOR`, `TOP_COLOR` |
| `src/main.cpp` | `SceneState::sphereFrame`; built in `updateScene()` as a translation only |
| `src/main.cpp` | `createMeshes()`, `renderScene()` and `main()` take a `sphereMesh`; Draw 9 added |
| `shaders/*` | Unchanged |

## The one idea

**For a sphere centred on its own origin, the normal is the position.**

```cpp
vertices.push_back({ position, glm::normalize(position), color });
```

The direction from the centre out to a point on the surface *is* the surface
normal there. Nothing has to be derived, no cross products, no table of face
directions - the position already holds the answer.

That is the simplest analytic normal in the project, and it is worth knowing
*why* it is this simple: because the mesh is a **unit mesh centred on the
origin** (Phase 16). A sphere built off-centre would need
`normalize(position - centre)`, and the whole convenience would be gone. The
unit-mesh rule keeps paying.

Compare the five meshes' normals, which is the whole arc of Stage B:

| mesh | normal | how it is known |
|---|---|---|
| cube | one of 6 axis directions | a table, written out per face |
| quad | `(0, 0, 1)` | a constant |
| grid | `(0, 1, 0)` | a constant |
| cylinder wall | `normalize(vec3(x, 0, z))` | derived - drop the `y`, point out from the axis |
| cylinder caps | `(0, +/-1, 0)` | a constant |
| **sphere** | **`normalize(position)`** | **nothing to derive at all** |

## Two parameters, and why they are different numbers

`stacks` is **latitude** - bands from the north pole to the south.
`slices` is **longitude** - steps around each ring.

They are shipped as `12` and `18`, deliberately **not** the same number. If both
were 16, a bug that mixed latitude up with longitude would look completely fine.
At 12 and 18 it shows immediately, because the ball would come out subdivided the
wrong way round. Using different values for two parameters that could be confused
is a cheap and permanent test.

```text
rings     = stacks - 1                  (the poles are not rings)
vertices  = 2 + (stacks - 1) * slices
triangles = 2 * slices * (stacks - 1)
```

The `- 1` is the pole-versus-ring distinction, and the `+ 2` is the two poles.

## Why the poles are fans, and why that matters

The obvious way to build a UV sphere is one rectangular lattice of
`(stacks + 1) x slices` vertices, with the top and bottom rows collapsed onto the
poles. It is shorter to write. It also produces a **ring of degenerate zero-area
triangles at each pole**, because two of each triangle's three corners end up at
the same point.

Degenerate triangles are not fatal - they rasterise to nothing - but they are
real waste, they are a sign the author did not think about the poles, and they
make a mesh that cannot pass a "no degenerate triangles" check. So each pole band
is a **triangle fan** instead, exactly like the cylinder's caps from Phase 21:

```text
        ring1_2 ___ ring1_1
         / \         / \          triangle 0: ring1_0, ring1_1, northPole
        /   \       /   \         triangle 1: ring1_1, ring1_2, northPole
   ring1_3 - northPole - ring1_0  ...
```

Verified: **0 degenerate triangles** at every combination tested.

## The poles are shared - unlike the cylinder's cap rims

This is the contrast most worth understanding, because Phase 21 said the
opposite about a shape that looks similar.

| | shared? | why |
|---|---|---|
| cylinder cap rim | **No** - needs its own vertices | the wall wants a sideways normal and the cap wants an axial one. Genuinely different directions at the same point |
| sphere pole | **Yes** - one vertex | the true normal at the north pole is `(0, 1, 0)`, and every triangle meeting there agrees with it |

So the rule is not "duplicate whenever surfaces meet". The rule is **a vertex
carries one normal**, and you duplicate only when the surfaces genuinely disagree
about what that normal is. At a sphere's pole they agree, so one vertex is both
correct and cheaper: measured, exactly two vertices sit on the axis, and each
pole serves all 18 triangles that meet it.

## The scale: all three factors the same

```cpp
const glm::mat4 sphereScale = glm::scale(glm::mat4(1.0f), glm::vec3(SphereConfig::DIAMETER));
```

The sphere is the only mesh in the project whose three scale factors are equal,
because it is round on every axis. The full set is now a nice illustration that
each mesh's own shape decides which factor goes where:

| mesh | scale | why |
|---|---|---|
| quad | `(SIZE, SIZE, 1)` | flat in `z` |
| grid | `(SIZE, 1, SIZE)` | flat in `y` |
| cylinder | `(DIAMETER, HEIGHT, DIAMETER)` | round in `x` and `z`, tall in `y` |
| **sphere** | `(DIAMETER, DIAMETER, DIAMETER)` | round in all three |

Measured on screen: the ball's width-to-height ratio is **1.069**, i.e. round,
not an ellipse. (The 7% is honest perspective - the ball sits off to the left, so
the camera sees it slightly from the side.)

## Where it sits

```cpp
const glm::vec3 POSITION(-1.8f, -0.88f, 0.0f);
constexpr float DIAMETER = 0.72f;
```

It completes a left-hand column: **cylinder on top, quad in the middle, ball at
the bottom.** Verified clear of everything - 0.12 below the quad's lower edge,
0.16 above the grid floor, and well clear of the test triangle's reach.

It does, correctly, **hide part of the grid floor** behind it - 582 pixels. That
is a ball floating above a floor being nearer the camera than the floor behind it,
which is what depth testing is for. Confirmed by checking what those pixels were:
every one of them blue-dominant (red 27-30, green 69-79, blue 144-150), i.e. the
grid, and **none** belonging to any other object.

## The scene, counted

| | Phase 21 | Phase 22 |
|---|---:|---:|
| Meshes uploaded | 5 | **6** |
| Generated meshes | 4 | **5** |
| Vertices on the GPU | 178 | **378** |
| Draw calls | 8 | **9** |
| Triangles drawn per frame | 232 | **628** |

The sphere alone is 396 of those 628 triangles - more than everything else put
together. A ball is expensive, which is exactly why `STACKS` and `SLICES` are
values you can turn down, and why Phase 25 makes them adjustable at runtime.

## Where each job happens

| Function | Job in this phase |
|---|---|
| `buildSphereGeometry()` | Builds `2 + (stacks-1)*slices` vertices for a 1-unit ball, with `normal = normalize(position)`. No OpenGL |
| `makeSphere()` | Builds it and uploads it once, at startup |
| `createMeshes()` | Calls `makeSphere()` with `SphereConfig`'s values |
| `updateScene()` | Builds `sphereFrame` - a translation, nothing else |
| `renderScene()` | Draw 9, with `scale(DIAMETER)` on all three axes |

## Likely teacher questions

### What is the sphere's normal?

`normalize(position)`. For a ball centred on its own origin, the direction out
from the centre to a surface point *is* the normal there, so there is nothing
else to work out. Verified as matching to exactly zero difference.

### Why is it that simple, when the cylinder's needed thought?

Because the mesh is a unit mesh centred on the origin. A sphere built off-centre
would need `normalize(position - centre)`. The cylinder's normal needs the `y`
component dropped, because a cylinder is only curved in two directions.

### What makes this the first two-parameter surface?

It is divided in two independent directions: `stacks` for latitude and `slices`
for longitude. Raising one makes it smoother pole-to-pole, the other smoother
round the middle. Verified independent: `stacks` controls the number of latitudes
and `slices` the vertices per ring, each without touching the other.

### Why are `STACKS` and `SLICES` different numbers?

So that confusing them is visible. At 12 and 18, a bug that swapped latitude for
longitude produces an obviously wrong ball; if both were 16 it would look
perfect.

### How many vertices, and why the `+ 2` and the `- 1`?

`2 + (stacks - 1) * slices`. The `+ 2` is the two poles. The `- 1` is because the
poles are not rings: `stacks` bands are separated by `stacks - 1` rings of
latitude.

### What goes wrong with the naive sphere?

Building one rectangular lattice with the top and bottom rows collapsed onto the
poles gives a ring of degenerate zero-area triangles at each pole, where two
corners coincide. Making the pole bands triangle fans instead avoids them -
verified at 0 degenerates everywhere.

### Why is a sphere's pole one shared vertex when the cylinder's cap rim was not?

Because the rule is "a vertex carries one normal", not "always duplicate". At a
sphere's pole every triangle meeting there agrees the normal is `(0, 1, 0)`, so
one vertex is correct. At a cylinder's cap rim the wall wants sideways and the
cap wants axial - genuinely different - so they must be separate.

### How do you know it is a closed solid?

Every edge is shared by exactly two triangles - 594 edges at 12 x 18, none used
once - and on screen turning culling off adds zero pixels.

### What is the "classic RGB ball", measured?

Red rises left to right across nearly the full 0-255 range, green sweeps top to
bottom monotonically, and blue peaks at the centre where the surface faces you.
Three sweeps at once, in three directions, which is what no earlier mesh could
produce.

### Why is the coarse ball smaller than the fine one?

Flat triangles always cut inside the true sphere, so a coarse mesh has less area:
78.62% of a true sphere at 3 x 6, rising to 99.91% at 48 x 96. On screen the fine
ball covers 21.2% more pixels than the coarse one - a far bigger effect than the
cylinder's 1.58%, because a sphere curves in two directions at once.

### Why does the ball hide part of the grid?

Because it floats above the floor and is nearer the camera than the part of the
floor behind it. That is correct occlusion, not an intersection - there is a
0.16-unit gap, and every hidden pixel was confirmed to be the grid's blue.

## Simple viva modifications

- **The named change: change stacks and slices independently.** Set `STACKS` to
  `4` with `SLICES` at `18`, then `STACKS` back to `12` with `SLICES` at `5`.
  Press `W` for each. Both remove a similar amount of ink and look completely
  different - one coarse in bands, the other in wedges.
- **See the RGB ball:** press `N`. Red across, green down, blue brightest in the
  middle.
- **Make it a diamond:** set `STACKS` to `2`, the minimum. Two pole fans meeting
  at one ring - still a closed solid.
- **Make it a sliver:** set `SLICES` to `3`, the minimum.
- **Prove the clamp:** set either to `0`. You still get a valid shape.
- **Resize instead of redivide:** edit `DIAMETER`. Same triangle count, bigger
  ball.
- **Squash it:** change the scale to `glm::vec3(DIAMETER, DIAMETER * 0.5f, DIAMETER)`
  for an egg, and say what that does to the normals (nothing yet - Phase 26's
  normal matrix is what would fix them).
- **Prove the normal claim:** temporarily change the normal to
  `normalize(position + glm::vec3(0.3f, 0, 0))`. The normal picture is unchanged;
  press `N` and the whole ball's colour sweep is visibly skewed.
- **Count the triangles:** at 12 x 18 the startup line says 396. Check it against
  `2 * slices * (stacks - 1)`.

## Checkpoint

Phase 22 passes when:

- Debug and Release builds succeed with no compiler warnings;
- both shaders compile and link, with no missing-uniform warning;
- a **ball** sits at the bottom of the left-hand column, round rather than
  elliptical, and closed from every angle;
- the startup output reports `sphere vertices=200 indices=1188 triangles=396` at
  `12 x 18`, and follows `2 + (STACKS-1)*SLICES` and `2*SLICES*(STACKS-1`)
  otherwise;
- with `N` pressed it renders as **the classic RGB ball** - red across, green
  down, blue brightest at the centre;
- `STACKS` and `SLICES` can be changed independently and look different from each
  other in wireframe;
- the grid, triangles, quad, cylinder, cubes and the `N`, `D`, `W` and `O` keys
  all still behave as before;
- the window remains responsive, reports frame timing, and closes cleanly with
  every GPU object freed.

## What is not part of Phase 22

No smooth-normal averaging - `computeSmoothNormals()` is Phase 23, and it exists
for shapes whose normals *cannot* simply be written down. The sphere is the
opposite case: its normal is exact and free, which makes it the right note to end
the analytic-normal run on.

No live counters in the window title (Phase 24). No runtime change of `STACKS` or
`SLICES` - the `+` and `-` keys are Phase 25. No lighting, so every normal in the
project so far is still only being displayed, never used in a calculation.

**Stage B's geometry is now complete.** All five meshes exist: cube, quad, grid,
cylinder, sphere. Phase 23 adds no new shape - it adds a second *way of deciding
normals*, and demonstrates it by comparing a shared-vertex cube against the flat
24-vertex one.

---

## Note: the audit of earlier phases

### Every documented count checked against the running program

All the `vertices=... indices=... triangles=...` claims in every phase document
were compared against what the program actually prints. They match, with one
expected exception: Phase 20's cylinder counts (`32 / 96 / 32`) describe the
side wall before Phase 21 added the caps. That document already carried a
"superseded" note near the top; **its checkpoint list has now been annotated
too**, so nobody can tick a stale number while running current code.

### No new defects

The sphere's only interaction with the existing scene is that it correctly hides
582 pixels of grid floor, which was confirmed to be occlusion rather than
intersection by checking the colour of every hidden pixel.

### Still open, and still deliberate

The test triangle passes through the largest cube about 0.81% of the time, a
consequence of the Phase 19 pulse fix. It renders correctly and moving a cube
would invalidate measured evidence in three documents. Recorded in full at the
end of [PHASE_20_EXPLANATION.md](PHASE_20_EXPLANATION.md).
