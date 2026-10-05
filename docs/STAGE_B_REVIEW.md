# Stage B Review - Reusable Geometry (Phases 14-25)

This is a check-up on the whole of Stage B, written in plain language. Stage B is
the part of the project that stopped writing shapes out by hand and started
**generating** them, and that decided - once, for the whole project - how a shape
gets onto the graphics card and how big it is when it is drawn.

Stage A got a triangle moving in real 3D space. Stage B turned that into a small
library of reusable shapes. Nothing in Stage B is lit. That is deliberate: a wrong
normal is nearly invisible once it is hidden inside a lighting equation, so every
normal in the project was made checkable by eye *before* the first light is
switched on in Phase 26.

---

## 1. Verdict

**Stage B is complete and correct. Nothing is broken, and there are no open
defects.**

| | |
|---|---|
| Phases covered | 14 to 25, twelve phases |
| Checkpoints passing | 12 of 12 |
| Builds | Debug, Release and strict `/W4` - no warnings of any kind |
| Known defects | none |
| Cosmetic items accepted on purpose | 1 (see 5.4) |
| Checks run for this review | 3,010, all passing |

What exists at the end of Stage B:

- **one** `Vertex` layout and **one** `Mesh` type that owns its own graphics-card
  memory and frees it properly;
- **five** shape generators - cube, quad, grid, cylinder, sphere - and no sixth
  will ever be added;
- **both** ways of deciding which way a surface faces: written down from knowing
  the shape, and averaged from the triangles;
- a rule that a shape is built 1 unit across and resized when it is drawn;
- a way to **see** every normal with one key press, before any lighting exists;
- live counters telling you what each frame actually cost;
- live detail control, so the amount of geometry can be changed while the program
  runs without leaking graphics memory.

That is the complete toolbox the rest of the project is built from. From Phase 26
onward, no new shapes are invented - the ship, the cannon, the cannonball, the
sea, the crew, the rain and the snow are all these five shapes, resized and placed
by matrices.

---

## 2. How this was checked, and what was not

Three separate test suites were used. None of them re-writes the project's logic -
they all call the project's own code.

| Suite | What it does | Checks |
|---|---|---:|
| **Shape maths** | Calls the real shape-building functions from `src/Mesh.h` at **every** detail setting, and checks the results | 2,946 |
| **Live graphics** | Runs the real program against a real graphics card, including 100 mesh rebuilds and a fake keyboard driving the real input code | 32 |
| **Build and run** | Clean Debug, Release and strict-warning builds, then runs both programs and reads their output | 32 |

### What "checked" means here

For every shape, at every detail setting it can be built at, the tests confirm:

- **the counts are right** - the number of corner points and triangles matches the
  formula written in the code's own comments;
- **no index points at nothing** - every triangle refers to corner points that
  actually exist;
- **no zero-area triangles** - a triangle with two corners in the same place
  covers no pixels and is pure waste; a common mistake at the poles of a ball;
- **every normal is unit length** - a normal that is not exactly length 1 would
  silently brighten or darken the object once lighting arrives;
- **the solids are watertight** - every edge is shared by exactly two triangles,
  compared by **position** rather than by index number, so a mesh that has
  duplicate corners at a seam cannot pass by accident.

### What was NOT checked

- **Nothing was checked by looking at it.** Everything here is measured
  numerically or read from the program's own output. A human still needs to look
  at the window once per phase.
- **Only one graphics card and one driver.** Everything ran on an NVIDIA RTX 5050.
  One known difference was found this way and is documented in Phase 15's notes:
  the value `0.5` in the debug view comes out as 128 on Intel graphics and 127 on
  NVIDIA, because `127.5` has to be rounded and the two drivers round it
  differently. That is the driver's choice, not a bug.
- **Linux was not built.** The `CMakeLists.txt` has a Linux path, and it has not
  been exercised recently.
- **No memory-leak tool was run on the CPU side.** GPU object leaks were tested
  directly (see 5.3); ordinary C++ memory was not, though the project allocates
  almost nothing outside startup.

---

## 3. What Stage B is, as one picture

### 3.1 The one-shape-many-sizes rule

This is the single most important idea in Stage B, and it is worth stating plainly
because it is easy to get backwards.

**A shape is always built exactly 1 unit across. Its real size is decided at the
moment it is drawn.**

```cpp
// The mesh is 1 x 1 x 1. Always. makeCube takes no size at all.
makeCube(cubeMesh, CubeConfig::FACE_COLORS);

// The size lives here, in the matrix, at draw time.
drawMesh(shader, stats, cubeMesh,
         frame * glm::scale(glm::mat4(1.0f), glm::vec3(0.75f)),
         tint);
```

Why this matters is not tidiness, it is **memory**. A mesh is a block of memory on
the graphics card. A matrix is sixteen numbers sent along with a draw command. If
size lived in the shape's data, then a big cube and a small cube would be two
separate blocks of memory. Because size lives in the matrix instead, **one** cube
on the graphics card can be drawn at any size, any number of times, in the same
frame.

The scene proves it: three cubes of three different sizes, one mesh, one upload.

There is a second reason, and it becomes the most important rule in the whole
project at Stage D. A stored matrix - a "frame" - holds only *where a thing is* and
*how it is turned*. It never holds a size. When the cannon becomes a child of the
deck and the deck a child of the hull, a size left in a parent's matrix would be
inherited by everything below it, so a stretched hull would stretch the cannon, the
masts and the crew standing on it. Keeping size out of stored matrices makes that
impossible by construction rather than by remembering.

### 3.2 What a vertex is, and why a cube needs 24 of them

```cpp
struct Vertex {
    glm::vec3 position;   // where this corner is
    glm::vec3 normal;     // which way the surface faces here
    glm::vec3 color;      // what colour it is here
};                        // 36 bytes: 12 + 12 + 12
```

The rule that explains most of Stage B's geometry is this:

> **A vertex carries exactly one value for each thing.** One position, one normal,
> one colour.

A cube has 8 corners in space. But at each corner, **three faces meet, and they
face three different directions**. A single vertex cannot hold three normals. So
each corner has to be stored three times - once per face - and `8 x 3 = 24`.

The same rule explains every other count in the project:

| Shape | Why it needs the vertices it has |
|---|---|
| Cube, 24 | Each corner belongs to 3 faces that face different ways |
| Quad, 4 | Flat, one normal, so corners can be shared |
| Grid, `(N+1)^2` | Flat, all normals point up, so every interior corner is shared by 4 cells |
| Cylinder, `4S+2` | The curved wall and the flat lids disagree about which way they face at the rim, so the rim is stored twice |
| Sphere, `2+(S-1)L` | Nothing disagrees anywhere - it is smooth all over - so almost everything is shared |

The cylinder is the clearest case. Its wall points sideways; its lid points
straight up. At the rim those are two different directions, so the rim needs two
copies - exactly the same reason the cube needs 24 instead of 8.

### 3.3 Two ways to decide which way a surface faces

| | Written down (analytic) | Averaged (`computeSmoothNormals`) |
|---|---|---|
| Where it comes from | A formula, from knowing the shape | Adding up the directions of the triangles that touch each corner |
| Example | For a ball centred on its own origin, the normal **is** `normalize(position)` | A cube from 8 shared corners |
| Accuracy | Exact | Approximate |
| When to use it | Whenever you can write the formula down | When you cannot |

All five generators use the written-down kind, because for all five shapes the
formula is known. `computeSmoothNormals()` is L9 slide 20 implemented directly, and
it is in the project as a demonstration of the *other* method.

The sharpest moment in Stage B is this: running the averaging formula on the flat
24-vertex cube **provably changes nothing**. Each of its vertices belongs to only
one face, so there is nothing to average. That is why the flat cube cannot be
smoothed, and it is why Phase 23 had to build a *second* cube from 8 shared corners
to demonstrate on. In the debug view the flat cube shows 3 colours and the
shared-corner one shows 15,952.

### 3.4 Division is not size

Two numbers that are easy to confuse and that Stage B deliberately keeps apart:

| | Division | Size |
|---|---|---|
| Names | `CELLS`, `SEGMENTS`, `STACKS`, `SLICES` | `SIZE`, `DIAMETER`, `HEIGHT` |
| Decides | How many triangles the surface is cut into | How big it is on screen |
| Where it lives | In the shape-building function | In the matrix at draw time |
| Changing it | Changes memory on the graphics card | Changes nothing on the graphics card |

Raise the grid's `CELLS` from 8 to 32 and it covers exactly the same ground, cut
into far more, far smaller triangles. In solid shading it looks almost identical,
which is exactly why the `W` wireframe key exists.

### 3.5 What is on screen at the end of Stage B

Ten things drawn, from seven meshes:

| Mesh | Drawn | Where |
|---|---:|---|
| triangle | 2 | The Stage A test triangle and its deeper copy |
| quad | 1 | Middle of the left-hand column |
| grid | 1 | The floor, below everything |
| cylinder | 1 | Top of the left-hand column |
| sphere | 1 | Bottom of the left-hand column |
| cube | 3 | Three sizes, one mesh |
| smoothcube | 1 | Beside the flat cube, for the Phase 23 comparison |

```text
10 draws, 640 triangles, 437 vertices submitted per frame
386 vertices actually stored on the graphics card
 51 vertices saved by reuse  (triangle +3, cube +48)
```

---

## 4. Phase-by-phase audit

### 4.0 Summary table

| Phase | What it added | Checkpoint | Status |
|---:|---|---|---|
| 14 * | `Mesh.h`: the `Vertex` layout and the `Mesh` class; `makeCube()` | Picture identical to Phase 13, `main.cpp` shorter | **PASS** - frames proven byte-identical |
| 15 | The `N` normals debug view | One flat colour per cube face | **PASS** |
| 16 * | The unit-mesh rule and `drawMesh()` | Three sizes from one mesh | **PASS** |
| 17 | `makeQuad()` | Debug view confirms it faces `+Z` | **PASS** - frame byte-identical to Phase 16 |
| 18 | `makeGrid(N)`, the first parameterised shape | A grid with `N` as a named value | **PASS** |
| 19 (o) | Wireframe comparison and printed counts | `N=4` vs `N=32` obviously different | **PASS** - also fixed Stage A finding F1 |
| 20 | `makeCylinder()`, wall only | An open tube | **PASS** - counts superseded by 21, annotated |
| 21 (o) | The cylinder's two end caps | A closed, watertight solid | **PASS** - watertightness tested exactly |
| 22 | `makeSphere()`, two parameters | The classic RGB ball | **PASS** |
| 23 * | `computeSmoothNormals()` and a shared-corner cube | The two cubes obviously different | **PASS** - 3 colours vs 15,952 |
| 24 (o) | Live counters in the window title | Reported triangles match a hand count | **PASS** - 640/10 matched exactly |
| 25 | `rebuildMeshes()` on `+` and `-` | Count changes live; 100 rebuilds leak nothing | **PASS** - proven with a negative control |

### 4.1 Phase 14 - the Mesh class

Before this phase, `main.cpp` had three separate structs of raw graphics handles,
two of them identical to each other, plus six create-and-destroy functions. All of
that became one `Mesh` class.

The class does three things a bare struct of handles could not: it remembers how
many triangles to draw, it frees its own graphics memory, and it refuses to be
copied. That last one matters - copying would give two objects the same handles,
and the second one to be destroyed would try to free memory that was already
freed.

**Checked:** the rendered frame was proven byte-for-byte identical to Phase 13's,
by comparing checksums of the finished images. A refactor that changes the picture
is not a refactor.

### 4.2 Phase 15 - seeing a normal

A normal is an invisible number until something draws it. This phase makes the
fragment shader paint each pixel from the normal instead of the colour:

```glsl
FragColor = vec4(vNormal * 0.5 + 0.5, 1.0);
```

Normals run from -1 to +1; screen colours run from 0 to 1. The `* 0.5 + 0.5`
converts between them, so `-1` becomes black, `0` becomes mid-grey and `+1` becomes
full brightness. A face pointing along `+X` turns bright red.

The reason this comes now rather than with the lighting is simple: once a normal is
inside a lighting equation, a wrong one just looks like slightly odd shading. On its
own, it is one flat predictable colour that you can *predict before pressing the
key* and check.

### 4.3 Phases 16-17 - the unit-mesh rule arrives

Phase 16 took the size parameter away from `makeCube()` and put a `glm::scale` in
`drawMesh()` instead. Phase 17 then did the simplest possible generator, the quad,
to show that the rule works for a new shape and not just the cube.

**Checked:** Phase 17's frame is byte-for-byte identical to Phase 16's. The quad's
hand-written numbers were replaced by a loop that produces the same numbers in the
same order.

### 4.4 Phases 18-19 - the first parameterised shape, and how to see it

`makeGrid(N)` is the first shape whose *memory footprint* depends on a number we
choose: `(N+1)^2` corner points and `2N^2` triangles.

Phase 19 adds no rendering code at all. It makes the change **visible**, by printing
the counts the formula predicts when the wireframe key is pressed, so `2 * N * N`
can be checked against what you can count by eye.

Phase 19 also fixed the one real defect Stage A's review found (F1): the test
triangle's size pulse had been running over the wrong range since Phase 6,
multiplying by `0.1` where it should have multiplied by `0.5`, so the triangle shrank
to a dot and came back inside out.

A measured observation worth keeping from this phase: in wireframe, the amount of
ink on screen scales with `N`, not with `N^2`. There are four times as many edges at
twice the division, but each is half as long. The pixels-per-unit-length figure
stayed between 436 and 467 across a sixteenfold change in `N`.

### 4.5 Phases 20-21 - the first curved surface, then closing it

Phase 20 builds the cylinder's wall only, from a ring of `sinf`/`cosf` points. It is
the first mesh whose normals differ from point to point, so the debug view sweeps
through the colours around its circumference instead of showing flat patches.

It is deliberately an **open tube** - you can see up through the missing lids - and
Phase 21 closes it. Each lid is a centre point plus a fan of triangles. The lids
need their own copies of the rim points, because the wall faces sideways there and
the lid faces straight up.

**Checked exactly:** every edge in the closed cylinder is used by exactly two
triangles, compared by position. The vertex count went from `2S` to `4S+2`.

**One superseded number, annotated rather than hidden:** Phase 20's checkpoint
recorded `32 / 96 / 32` for the cylinder. Phase 21's caps changed it to
`66 / 192 / 64`. Phase 20's document says so.

### 4.6 Phase 22 - the sphere

The first shape with **two** division numbers, and they are deliberately given
*different* values - 12 stacks and 18 slices. If both were 16, a bug that mixed
latitude up with longitude would look perfectly fine. At 12 and 18 it shows
immediately.

Its normal is the simplest in the project: for a ball centred on its own origin,
`normalize(position)` **is** the surface normal. Checked numerically - the largest
difference anywhere was below one ten-thousandth.

The poles are shared triangle fans rather than collapsed rows of a lattice, so the
mesh contains no zero-area triangles. Checked at every detail setting.

### 4.7 Phase 23 - the other kind of normal

Covered in 3.3. The one thing worth repeating: the two cubes are the same twelve
triangles at the same size in the same rotation. The **only** difference is how
their normals were decided. That is what makes the comparison fair.

### 4.8 Phase 24 - counting what a frame costs

Every draw in the project goes through `drawMesh()`, so that is where the counting
happens:

```cpp
++stats.drawCalls;
stats.vertices += mesh.vertexCount();
stats.triangles += mesh.triangleCount();
```

Counting at the single place every draw passes through means the totals **cannot**
drift away from reality. Add a draw, remove one, or accidentally issue one twice,
and the numbers change by themselves.

The distinction this phase is really about:

| | The `W` key (Phase 19) | The window title (Phase 24) |
|---|---|---|
| Where the numbers come from | The configuration, by formula | The meshes, as they are drawn |
| What it tells you | What the code **intends** | What the code **did** |
| If the code is wrong | Still prints the intended number | Prints the wrong number, visibly |

Both are worth having, and the reason is the point: **a prediction and a measurement
that disagree have found you a bug.**

### 4.9 Phase 25 - changing the geometry while it runs

`+` and `-` move one detail level, and the grid, cylinder and sphere are built again
from it while the scene keeps turning.

| Level | Grid | Cylinder | Sphere | Draws | Triangles |
|---:|---:|---:|---:|---:|---:|
| -2 | 2 | 4 | 3 x 4 | 10 | 92 |
| -1 | 4 | 8 | 6 x 9 | 10 | 206 |
| **0** | **8** | **16** | **12 x 18** | **10** | **640** |
| +1 | 16 | 32 | 24 x 36 | 10 | 2,348 |
| +2 | 32 | 64 | 48 x 72 | 10 | 9,124 |
| +3 | 64 | 128 | 96 x 144 | 10 | 36,116 |

**The draw count is 10 at every single level.** That is the most important column.
Fifty-six times the geometry, the same ten draws, because detail changes how finely
the same objects are divided and never how many objects there are. Those two costs
are independent and are improved by completely different means, which is the whole
argument Phase 62 will make.

The line that makes this phase safe was written in **Phase 14**:

```cpp
bool upload(...)
{
    // Uploading into a Mesh that already holds buffers would leak the old
    // ones. Freeing first also makes this safe to call twice, which
    // Phase 25's rebuildMeshes() will rely on.
    destroy();
```

That comment named this phase by number, eleven phases early. It was not written
because a leak had been seen; it was written because somebody asked "what happens if
this is called twice?" and answered it in the code.

---

## 5. Findings

### 5.1 Defects found and fixed inside Stage B

| # | Phase found | What it was | Status |
|---|---|---|---|
| B1 | 19 | The Stage A pulse defect: the test triangle's size multiplied by `0.1` instead of `0.5`, so it shrank to a dot and returned inside out | **FIXED** |
| B2 | 18 | `CubeConfig::COUNT` said 4 while only 3 positions existed, breaking the build | **FIXED** - the count is now derived from the array, with a compile-time check that the two arrays match |
| B3 | 24 | A Debug-only linker warning (`LNK4098`) present since **Phase 1**: the supplied GLFW library was built against the release C runtime, so a Debug build asked for two runtimes | **FIXED** - the Debug link now excludes the release one |
| B4 | 25 | Detail was not reversible. Halving a running value loses the remainder, so `-` `-` `+` `+` left the sphere at 12 x 12 instead of 12 x 18 - silently undoing Phase 22's reason for choosing unequal numbers | **FIXED** - every level is now worked out from the level-0 value, and a compile-time check enforces it |
| B5 | 25 | The `W` key still predicted the grid's counts from the *starting* division after detail became changeable, so it would have kept printing 8 while the grid was built at 16 | **FIXED** |

B4 is the one worth dwelling on, because the program looked completely fine with the
bug in it. The sphere at 12 x 12 renders perfectly. What it loses is the *ability to
detect a different bug* - the whole reason Phase 22 made those two numbers
different. A compile-time check now makes it impossible:

```cpp
static_assert(sphereDivisionsDifferAt(-2) && ... && sphereDivisionsDifferAt(3),
              "sphere STACKS and SLICES must differ at every reachable detail level");
```

### 5.2 Faults found in the testing itself

Worth recording separately, because a test that cannot fail is worse than no test:
it produces false confidence.

| # | What was wrong | Fix |
|---|---|---|
| T1 | The strict-warning build was verified from Git Bash, which rewrote the `/W4` flag into a file path. A failed configure then looked identical to a clean build | Verified from PowerShell, three ways: the cached flag, the generated project's warning level, and `/W4` on the real compiler command line |
| T2 | Passing `/W4` alone replaced the build system's default flags, dropping `/EHsc` and producing a warning from inside a system header that had nothing to do with the project | Pass the defaults explicitly alongside it |
| T3 | Build logs were filtered for *compiler* warnings. A linker warning carries no `C####` code, so B3 hid for 24 phases | Logs are now scanned for `warning` and `error` anywhere |
| T4 | The first leak test skipped its check whenever the graphics driver reused the same handle - which is the normal case, and is itself the proof. It reported "nothing leaked" having tested nothing | Rewritten around handle reuse, and given a negative control |
| T5 | A "no shader problem reported" check only tested that the word *shader* appeared somewhere in the output | Replaced with a check for the linked-program line and the absence of failure text |
| T6 | A table of triangle counts was written into a source comment from arithmetic done by hand. Level +2 was wrong by 96 | Every number in that table is now measured by the program |

### 5.3 How "no leak" was actually proven

"Nothing leaked" is the hard thing to show, because a leak is invisible - the program
behaves identically until it runs out of memory.

The test uses the fact that OpenGL reuses object numbers. Ask for a new one and the
driver gives you the lowest free number. After 100 rebuilds, 300 uploads:

```text
rebuilds that got the SAME vertex-array number back: 100 of 100
lowest free buffer number before : 15      after : 15
```

A number can only be reissued if it is free, and it can only be free if the old
object was really deleted. But those results would *also* appear if the test were
simply blind. So the whole thing runs a second time with one line disabled - the
`destroy()` call - reintroducing exactly the leak:

```text
rebuilds that got the SAME number back: 0 of 100
lowest free buffer number  15 -> 615        vertex array  8 -> 308
```

300 uploads leaked exactly 600 buffers (two each) and exactly 300 vertex arrays. The
test did not merely notice the leak - it counted it precisely. **That** is what makes
the passing run mean something.

### 5.4 Still open, and deliberate

**The test triangle passes through the largest cube about 0.81% of the time.**

An exact test over 1,006,783 sample positions measured it. It renders correctly -
depth testing handles it properly, which is arguably Phase 8 working - and moving a
cube would invalidate measured pixel evidence in three separate documents. Left alone
on purpose. Recorded in full at the end of Phase 20's document.

### 5.5 Small things noticed, not worth changing

- Several comments still describe future phases in the present tense. They are
  accurate about *intent* and are useful as signposts.
- The `Vertex` struct carries a `color` the plan did not originally specify. It is
  needed because until materials arrive at Phase 29, the per-vertex colour is the
  only thing giving the cube its six faces. It is expected to disappear then.
- The Linux build path has not been exercised recently.

---

## 6. Controls at the end of Stage B

| Key | What it does | What it proves |
|---|---|---|
| `N` | Paint every pixel from its normal | Every normal in the scene is correct, before any lighting can hide a mistake |
| `W` | Wireframe, with face-hiding switched off | How finely a shape is divided; that a wrongly-wound face is still there |
| `D` | Depth test off and on | The nearer object wins because of depth testing, not draw order |
| `O` | Build the transform matrices backwards | Why `T * R * S` is the order it is |
| `+` / `-` | Raise / lower detail and rebuild | Division is separate from size; triangle count and draw count are separate costs |
| mouse drag | Orbit the camera | Inspect any object from any angle |
| scroll | Zoom | - |
| `ESC` | Close | - |

Every toggle uses the same "was it already down last frame" check, so holding a key
flips the state once rather than about 120 times a second. For `+` and `-` this
matters more than for the others: a held key would otherwise rebuild three meshes on
the graphics card every frame.

---

## 7. Who frees what

| Thing | Freed by | When |
|---|---|---|
| Each mesh's VAO, VBO and EBO | `Mesh::destroy()`, called explicitly in `main()` | Before the window is destroyed |
| The same, on a rebuild | `Mesh::upload()` calling `destroy()` first | Before each new upload |
| The shader program | `ShaderProgram::destroy()` | Before the window is destroyed |
| The window and GLFW | `glfwDestroyWindow`, `glfwTerminate` | Last |

The explicit calls matter. A `Mesh` declared inside `main()` would otherwise be
destroyed *after* `glfwTerminate()` has already destroyed the graphics context, and
deleting a graphics object with no context is not valid. Calling `destroy()`
explicitly frees everything at the right moment and leaves every handle at zero, so
the later automatic destructor finds nothing to do.

---

## 8. Stage B viva pack

### 8.1 Likely questions

| Question | Answer |
|---|---|
| Why does a cube need 24 vertices and not 8? | A vertex carries one normal. At each of the 8 corners, three faces meet facing three different directions, so each corner must be stored once per face. |
| Then why does the sphere share almost everything? | Because nothing disagrees anywhere on a smooth ball - each point has exactly one surface direction. |
| Why does the cylinder need `4S+2`? | The wall points sideways at the rim and the lid points straight up, so the rim needs two copies. Same reason as the cube. |
| Where is scaling used? | Every object is a 1-unit mesh scaled to its real size at draw time. That *is* the mesh-reuse optimisation. |
| How do you know a normal is right? | Press `N`. Each one becomes a flat, predictable colour you can state before pressing the key. |
| Why is there no lighting yet? | So that a wrong normal is visible. Inside a lighting equation it would just look like slightly odd shading. |
| What is the difference between `CELLS` and `SIZE`? | `CELLS` is how finely the surface is cut up; `SIZE` is how big it is. Changing `CELLS` changes graphics memory; changing `SIZE` changes only a matrix. |
| Why can the flat cube not be smoothed? | Each of its vertices belongs to exactly one face, so there is nothing to average. Running the formula on it provably changes nothing. |
| What happens to the old mesh when you press `+`? | `upload()` deletes the old VAO, VBO and EBO before uploading the new ones. Without that one line, every press would abandon three buffers permanently. |
| How do you know it does not leak? | The driver reissued the same handle on all 100 rebuilds, which is only possible if the old one was deleted. With `destroy()` disabled, the lowest free handle climbs from 15 to 615 - so the test can detect a leak. |
| Why does the draw count stay at 10 when triangles go up 56 times? | Detail changes how finely the same objects are divided, not how many objects there are. The two costs are independent. |
| Why is the window title a HUD? | Real text needs a font, a texture atlas, a shader and geometry. The title bar needs one function call - and unlike a console window, it appears in a screen recording. |

### 8.2 A two-minute Stage B demonstration

| Time | Do this | Say this |
|---|---|---|
| 0:00 | Scene running, point at the three cubes | "Three sizes, one mesh, one upload. The size is in the matrix, not the data." |
| 0:20 | Press `N` | "Every normal in the scene, visible. The `+X` face is red because its normal is `(1,0,0)`." |
| 0:40 | Point at the two cubes on the right, still in `N` | "Same twelve triangles. The left one has 24 vertices and 3 colours; the right has 8 shared vertices and 15,952. That is the difference between a written-down normal and an averaged one." |
| 1:00 | Press `N` off, press `W` | "Wireframe. Watch the console - it prints what the formula predicts, so you can check it by eye." |
| 1:15 | Press `+` twice, still in wireframe | "Rebuilt on the graphics card while running. Same size, far more triangles - division and size are separate." |
| 1:35 | Point at the window title | "Measured, not predicted: 9,124 triangles now. But still ten draws. Detail is never more objects." |
| 1:50 | Press `-` twice | "Back to exactly 640. Each level is worked out from the original number, so it is reversible." |

---

## 9. Before starting Phase 26

Stage C is lighting, and it is where the marks are. Four things from Stage B that
Stage C depends on, and must not break:

1. **The `N` debug view must keep working.** Phase 26's whole checkpoint is that the
   debug view stays correct on a stretched cube. If the debug view breaks, Phase 26
   cannot be demonstrated.
2. **Normals are currently handed to the fragment shader untransformed**, still in
   the shape's own space. `shaders/basic.vert` says so in a comment that names
   Phase 26. That is exactly what Phase 26 changes.
3. **The unit-mesh rule means non-uniform scaling is everywhere.** The hull will be
   a stretched cube; the cylinder's scale is already `(DIAMETER, HEIGHT, DIAMETER)`.
   This is precisely the situation that makes the normal matrix necessary rather than
   optional, so the project already has the problem Phase 26 solves.
4. **The two Stage C demonstrations need the detail control from Phase 25.** Demo A
   needs the grid at 8-16 divisions; Demo B needs the cylinder at 6-8 segments. The
   `+`/`-` keys are how a teacher will be asked to reach them.

One thing to carry forward as a habit rather than a rule: the `destroy()` call that
made Phase 25 safe was written eleven phases before it was needed, by asking "what
happens if this is called twice?" The equivalent question for Stage C is "what
happens if this object is scaled unevenly?" - and the answer is Phase 26.

---

## Appendix A - the numbers you can change in Stage B

| Where | Name | Value | Changing it affects |
|---|---|---|---|
| `GridConfig` | `CELLS` | 8 | Grid divisions at level 0 |
| `GridConfig` | `CELLS_MIN` | 2 | The floor `-` cannot go below |
| `GridConfig` | `SIZE` | 4.0 | How big the floor is |
| `CylinderConfig` | `SEGMENTS` | 16 | Roundness of the tube at level 0 |
| `CylinderConfig` | `SEGMENTS_MIN` | 4 | The floor; at 4 it is a square prism |
| `CylinderConfig` | `DIAMETER`, `HEIGHT` | 0.5, 0.9 | Its real size |
| `SphereConfig` | `STACKS`, `SLICES` | 12, 18 | Latitude and longitude at level 0 |
| `SphereConfig` | `STACKS_MIN`, `SLICES_MIN` | 3, 3 | The fewest that still enclose a volume |
| `SphereConfig` | `DIAMETER` | 0.72 | Its real size |
| `DetailConfig` | `LEVEL_MIN`, `LEVEL_MAX` | -2, +3 | How far `+` and `-` reach |
| `CubeConfig` | `POSITIONS`, `SCALES` | 3 each | How many cubes and how big; the count is derived |
| `QuadConfig` | `SIZE` | 0.8 | The quad's size |

## Appendix B - requirement compliance (Stage B)

| Requirement | Status |
|---|---|
| C++17, OpenGL 3.3 Core, GLSL 330, GLFW, GLAD, GLM, CMake | met |
| No Python anywhere | met |
| No game engine, physics engine, scene-graph library, model loader, GUI toolkit | met |
| One concept and one visual checkpoint per phase | met, 12 of 12 |
| Named, centralised tunable values | met - see Appendix A |
| Parent matrices unscaled; scale applied only at draw time | met - the Phase 16 rule, enforced throughout |
| `w = 1` for positions, `w = 0` for directions | not yet exercised - no direction is transformed until Phase 26 |
| No pre-computed animation | met - all motion is a function of elapsed time |

## Appendix C - verification totals

```text
shape maths     2,946 checks   0 failures
live graphics      32 checks   0 failures
build and run      32 checks   0 failures
                -----------
total           3,010 checks   0 failures

builds: Debug, Release, strict /W4   - no warnings of any kind
runs:   Debug and Release            - exit 0, nothing on the error stream
```
