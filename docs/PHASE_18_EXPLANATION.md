# Phase 18 - `makeGrid(N)`: the First Parameterised Generator

## Status

Phase 18 is complete and verified. Debug and Release builds succeeded with no
compiler warnings, both shaders linked with no missing-uniform warning, and a
live Release run showed a flat grid lying across the bottom of the scene like a
floor. Nothing was printed to the error output, the loop held about `120 FPS`,
and the program closed cleanly.

```text
[mesh] triangle vertices=3  indices=3   triangles=1
[mesh] quad     vertices=4  indices=6   triangles=2
[mesh] grid     vertices=81 indices=384 triangles=128
[mesh] cube     vertices=24 indices=36  triangles=12
```

Those grid numbers are the phase in one line. At `CELLS = 8`:
`(8 + 1)^2 = 81` vertices, `6 x 8 x 8 = 384` indices, `2 x 8 x 8 = 128`
triangles.

### 1. The three formulas hold for every N from 1 to 64

Not just at `CELLS = 8`. The generator was run for all 64 values and checked:

```text
vertices = (N+1)^2 and indices = 6*N*N, for all 64 values   OK
every index is inside the vertex array, for all 64          OK
every vertex is referenced by at least one triangle         OK
no NaN or infinity anywhere, for all 64                     OK
```

| N | vertices | triangles | indices |
|---:|---:|---:|---:|
| 1 | 4 | 2 | 6 |
| 2 | 9 | 8 | 24 |
| 4 | 25 | 32 | 96 |
| 8 | 81 | 128 | 384 |
| 16 | 289 | 512 | 1 536 |
| 32 | 1 089 | 2 048 | 6 144 |

### 2. The shape, the normals and the winding

```text
extents x[-0.500,+0.500] y[+0.000,+0.000] z[-0.500,+0.500]
exactly 1 x 1 in the XZ plane (a unit mesh)                            OK
perfectly flat: every y is exactly 0                                   OK
centred on its own origin                                              OK
every normal is exactly (0, 1, 0) - straight up, not +Z like the quad  OK
all 128 triangles wound counter-clockwise seen from above (+Y)          OK
all 128 triangles' winding AGREES with the stored normal                OK
total surface area = 1.0000000000                                       OK
a regular 9 x 9 lattice of positions                                    OK
```

The area is the check worth noticing. 128 triangles summing to **exactly** 1.0
means they tile the unit square with no gaps and no overlaps - a single
misplaced vertex or a duplicated index would change that total.

### 3. The vertices really are shared

```text
how many triangles each vertex belongs to:
       1 triangles :   2 vertices     <- two opposite corners
       2 triangles :   2 vertices     <- the other two corners
       3 triangles :  28 vertices     <- the edges
       6 triangles :  49 vertices     <- the interior
81 vertices stored, but 384 corner slots filled: a 4.7x saving
```

The 49 interior vertices each serve **six** triangles. That is what makes a grid
a grid rather than a heap of separate quads, and it is the whole payoff of
indexed drawing from Phase 9.

### 4. `N = 1` is a quad

```text
N=1 gives 4 vertices and 6 indices
4 vertices and 6 indices - the same topology as makeQuad       OK
each of the 4 corners carries exactly its own colour           OK
```

A grid cut into one piece *is* a quad. Being able to say that, and show it, is a
good sign the generator is built on the right idea.

### 5. The viva change: `CELLS` changes division, not size

```text
N= 1: 1028 x 157 px at (126,0), 126274 px
N= 4: 1028 x 157 px at (126,0), 126274 px
N= 8: 1028 x 157 px at (126,0), 126274 px
N=32: 1028 x 157 px at (126,0), 126274 px
```

Four values of `N`, from 2 triangles to 2048, covering **exactly the same
126 274 pixels** in exactly the same place. The grid's footprint never moves.
Separately, `GridConfig::SIZE` moves it and nothing else.

In solid shading the colours differ a little - 43 954 colour bytes, every one of
them by **1 level out of 255**. That is a real difference and an invisible one,
and it is explained below under "why the solid picture barely changes". In
wireframe the difference is not subtle at all:

```text
wireframe: N=4 draws 5 002 edge px, N=32 draws 34 380 edge px
```

Nearly seven times the edges on the same patch of floor. That contrast is
Phase 19's checkpoint, already measurable here.

### 6. It really is horizontal, and really is on screen

```text
grid alone, default view : 1028 x 157 px, 126 274 px, 13.7% of the window
                           reaching the bottom edge of the window
grid alone, looking down : 786 x 475 px, 317 788 px
```

From the default eye-level view the grid reads as a band of floor across the
bottom of the window - not an edge-on sliver. Lifting the camera (dragging the
mouse up) shows two and a half times as much of it, which is the proof that it
is lying flat rather than standing upright.

### 7. The normals view confirms it faces up - and that it is not the quad

With `N` pressed and everything else hidden, the grid is **one flat colour**,
`(127, 255, 127)`, across all 126 274 of its pixels. That is Phase 15's `+Y`
colour: green at full, red and blue at half. The quad, in the same view, is
light blue `(127, 127, 255)`.

Two different generators, two different normals, each provably correct. If
`makeGrid` had copied the quad's `(0, 0, 1)` by mistake, the picture would look
identical and this one key would catch it.

### 8. Drawing the grid first protects Phase 8's demonstration

The grid is drawn **before** everything else, and that is a decision, not an
accident. To prove it matters, a deliberately wrong build was made with the grid
drawn last, and both were rendered with depth testing switched off:

```text
pixels where the grid overlaps another object            : 3 596
depth OFF, grid drawn FIRST (as shipped): 3 596 still show the object
depth OFF, grid drawn LAST  (deliberately wrong):     0
```

With depth off and the grid last, the grid covers **every single one** of those
pixels and Phase 8's near-versus-far triangle demonstration is destroyed. Drawn
first, not one pixel is lost. With depth testing on, order makes no difference
at all - which is exactly Phase 8's point.

Also confirmed: with depth on, every pixel belonging to a triangle, the quad or a
cube is **identical** with and without the grid. The new object took nothing away
from the old ones.

## What changed

| File | Change |
|---|---|
| `src/Mesh.h` | **`buildGridGeometry()` and `makeGrid()` added** - the third generator and the first parameterised one |
| `src/main.cpp` | New `GridConfig`: `CELLS`, `SIZE`, `POSITION`, `CORNER_COLORS` |
| `src/main.cpp` | `SceneState::gridFrame`; built in `updateScene()` as a translation only |
| `src/main.cpp` | `createMeshes()` and `renderScene()` take a `gridMesh`; `main()` declares and destroys it |
| `src/main.cpp` | The grid is drawn **first**, with its size applied as a draw-time `glm::scale` |
| `src/main.cpp` | **Robustness fix:** `CubeConfig::COUNT` is now counted from `POSITIONS` instead of typed in - see the note at the end |
| `shaders/*` | Unchanged |

## The one idea

Every generator so far builds a fixed amount of geometry. `makeCube` always
makes 24 vertices. `makeQuad` always makes 4. **`makeGrid` is handed a number
and makes a different amount depending on it.** That is what "parameterised"
means, and it is the first time the project has a mesh whose size in memory is a
decision rather than a constant.

```text
makeCube(mesh, colors)              -> always 24 vertices, 36 indices
makeQuad(mesh, colors)              -> always  4 vertices,  6 indices
makeGrid(mesh, cells, colors)       -> (cells+1)^2 vertices, 6*cells*cells indices
```

The three formulas, which are worth being able to recite:

```text
vertices  = (cells + 1) * (cells + 1)
triangles = 2 * cells * cells
indices   = 6 * cells * cells
```

**The `+ 1` is a fence-post count.** A side cut into 4 pieces has 5 posts along
it. Forgetting that is the classic grid bug: it gives a mesh one row short, and
the last row of cells indexes past the end of the vertex array.

## `CELLS` and `SIZE` are different things

This is the distinction the phase exists to make, and they are easy to confuse
because both sound like "how big".

| | `GridConfig::CELLS` | `GridConfig::SIZE` |
|---|---|---|
| What it means | how finely the grid is **divided** | how **big** the grid is |
| Where it is used | inside `makeGrid`, at startup | in a `glm::scale`, at draw time |
| Changing it changes | the number of vertices and triangles | the space the grid covers |
| Changing it costs | a re-upload to the graphics card | nothing at all |
| Measured effect on screen | the footprint does not move at all | the footprint grows or shrinks |

The mesh is a **unit** mesh either way (Phase 16): always exactly 1 x 1,
whatever `CELLS` is. That was checked for every `N` from 1 to 64 - all of them
produce the identical 1 x 1 footprint.

This pair becomes important later. Phase 25's `+` and `-` keys change the
division live, to trade polygon count against quality, while the sea stays the
same size. Phase 33's Demo A relies on being able to make the ocean coarse
*without* making it small.

## Why it lies flat, not upright

The quad faces `+Z`, standing upright like a panel. The grid faces `+Y`, lying
flat like a floor. They are not interchangeable, and the choice is forced:

**This mesh is the sea.** In Phase 81 the wave displaces each vertex's `y` from
functions of its `x` and `z`:

```text
y = waveHeight(x, z, t)
```

So the grid's two varying coordinates have to be `x` and `z`, with `y` free to
be pushed up and down. Building it upright now would mean rebuilding it then,
and Phase 82's analytic normals would have to be rederived in a different plane.

It also means the two flat generators demonstrate two different normals, which is
why pressing `N` is a genuine test of both rather than a test of one.

## Reading the two loops

### The vertices: a lattice, row by row

```cpp
for (int j = 0; j < side; ++j) {           // side = cells + 1
    const float v = j * step;              // 0 .. 1 across z
    for (int i = 0; i < side; ++i) {
        const float u = i * step;          // 0 .. 1 across x
        position = (-UNIT_HALF_EXTENT + u, 0, -UNIT_HALF_EXTENT + v);
        ...
    }
}
```

`u` and `v` run from `0` to `1`, so subtracting the half extent centres the grid:
`x` and `z` both run `-0.5` to `+0.5`. The outer loop walks `z` and the inner
loop walks `x`, which fixes one formula that the second loop depends on
completely:

```text
vertex (i, j) is stored at index  j * side + i
```

### The indices: two triangles per cell

```text
 v01 --- v11        v00 = (i,     j)        = j * side + i
  |  \    |         v10 = (i + 1, j)        = v00 + 1
  |    \  |         v01 = (i,     j + 1)    = v00 + side
 v00 --- v10        v11 = (i + 1, j + 1)    = v01 + 1
```

Both triangles are `v00 -> v01 -> v11` and `v00 -> v11 -> v10`. Those turn
**counter-clockwise as seen from above**, which is what `GL_CULL_FACE` needs for
a surface whose normal points up.

The obvious-looking `v00 -> v10 -> v11` turns the *other* way. It would make the
entire grid invisible from above and visible only from underneath - and with no
error message, exactly as Phase 11's reversed cube face vanished. All 128
triangles were checked against the stored normal rather than trusted.

## The bilinear colour blend

`makeGrid` takes four colours, one per corner of the **whole grid**, in the same
cyclic order `makeQuad` uses. Every vertex in between is blended from all four:

```cpp
const glm::vec3 nearEdge = glm::mix(cornerColors[0], cornerColors[1], u);
const glm::vec3 farEdge  = glm::mix(cornerColors[3], cornerColors[2], u);
const glm::vec3 color    = glm::mix(nearEdge, farEdge, v);
```

Blend across `x` first, then across `z`. Checked: each corner vertex gets exactly
its own corner colour, and the centre vertex is exactly the average of all four.

The reason for a gradient rather than one flat colour is that it makes the
sharing visible. If each cell had its own colour the grid would look like a
patchwork, and a patchwork is what you get when vertices are *not* shared. A
smooth, seamless sheet is the visible evidence that neighbouring cells use the
same corner vertices.

### Why the solid picture barely changes with `N`

Measured: `N = 4` against `N = 32`, 43 954 colour bytes differ, every one of them
by exactly **1 level out of 255**.

The reason is worth understanding, because it is a small preview of the biggest
lesson in the project. The intended colour across the surface is **bilinear** -
it curves. What the GPU actually draws is **linear across each triangle**
(Phase 2's interpolation). Linear pieces cannot reproduce a curved function
exactly; they can only approximate it, and the approximation gets better the more
pieces there are.

That is precisely the Gouraud-versus-Phong argument from Phase 31 onward: a value
computed at the corners and interpolated between them is only as good as the
tessellation. Here the error is 1/255 and invisible, because colour across a flat
sheet barely curves. When the interpolated quantity is a specular highlight
instead, the same error becomes a highlight that vanishes entirely - Phase 33's
Demo A, on this very mesh.

## Why the grid is drawn first

With depth testing on, draw order is irrelevant - that is Phase 8's whole point.
It matters only when `D` switches depth testing off, and then whatever is drawn
last simply wins.

The grid is the largest thing on screen. Drawn last with depth off it would paint
straight over the two triangles and Phase 8's demonstration would be gone. Drawn
first, the triangles land on top of it and that demonstration is untouched. This
was measured both ways (section 8 above): shipped order keeps all 3 596 overlap
pixels, the wrong order loses every one.

**An earlier phase's evidence has to keep working.** That rule costs one line of
thought here and saves a confusing regression later.

## Where the grid sits, and why there

```cpp
constexpr float SIZE = 4.0f;
const glm::vec3 POSITION(0.0f, -1.40f, 0.0f);
```

`-1.40` is not arbitrary. The lowest point any other object reaches is the large
cube's bottom corner at `y = -1.15`, so `-1.40` clears every one of them - checked
in the geometry test, which confirms the grid occupies exactly `y = -1.40` and
spans `x` and `z` from `-2.0` to `+2.0`.

At the default camera position the grid is seen at a shallow angle and reads as a
band of floor across the bottom of the window. That is honest perspective, not a
problem: a floor seen from eye level looks like that. Dragging the mouse up lifts
the camera and shows it properly, and the startup message now says so.

## The scene, counted

| | Phase 17 | Phase 18 |
|---|---:|---:|
| Meshes uploaded | 3 | **4** |
| Generated meshes | 2 | **3** |
| Vertices on the GPU | 31 | **112** |
| Draw calls | 6 | **7** |
| Triangles drawn per frame | 40 | **168** |

## Where each job happens

| Function | Job in this phase |
|---|---|
| `buildGridGeometry()` | Builds `(N+1)^2` vertices and `6N^2` indices for a 1 x 1 horizontal grid. No OpenGL |
| `makeGrid()` | Builds the grid and uploads it, once, at startup |
| `createMeshes()` | Calls `makeGrid()` with `GridConfig::CELLS` and the corner colours |
| `updateScene()` | Builds `gridFrame` - a translation, nothing else |
| `renderScene()` | Draws the grid **first**, with `scale(SIZE, 1, SIZE)` applied at draw time |

## Likely teacher questions

### What makes this generator different from the first two?

It takes a parameter that changes how much geometry it builds. `makeCube` always
produces 24 vertices and `makeQuad` always 4; `makeGrid(N)` produces `(N+1)^2`,
so the size of the mesh in graphics memory becomes a decision.

### What are the three formulas?

`vertices = (N+1)^2`, `triangles = 2 * N * N`, `indices = 6 * N * N`.

### Why `N + 1` and not `N`?

Fence posts. A side cut into `N` pieces needs `N + 1` points along it. Using `N`
leaves the mesh one row short and the last row of cells indexes past the end of
the array.

### What is the difference between `CELLS` and `SIZE`?

`CELLS` is how finely the grid is divided; `SIZE` is how big it is. `CELLS`
changes the vertex and triangle counts and needs a re-upload. `SIZE` is a
`glm::scale` at draw time and costs nothing. Measured: every `N` from 1 to 64
produces the identical 1 x 1 footprint.

### Why does the grid lie flat when the quad stands up?

Because this mesh becomes the sea. Phase 81 displaces each vertex's `y` from
functions of `x` and `z`, so `x` and `z` have to be the two coordinates that vary
and `y` has to be free. Building it upright would mean rebuilding it then.

### How do you know the winding is right?

Every triangle's geometric normal - the cross product of two of its edges - was
computed and compared against the stored normal. All 128 agree and all point
`+Y`. On screen, pressing `N` shows the whole grid as one flat `+Y` colour.

### What would happen if you wound the triangles the other way?

The grid would be culled when seen from above and visible only from underneath,
with no error message, exactly like Phase 11's reversed cube face.

### Why do neighbouring cells share vertices?

Because that is what indexed drawing is for. At `N = 8`, 81 stored vertices fill
384 triangle-corner slots - a 4.7x saving - and the 49 interior vertices each
serve six triangles. Without sharing, a finely divided sea would be unaffordable.

### Why is the colour a gradient rather than one flat colour?

So the sharing is visible. A smooth, seamless sheet is evidence that neighbouring
cells use the same corner vertices; separate colours per cell would look like a
patchwork, which is what unshared vertices produce.

### Why does the solid grid look almost the same at `N = 4` and `N = 32`?

Because a flat surface's colour barely curves. The intended blend is bilinear but
the GPU interpolates linearly across each triangle, so more triangles means a
closer approximation - measured here as a difference of exactly 1 level out of
255. The same effect on a specular highlight instead of a colour is what makes
Gouraud shading fail in Phase 33.

### Then how do you show that `N` did something?

Press `W`. In wireframe, `N = 4` draws 5 002 pixels of edges and `N = 32` draws
34 380 on the same patch of floor.

### Why is the grid drawn before everything else?

With depth testing on it makes no difference. With `D` pressed it makes all the
difference: the grid is the largest thing on screen, and drawn last it would
paint over the two triangles and destroy Phase 8's demonstration. Measured both
ways - shipped order loses no pixels, the wrong order loses all 3 596.

### What happens if someone passes `N = 0`?

It is clamped to 1. Without the clamp, `1 / cells` would divide by zero and every
position would be a NaN, so nothing would be drawn at all. Checked for `0`, `-1`
and `-1000`: all three produce a valid 4-vertex grid with no NaN.

## Simple viva modifications

- **The named change: change `N`.** Edit `GridConfig::CELLS`. Point out that the
  startup line's three numbers all change together and still match
  `(N+1)^2`, `6N^2` and `2N^2`, while the grid covers exactly the same ground.
- **Then press `W`** and show the same edit again at `4` and `32`. This is the
  modification that makes `CELLS` visible, and it is Phase 19's checkpoint.
- **Resize instead of redivide:** edit `GridConfig::SIZE`. The grid covers more
  ground with exactly the same number of triangles - the opposite of the above,
  which is the whole point of keeping the two apart.
- **Move it:** edit `GridConfig::POSITION`. Raise it above `-1.15` and watch the
  large cube start to poke through the floor.
- **Prove it is horizontal:** press `N`, then drag the mouse up. The grid is
  green `(0, 1, 0)` while the quad is blue `(0, 0, 1)`.
- **Break the winding on purpose:** swap `v01` and `v10` in one of the two
  `indices.push_back` groups in `buildGridGeometry()`. Half of every cell
  disappears from above. Press `W` and the edges are still there.
- **Break the fence post on purpose:** change `const int side = cells + 1;` to
  `cells`. The grid comes out wrong and the indices run off the end of the
  vertex list - a good reminder of why that `+ 1` has a name.
- **Show the draw-order argument:** move the grid's `drawMesh` call to the end of
  `renderScene()`, then press `D`. The grid covers the triangles and Phase 8's
  demonstration is gone. Put it back.
- **Recolour the sea:** edit `GridConfig::CORNER_COLORS`. Corner 0 is the
  `(-x, -z)` corner.

## Checkpoint

Phase 18 passes when:

- Debug and Release builds succeed with no compiler warnings;
- both shaders compile and link, with no missing-uniform warning;
- a **flat grid** is visible, lying horizontally below the rest of the scene,
  and dragging the mouse up reveals more of it;
- the startup output reports `grid vertices=81 indices=384 triangles=128` at
  `CELLS = 8`, and those three numbers follow `(N+1)^2`, `6N^2` and `2N^2` for
  any other value;
- `CELLS` is a named value, and changing it changes the counts while the grid
  covers exactly the same ground;
- with `N` pressed, the grid is one flat `+Y` colour, distinct from the quad's;
- with `W` pressed, a higher `CELLS` is obviously denser;
- with `D` pressed, Phase 8's near-versus-far triangle demonstration still works
  exactly as before;
- the triangles, the quad, the three cubes, and the `N`, `D`, `W` and `O` keys
  and the mouse orbit and zoom all still behave as before;
- the window remains responsive, reports frame timing, and closes cleanly with
  every GPU object freed.

## What is not part of Phase 18

No waves: the grid is perfectly flat, and `uIsOcean`, `waveHeight` and the GPU
displacement are Phase 81. No analytic normals - every normal is `(0, 1, 0)`,
and Phase 82 is where the wave's slope makes them vary. No runtime change of
`CELLS`: the `+` and `-` keys that rebuild meshes live are Phase 25, so `CELLS`
is still a value you edit and rebuild. No cylinder or sphere (Phases 20-22). No
lighting, and no material - the grid's colours are still per-vertex.

Phase 19 is the wireframe comparison, and it needs almost no new code: the `W`
key has existed since Phase 11, so its real content is setting `CELLS` to `4` and
to `32`, photographing both, and counting the triangles by eye against
`2 * N * N`.

---

## Note: a robustness fix to `CubeConfig`, and an unfinished edit found

While starting this phase the project **did not compile**. The error was the
`static_assert` added in Phase 16:

```text
error C2338: static_assert failed: 'cubeFrames needs one identity matrix in its
initialiser list per cube - add or remove one to match CubeConfig::COUNT.'
```

`CubeConfig::COUNT` had been changed from `3` to `4` in the working tree, but no
fourth entry had been added to `POSITIONS` or `SCALES`. That is the "add a fourth
cube" exercise from Phase 16's own viva list, left part-finished - and the
`static_assert` did exactly the job it was written for, which is to stop that
state from compiling instead of letting it draw a cube with no position and no
size.

It was still a bad design, for two reasons: the number had to be kept in step
with two separate lists by hand, and the assert checked a magic `== 3` rather
than the thing that actually mattered. Both are fixed:

```cpp
const glm::vec3 POSITIONS[] = { ... };                 // the single source of truth
constexpr int COUNT = static_cast<int>(sizeof(POSITIONS) / sizeof(POSITIONS[0]));
constexpr float SCALES[] = { 0.75f, 0.48f, 0.30f };
static_assert(sizeof(SCALES) / sizeof(SCALES[0]) == sizeof(POSITIONS) / sizeof(POSITIONS[0]),
              "CubeConfig: POSITIONS and SCALES must have the same number of entries ...");
```

`COUNT` is now **counted** from the positions, so it can never disagree with
them, and the `static_assert` checks a real invariant: every cube needs both a
place and a size. `SceneState::cubeFrames` lost its hand-written list of three
identity matrices in favour of `= {}`, which value-initialises any number of
elements (safe here because `glm::mat4`'s default constructor is `= default` and
`GLM_FORCE_CTOR_INIT` is not set, so the array is zero-filled rather than left
holding whatever was in memory - and `updateScene()` overwrites every frame
before `renderScene()` reads any).

**This had the side effect of putting `COUNT` back to 3**, because `POSITIONS`
still lists three cubes. That matches the plan, Phase 16's checkpoint and all of
its measured evidence, so it is the documented state - but it does undo that
edit, and adding a fourth cube is now a single change in one place:

```cpp
const glm::vec3 POSITIONS[] = {
    {  1.10f, -0.50f, 0.0f },
    {  2.20f,  0.10f, 0.0f },
    {  1.25f,  0.85f, 0.0f },
    {  0.00f,  1.00f, 0.0f },   // a fourth cube: add a scale below to match
};
constexpr float SCALES[] = { 0.75f, 0.48f, 0.30f, 0.40f };
```

Phase 16's viva list has been corrected to describe this, since it previously
told you to expect the `static_assert`.
