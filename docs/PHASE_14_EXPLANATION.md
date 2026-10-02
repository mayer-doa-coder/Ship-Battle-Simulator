# Phase 14 - One Vertex, One Mesh

## Status

Phase 14 is complete and verified. Debug and Release builds succeeded with no
compiler warnings, both shaders linked, and a live Release run drew the same
scene Phase 13 drew. Nothing was printed to the error output, and the program
closed cleanly.

This is a **refactor** phase, and it is a graded milestone (`*` in the plan).
The whole point of a refactor is that the picture must not change, so "nothing
looks different" is the result we want, not a disappointment. That claim was
**measured**, not judged by eye:

| Check | Result |
|---|---|
| Generated cube's 24 **positions** vs Phase 13's hand-typed ones | **0 differ** |
| Generated cube's 24 **colours** vs Phase 13's | **0 differ** |
| Generated cube's 36 **indices** vs Phase 13's | **0 differ** |
| All 12 triangles still wound counter-clockwise seen from outside | 12 of 12 |
| Each stored normal agrees with the face it belongs to | 24 of 24 |
| All 24 normals are exactly unit length | 24 of 24 |
| `makeCube(0.9)` equals `makeCube(0.5)` scaled by `1.8` | yes |
| `sizeof(Vertex)` and its field offsets | 36 bytes; 0, 12, 24 |
| Startup report | `triangle 3/3/1`, `quad 4/6/2`, `cube 24/36/12` |
| Strict-warning build (`/W4 /permissive-`) | clean |

### The rendered frame itself was compared, pixel by pixel

The strongest check available for a refactor was run directly. The Phase 13
source was taken out of git and the Phase 14 source as it stands now; both were
built, both had their clock pinned to exactly `t = 2.0 s` so the animation
would be at the identical instant, and both had their finished frame read back
out of the framebuffer with `glReadPixels` and written to a file.

```text
frame size                : 1280 x 720, 2 764 800 colour bytes each
differing bytes           : 0
largest single difference : 0
sha256, Phase 13 frame    : 9879EC77E7D7DEDD...B7FDF1097
sha256, Phase 14 frame    : 9879EC77E7D7DEDD...B7FDF1097
```

The two images are **byte-for-byte identical**. To confirm this is a real
picture and not two identically blank screens, the frame was also measured: it
contains **30 551 distinct colours**, and **122 064 pixels (13.2%)** of it are
geometry rather than background.

So the refactor is proven clean at both ends - the numbers that go into the GPU
and the pixels that come out of it.

## What changed

| File | Change |
|---|---|
| `src/Mesh.h` | **New file.** `struct Vertex`, `class Mesh`, `buildCubeGeometry()`, `makeCube()` |
| `src/main.cpp` | `TriangleGpu`, `QuadGpu`, `CubeGpu` deleted |
| `src/main.cpp` | `createTriangle`/`destroyTriangle`, `createQuad`/`destroyQuad`, `createCube`/`destroyCube` deleted - six functions replaced by one `createMeshes()` |
| `src/main.cpp` | The cube's 24-row vertex array and 36 indices deleted; only `FACE_COLORS[6]` remains |
| `src/main.cpp` | The triangle's and quad's rows are now `Vertex` values instead of loose floats |
| `src/main.cpp` | `renderScene()` takes three `const Mesh&` and calls `mesh.draw()` |
| `shaders/basic.vert` | Attribute `1` is now `aNormal`; the colour moved to attribute `2` |
| `shaders/basic.frag` | Unchanged |

`src/main.cpp` went from **1125 lines to 892** - 233 lines gone, with no
feature lost. `src/Mesh.h` is 331 lines, most of it comments, so the project as
a whole grew slightly. That is the normal shape of a good refactor: the file
you read every day gets smaller, and the knowledge moves somewhere it can be
reused.

## The one idea

Three times now, this project has written out the same recipe:

```text
Phase 2  (triangle)   make a VAO, make a VBO, upload, describe the layout
Phase 9  (quad)       make a VAO, make a VBO, make an EBO, upload, describe the layout
Phase 10 (cube)       make a VAO, make a VBO, make an EBO, upload, describe the layout
```

Phase 9's and Phase 10's versions were almost character-for-character the same.
Nothing was wrong with any of them - but three copies of one idea is three
places to make a mistake, and three places to fix when something changes.

**Phase 14's one idea is to write that recipe once**, inside a type that also
remembers what it made and can throw it away again.

```text
before                               after
------                               -----
struct TriangleGpu { vao, vbo }      Mesh triangleMesh;
struct QuadGpu { vao, vbo, ebo }     Mesh quadMesh;
struct CubeGpu { vao, vbo, ebo }     Mesh cubeMesh;
createTriangle() / destroyTriangle()
createQuad()     / destroyQuad()     createMeshes()   <- one function
createCube()     / destroyCube()
```

## Why a `struct Vertex` instead of six loose floats

Until now, a vertex was six floats in a row, and the code told OpenGL how to
divide them up with numbers counted by hand:

```cpp
// the old way - every number here was worked out by a human
const GLsizei stride = 6 * sizeof(float);
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                      reinterpret_cast<const void*>(3 * sizeof(float)));
```

Those hand-counted offsets are a real hazard. Get one wrong and OpenGL reads
the colour where the position should be. There is no error message; the shape
simply comes out wrong.

A struct says the same thing once, and lets the **compiler** do the counting:

```cpp
struct Vertex {
    glm::vec3 position;   // where this corner is
    glm::vec3 normal;     // which way the surface faces here
    glm::vec3 color;      // this corner's own colour
};
```

```cpp
const GLsizei stride = sizeof(Vertex);                       // the compiler knows: 36
offsetof(Vertex, position)                                   // the compiler knows: 0
offsetof(Vertex, normal)                                     // the compiler knows: 12
offsetof(Vertex, color)                                      // the compiler knows: 24
```

`sizeof` asks "how big is one of these?" and `offsetof` asks "how far into it
does this field start?". Both are answered while the program is being compiled,
so they cannot disagree with the struct. If a field is ever added, moved, or
resized, every one of those numbers updates by itself.

This was checked: `sizeof(Vertex)` is `36` bytes and the three offsets are
`0`, `12`, and `24` - three `glm::vec3` values packed end to end, exactly as
expected, with no hidden padding.

## What the `Mesh` owns, and when it is freed

This is the phase's viva question in the plan, so it is worth being precise.

A `Mesh` owns **three OpenGL objects**:

| It owns | What that object holds |
|---|---|
| A **VAO** | The layout: which attribute reads which bytes, and which EBO to use |
| A **VBO** | The vertex data itself, on the GPU |
| An **EBO** | The index list, on the GPU |

It also remembers two plain numbers - how many vertices and how many indices -
because `glDrawElements` needs the index count, and because a mesh that cannot
say how big it is cannot report itself in Phase 24's counters.

**When are they freed?** There are two paths, and only one of them is the real
one:

```cpp
cubeMesh.destroy();      // main(), before glfwDestroyWindow - THIS is the real one
```

```cpp
~Mesh() { destroy(); }   // the safety net - by now there is nothing left to do
```

The order matters, and getting it wrong is a genuine bug. A `Mesh` declared
inside `main()` is destroyed automatically when `main()` ends - but that is
**after** `glfwTerminate()` has already destroyed the OpenGL context, and
deleting a GPU object when there is no context is not valid. So `main()` calls
`destroy()` explicitly, at the right moment, while the context is still alive.
`destroy()` sets every handle back to `0`, so when the destructor runs later it
finds `0` everywhere and does nothing.

This is exactly the arrangement `ShaderProgram` has used since Phase 2. Copying
a `Mesh` is deliberately forbidden for the same reason it is forbidden for
`ShaderProgram`: two copies would hold the same handles, and the second one to
be destroyed would try to delete GPU objects that were already deleted.

## Generating the cube instead of typing it

Phase 10 typed out 24 vertices and 36 indices by hand and checked all six
faces' winding by hand. Phase 14 replaces both arrays with a loop.

The trick is to describe each face by **three directions** rather than four
corners:

```cpp
struct Face {
    glm::vec3 normal;   // which way the face looks, straight out of the cube
    glm::vec3 right;    // one step across the face
    glm::vec3 up;       // one step up the face
};
```

Those three are chosen so that `right` crossed with `up` gives `normal`. That
one rule is what makes the corner loop come out **counter-clockwise as seen
from outside**, which is the winding `GL_CULL_FACE` needs (Phases 9 and 11).
Getting it right once, in the table, replaces checking it six separate times.

From those three directions, a corner is just a short walk:

```text
centre of the face  =  normal * halfSize
a corner            =  centre  ±  right * halfSize  ±  up * halfSize
```

and the four corners are walked bottom-left, bottom-right, top-right, top-left,
which is counter-clockwise. The indices are then the same six-number pattern
the quad introduced in Phase 9 (`0,1,2, 2,3,0`), once per face, shifted along
by 4 each time.

### The proof that nothing changed

A generator is only trustworthy if it produces what the hand-written version
produced. So the two were compared directly: the Phase 13 array was taken out
of git, the Phase 14 generator was run, and all 24 vertices and 36 indices were
compared one by one.

```text
positions identical to Phase 13 : YES (0 differ)
colours   identical to Phase 13 : YES (0 differ)
indices   identical to Phase 13 : YES (0 differ)
```

Not "close enough" - the same numbers, in the same order. This is the
strongest form the Phase 14 checkpoint can take, and it is worth showing to a
teacher who asks "how do you know your loop is right?".

## Why everything is indexed now

`Mesh` always draws with `glDrawElements`, so the triangle - which has no
shared corners at all - was given three indices of its own:

```cpp
const unsigned int INDICES[] = { 0, 1, 2 };
```

Those indices say exactly what `glDrawArrays` was doing on its own: use corner
0, then 1, then 2. Nothing about the picture changes.

This is a **deliberate trade**, and it is fair to be asked about it. The cost
is that `glDrawArrays` no longer appears anywhere in the project, so Phase 2's
original draw call is now only in the documents and in git history. The gain is
that there is one drawing path instead of two, which means one thing to explain
and one thing that can break. Every mesh from Phase 17 onward - quad, grid,
cylinder, sphere - produces indices anyway, so the indexed path is the one that
had to exist regardless.

## Why the normal arrives now, before anything reads it

`Vertex` has a `normal` field, every mesh fills it in, and **no line of shader
code reads it yet**. Phase 15's debug view is the first to use it.

That is on purpose, and the plan asks for it. A normal is a property of the
*surface*, and the only moment anyone truly knows which way a surface faces is
the moment the geometry is being built. Bolting normals on afterwards means
guessing, and a wrong normal is almost invisible once it is buried inside a
lighting equation - which is exactly why Phase 15 brings the debug view forward
rather than waiting for the lighting stage.

So from this phase onward, every generator has to get its normals right while
it still has the information to do so. The cube's were checked: each face's
stored normal points the same way as the face's actual geometry, and all 24 are
exactly unit length.

In the shader, the unused attribute is simply removed by the GLSL compiler.
That is harmless - an unused **attribute** is just not fetched. It is worth
contrasting with Phase 3's lesson: an unused **uniform** also disappears, but
there the disappearance is dangerous, because writing to it then silently goes
nowhere.

## The shader's attribute numbers moved

```glsl
layout (location = 0) in vec3 aPosition;   // unchanged
layout (location = 1) in vec3 aNormal;     // NEW - was aColor
layout (location = 2) in vec3 aColor;      // moved here to make room
```

These numbers and the field order in `struct Vertex` have to agree, because
`Mesh::upload()` sets attribute `0` from `position`, `1` from `normal`, and `2`
from `color`. If they ever disagree, the cube's colours would come out as
normals. Keeping the shader's three `layout` lines in the same order as the
struct's three fields is what keeps that obvious.

## Where each job happens

| Function | Job in this phase |
|---|---|
| `Mesh::upload()` | Creates the VAO/VBO/EBO, sends the data, describes the layout once |
| `Mesh::draw()` | Binds the VAO and issues `glDrawElements` |
| `Mesh::destroy()` | Deletes all three GPU objects and forgets their handles |
| `buildCubeGeometry()` | Works out the cube's 24 vertices and 36 indices - **no OpenGL at all** |
| `makeCube()` | Calls the above, then uploads the result |
| `createMeshes()` (main.cpp) | Builds all three meshes at startup |
| `renderScene()` | Sets `uTint` and `uModel`, then asks each mesh to draw itself |

Notice that `buildCubeGeometry()` touches no OpenGL. That split is what let the
numbers be checked against Phase 13 in an ordinary test program, with no window
and no graphics card involved. It is also what Phase 23 needs, when
`computeSmoothNormals()` has to change vertex data *before* it is uploaded.

## What `renderScene()` looks like now

```cpp
shader.setVec3("uTint", AppConfig::TINT);
shader.setMat4("uModel", scene.triangleModel);
triangleMesh.draw();
```

Three lines per object, and all three say something about **this** object.
Binding the right VAO, choosing between `glDrawArrays` and `glDrawElements`,
and knowing how many indices there are have all moved inside `Mesh::draw()`,
because those are facts about the mesh, not decisions the renderer should be
making each time.

## Likely teacher questions

### What does a `Mesh` own?

Three OpenGL objects - a VAO, a VBO, and an EBO - plus the vertex and index
counts needed to draw it.

### When are those objects freed?

When `destroy()` is called. `main()` calls it explicitly before the window and
the OpenGL context are destroyed, because deleting a GPU object without a
context is not valid. The destructor calls `destroy()` too, as a safety net,
but by then every handle is already `0` so it does nothing.

### Why is copying a `Mesh` forbidden?

Both copies would hold the same VAO, VBO, and EBO numbers. When the second one
was destroyed it would delete GPU objects that had already been deleted. The
same reason `ShaderProgram` forbids copying.

### Why use a struct for a vertex instead of loose floats?

So the compiler works out the stride and the offsets instead of a human
counting them. `sizeof(Vertex)` and `offsetof(Vertex, color)` cannot disagree
with the struct, and they update by themselves if a field is ever added.

### What is `offsetof` doing?

Asking, at compile time, how many bytes into a `Vertex` a particular field
begins. `glVertexAttribPointer` needs exactly that number to know where to
start reading each attribute.

### Why is the normal there if nothing reads it?

Because the only moment anyone reliably knows which way a surface faces is
while its geometry is being built. Adding normals later means guessing. Phase
15 is the first phase to read them, and the lighting phases are what they
really exist for.

### Does an unused attribute cause a problem?

No. The GLSL compiler removes it and OpenGL simply does not fetch it. This is
different from an unused **uniform**, where Phase 3 showed that writing to the
removed name fails silently and is genuinely dangerous.

### Why does the triangle need indices now when it has no shared corners?

Because `Mesh` always draws with `glDrawElements`, so every mesh needs an index
list. The triangle's is `{0, 1, 2}`, which is exactly the order `glDrawArrays`
walked through by itself. One drawing path is easier to explain and harder to
break than two.

### How do you know the generated cube is the same as the typed-out one?

They were compared number by number against the Phase 13 version taken from
git: all 24 positions, all 24 colours, and all 36 indices are identical, with
zero differences.

### How does the generator get the winding right on all six faces?

Each face is described by a normal, a `right` direction, and an `up` direction,
chosen so that `right` crossed with `up` equals the normal. Walking the four
corners bottom-left, bottom-right, top-right, top-left is then automatically
counter-clockwise as seen from outside, which is what `GL_CULL_FACE` keeps.

### Why is `buildCubeGeometry()` separate from `makeCube()`?

So the numbers can be built, checked, and modified before they ever touch the
GPU. That is what allowed the comparison above to run in a plain test program,
and it is what Phase 23 needs when it averages normals across shared vertices.

### Why did the face colours stay in `main.cpp`?

Because the generator's job is the cube's **shape**, and the colours are a
choice about how it **looks**. Keeping them apart is the same separation that
materials formalise in Phase 29 - and it keeps the colours where the other
tuning values live.

### `main.cpp` got shorter, but the project got bigger. Is that a win?

Yes. 233 lines left the file that is read and edited constantly, and what
replaced them is reusable by every mesh in the rest of the project. `Mesh.h` is
larger than it strictly needs to be only because it is heavily commented.

## Simple viva modifications

- **Resize the cube:** change `CubeConfig::HALF_SIZE`. One number now controls
  all 24 generated vertices.
- **Recolour one face:** change one row of `CubeConfig::FACE_COLORS`. Phase 10
  needed four rows edited for this; now it is one.
- **Reorder the faces:** swap two rows of `FACE_COLORS` and watch two faces
  swap colour, proving the generator walks them in the order listed.
- **Break the winding on purpose:** in `buildCubeGeometry()`, swap `right` and
  `up` for one face. That face turns inside out and vanishes, exactly like
  Phase 11's hole - but now one edit does it, instead of six index numbers.
- **Prove the normals are real:** temporarily set the cube's colours from its
  normals to preview Phase 15, or print `v[i].normal` from a test program.
- **Prove the counts:** read the `[mesh]` lines at startup and check the cube's
  `triangles=12` against `6 faces x 2 triangles` by hand.
- **Show the layout is compiler-driven:** print `sizeof(Vertex)` and confirm it
  is 36 - three `vec3`s of 12 bytes each.

## Checkpoint

Phase 14 passes when:

- Debug and Release builds succeed with no compiler warnings;
- both shaders compile and link, with no missing-uniform warning;
- **the picture is identical to Phase 13** - two triangles, a quad, and a
  spinning six-coloured cube, all behaving exactly as before;
- `main.cpp` is visibly shorter and contains no VAO, VBO, or EBO handle of its
  own;
- the `D`, `W`, and `O` keys and the mouse orbit and zoom all still work
  exactly as before;
- the three `[mesh]` lines at startup report `3/3/1`, `4/6/2`, and `24/36/12`;
- the window remains responsive, reports frame timing, and closes cleanly with
  every GPU object freed.

## What is not part of Phase 14

No lighting - `basic.frag` still just multiplies a colour by a tint, and the
normals go unread. No debug view yet: turning normals into colours on a key is
Phase 15. No `makeQuad()`, `makeGrid()`, `makeCylinder()`, or `makeSphere()` -
the quad still carries its four corners written out by hand, and the other
three shapes do not exist. No unit-mesh rule yet: `makeCube()` still takes a
size, and Phase 16 is where scale moves into the model matrix so that one mesh
can be drawn at three different sizes.

**One thing deliberately left alone.** The Stage A review recorded a defect in
the Phase 6 pulse maths (`src/main.cpp`, `* 0.1f` where `* 0.5f` was meant), so
the triangle pulses over the wrong range. It was **not** fixed in this phase,
on purpose: Phase 14's checkpoint is that the picture does not change, and
fixing a separate bug in the middle of a refactor would make it impossible to
tell whether the refactor itself was clean. It should be fixed as its own small
change, before or after this phase, but not inside it.

Phase 15 adds the normals-as-colour debug view: a key that makes the shader
write `N * 0.5 + 0.5` instead of the vertex colour, so that every normal in the
project can be checked by eye before any lighting depends on it.
